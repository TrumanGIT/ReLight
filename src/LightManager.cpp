#include "LightManager.h"
#include "Utility.h"
#include "config.hpp"
#include "ClibUtil/EditorID.hpp"
#include "LightAttachmentHooks.h"
#include <chrono>

//ATTACH LIGHTS AT CORRECT MESH INDEX, USEFULL FOR TORCHES WHERE LIGHT MUST BE INSERTED TO SPECIFIC SPOT
void LightManager::attachLightUsingAttachPath(
	const LightConfig& cfg,
	RE::NiNode* root,
	RE::NiPointLight* light, RE::FormID refFormID)
{
	if (!root || !light) {
		logger::warn("attachLightUsingAttachPath: null root or light");
		return;
	}

	RE::NiAVObject* current = root;

	for (int index : cfg.attachPath) {
		auto* node = current->AsNode();
		if (!node) {
			logger::warn("attachLightUsingAttachPath: object is not a NiNode");
			return;
		}

		auto& children = node->GetChildren();
		if (index < 0 || index >= children.size() || !children[index]) {
			logger::warn("attachLightUsingAttachPath: index {} out of bounds", index);
			return;
		}

		current = children[index].get();
	}

	auto* finalNode = current->AsNode();
	if (!finalNode) {
		logger::warn("attachLightUsingAttachPath: final target is not a NiNode");
		return;
	}

	auto finalNodeName = finalNode->name.c_str();

	finalNode->AttachChild(light);

	logger::debug("attached light to node {} on ref {:08X} ", finalNodeName, refFormID);
}

// ATTACH NI POINT LIGHT TO SHADOW SCENE NODE TO GET BS LIGHT IN RETURN. BS LIGHT IS THE LIGHT YOU VISUALLY SEE RENDERED IN GAME
//BS LIGHT READS ITS NI POINT LIGHT DATA EVERY FRAME
void LightManager::attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::TESObjectREFR* a_this) {

	//logger::info("attempting to create NiPointLight BSlight and attach to ShadowSceneNode");

	if (!niPointLight) {
		logger::error("createShadowSceneNode: niPointLight is null");
		return;
	}

	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params = LightData::makeLightParams(cfg);

	logger::debug("Light paramaters for ref {:08X} created for light {}",a_this->GetFormID(), niPointLight->name.c_str() );

	LightData::printLightParams(params);

	auto* shadowSceneNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

	if (!shadowSceneNode) {
		logger::warn("no shadow scene node to grab in (createShadowSceneNode()");
		return;
	}

	RE::BSLight* bsLight = shadowSceneNode->AddLight(niPointLight, params);

	if (!bsLight) {
		logger::warn("no BSLight {} created for ref {:08X}", niPointLight->name.c_str(), a_this->GetFormID());
		return;
	}
	
	uint32_t mask = cfg.flags; 

	if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kCandle)) {
		bsLight->unk060 = 1; 
	}

	if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kChandelier)) {
		bsLight->unk060 = 2;
	}

	if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kFire)) {
		bsLight->unk060 = 3;
	}
}

std::vector<LightConfig>* LightManager::findConfigsForRef(RE::TESObjectREFR* ref, bool isInterior)
{
	if (!ref)
		return nullptr;

	RE::FormID formID = ref->GetFormID();

	if (isLightPluginFormID(formID)) {
		const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
		if (!refOriginFile)
			return nullptr;

		std::string modName = refOriginFile->fileName;
		toLower(modName);

		std::uint32_t localID = formID & 0x00000FFF;

		if (isInterior) {
			auto modIt = LightData::lightPluginRefFormIDToJsonCfg.find(modName);
			if (modIt == LightData::lightPluginRefFormIDToJsonCfg.end())
				return nullptr;

			auto idIt = modIt->second.find(localID);
			if (idIt == modIt->second.end())
				return nullptr;

			return &idIt->second;
		}
		else {
			auto modIt = LightData::lightPluginRefFormIDToJsonCfgExteriors.find(modName);
			if (modIt == LightData::lightPluginRefFormIDToJsonCfgExteriors.end())
				return nullptr;

			auto idIt = modIt->second.find(localID);
			if (idIt == modIt->second.end())
				return nullptr;

			return &idIt->second;
		}
	}

	if (isInterior) {
		auto it = LightData::refFormIDToJsonCfg.find(formID);
		if (it != LightData::refFormIDToJsonCfg.end())
			return &it->second;
	}
	else {
		auto it = LightData::refFormIDToJsonCfgExteriors.find(formID);
		if (it != LightData::refFormIDToJsonCfgExteriors.end())
			return &it->second;
	}

	return nullptr;
}

bool LightManager::processByFilePath(RE::TESObjectREFR* a_this, std::string meshName, RE::NiNode* a_root, bool isInterior) {



	const auto cfgs = findConfigsForMeshPath(meshName, isInterior);

	// if no config found then move to node name check
	if (cfgs.empty()) {
		//logger::warn("cfgs is empty for ref {:08X}, with name {} ", a_this->GetFormID(), meshName);
		return false;
	}

	if (isExclude(meshName, a_this)) return true; 
	
	const auto refFormID = a_this->GetFormID();

	logger::debug("file path match found {}, Processing ref {:08X} ", meshName, refFormID);

	auto ui = RE::UI::GetSingleton();
	if (ui && ui->IsMenuOpen("InventoryMenu")) {
		return true;
	}

	if (globals::excludedRefFormIDs.contains(refFormID)) {
		logger::debug("excluded ref {:08X} found skipping light attachment", refFormID);
		return true;
	}

	uint32_t flags = cfgs[0].flags;

	// not a multi light, send off to merging logic
	if (cfgs.size() == 1 && !(flags & static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging))) {
		auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());
		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for ref {:08X} with mesh '{}' )", refFormID, meshName);
			return false;
		}

		LightManager::fillPendingMerges(a_this, cloneLight, cfgs[0], a_root);
	}

	// multi lights cant cleanly merge so just attach right now
	else {
		static bool alreadyAttachedDebugMarker = false;

		for (auto& cfg : cfgs) {
			auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());

			if (!cloneLight) {
				logger::warn("Failed to clone NiPointLight for node '{}' for ref {:08X})", meshName, refFormID);
				return true;
			}

			if (!alreadyAttachedDebugMarker) {
				if (globals::enableDebugLightBulbs) AttachDebugMarker(a_root, cloneLight);
			}

				LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, a_this->GetFormID());

				LightData::setNiPointLightDataFromCfg(cloneLight, cfg);

				cloneLight->name = "RL" + cfg.meshPath; 

				attachNiPointLightToShadowSceneNode(cloneLight, cfg, a_this);
		}
	}
	return true;
}

 //TODO:: doesent handle multi lights in a single config (idk if it needs to really) 
// some nodes are called dummy this is to take care of them.
 bool LightManager::dummyHandler(RE::TESObjectREFR* a_this, std::string modelName, RE::NiNode* a_root, bool isInterior)
 {

	 //TODO:: this only works if the node name in the json config is labled exactly as matched below. 
	 // should probly cover cases where the user changed the node name from chandel to something else 
	 static const std::unordered_map<std::string, std::string> dummyMeshPaths = {
		 { "ruinsfloorcandlelampmidon", "ruinsfloorcandlelampmidon" },
		 { "ruinsfloorcandlelampmidon02", "ruinsfloorcandlelampmidon" },
		 { "ruinsfloorcandlelampsmon", "ruinsfloorcandlelampsmon" },
		 { "ruinsfloorcandlelampsmon02", "ruinsfloorcandlelampsmon" },

		 { "impchandelliercandle01", "impchande" },
		 { "impchandelliercandle01uskp", "impchande" },

		 { "candlelanternwithcandle01", "candle" }
	 };

	 auto it = dummyMeshPaths.find(modelName);
	 if (it != dummyMeshPaths.end()) {

		 std::string match = it->second;

		 //cant do multi lights currently, just would have to iterate if wanted to.
		 auto cfgs = findConfigsForNode(match, isInterior);

		 if (cfgs.empty()) return false;

		 LightConfig cfg = cfgs[0];

		 auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());

		 if (!cloneLight) {
			 logger::warn("Failed to clone NiPointLight for node '{}', for ref{:08X})", modelName, a_this->GetFormID());
			 return true;
		 }

		 logger::debug("dummy node {} with model {}. Processing ref {:08X} and got light from config: {}", modelName, modelName, a_this->GetFormID(), cfg.nodeName);
			 LightManager::fillPendingMerges(a_this, cloneLight, cfg, a_root);

		 return true;
	 }

	 // already returned early if not a dummy, therefor might as well skip this object as it wouldent get light anyway
	 return true;
}


 void LightManager::processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this, bool isInterior) {

	// matched name with a configs nodename
	std::string matchStr = match.c_str();

	auto refFormID = a_this->GetFormID(); 

	auto ui = RE::UI::GetSingleton();

	if (ui && ui->IsMenuOpen("InventoryMenu")) {
		//logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
		return;
	}

	if (globals::removeFakeGlowOrbs)
		glowOrbRemover(a_root);

	auto cfgs = findConfigsForNode(matchStr, isInterior);

	if (cfgs.empty()) {
		logger::warn("cfgs is empty for ref {:08X}, with name {} ", refFormID, matchStr); 
		return; 
	}

	logger::debug(" processing ref {:08X} by node name with light {} ", refFormID, matchStr);

	uint32_t flags = cfgs[0].flags; 

	logger::debug(
		"ref {:08X} node '{}' cfg count = {}, flags = 0x{:X}, noMerging = {}",
		refFormID,
		matchStr,
		cfgs.size(),
		flags,
		(flags & static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging)) != 0
	);

	// not a multi light, send off to merging logic
	if (cfgs.size() == 1 && !(flags & static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging))) {
		auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());
		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for ref {:08X} with nodeName '{}' )", refFormID, matchStr);
			return; 
		}

		LightManager::fillPendingMerges(a_this, cloneLight, cfgs[0], a_root);

	}

	// multi lights cant cleanly merge so just attach right now
	else {

		static bool alreadyAttachedDebugMarker = false; 

		for (auto& cfg : cfgs) {

			auto cloneLight = cloneNiPointLight(LightData::masterNiPointLight.light.get());

			if (!cloneLight) {
				logger::warn("Failed to clone NiPointLight for node '{}' for ref {:08X})", matchStr, refFormID);
				return;
			}

			if (!alreadyAttachedDebugMarker) {
				if (globals::enableDebugLightBulbs) AttachDebugMarker(a_root, cloneLight);
				alreadyAttachedDebugMarker = true; 
			}

			LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight, a_this->GetFormID());

			LightData::setNiPointLightDataFromCfg(cloneLight, cfg);

			cloneLight->name = "RL" + cfg.nodeName; 

			attachNiPointLightToShadowSceneNode(cloneLight, cfg, a_this);

			return;
		}
	}
}

//USED TO REINITIALIZE LIGHTS CLEANED BY THE ENGINE. USUALLY WHEN GOING FROM INTERIOR TO EXTERIOR, BACK TO INTERIOR AND VICE VERSA
RE::BSEventNotifyControl LightManager::ProcessEvent(const RE::BGSActorCellEvent* event,
	RE::BSTEventSource<RE::BGSActorCellEvent>*) {

	
if (!event || event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
		return RE::BSEventNotifyControl::kContinue;
	}

	auto player = RE::PlayerCharacter::GetSingleton();

	if (!player) return RE::BSEventNotifyControl::kContinue;

	static bool s_firstCellEvent = true;

	//logger::debug("cell event fired for player");

	auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(event->cellID);
	if (!cell) {
		return RE::BSEventNotifyControl::kContinue;
	}

	// set the current cell is interior or not
	globals::currentCellIsInterior = cell->IsInteriorCell();

	if (s_firstCellEvent) {
	
		s_firstCellEvent = false;

		logger::info("player is in interior on startup: {}", globals::currentCellIsInterior);
		globals::lastCellWasInterior = cell->IsInteriorCell();

			globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
			globals::cellFullyLoaded.store(true);

		return RE::BSEventNotifyControl::kContinue;
	}

	// player changes from interor to exterior or vice versa, must reinitialize lights
	if (globals::lastCellWasInterior !=	globals::currentCellIsInterior ) {

		globals::secondAfterCellFullyLoaded.store(false);

		LightData::ResetTriLightCache();

		globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
		globals::cellFullyLoaded.store(true);

	

		logger::debug(" new cell detected.. islightaffectingsurface hook stopped");

		LightManager::reinitializeLightsWithinRange(player);

	}

	// player changes from interor to another interior, must reinitialize otherwise engine cleans the lights
	if (globals::currentCellIsInterior && globals::lastCellWasInterior) {

		// not a second after cell fully laoded so reset
		globals::secondAfterCellFullyLoaded.store(false);

		LightData::ResetTriLightCache();

		// start timer and say cell fully loaded is true
		globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
		globals::cellFullyLoaded.store(true);


		logger::debug("new cell detected.. islightaffectingsurface hook stopped");

		LightManager::reinitializeLightsWithinRange(player);

	}

	//set the prev cell after finished evaluating new cell
	globals::lastCellWasInterior = globals::currentCellIsInterior;

	return RE::BSEventNotifyControl::kContinue;
}

void LightManager::registerEventSink()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AsBGSActorCellEventSource()->AddEventSink(LightManager::GetSingleton());
		logger::info("BGSActorCellEvent sink registered");
	}
}

void LightManager::reinitializeLightsWithinRange(RE::PlayerCharacter* player) {

	logger::debug("reinitializing lights within range");

	// clear this lights list that has merged lights placed into it so tehey can get lights attached again.
	
	{
		std::scoped_lock lock(globals::refsWithAttachedLightsMutex);
		globals::refsWithAttachedLights.clear();
	}

	{
		std::scoped_lock lock(globals::mergedRefsMutex);
		globals::mergedRefs.clear();
	}
	
	
	RE::TES::GetSingleton()->ForEachReferenceInRange(player, globals::fLODFadeOutMultObjects, [](RE::TESObjectREFR* ref) {

		if (!ref) return RE::BSContainer::ForEachResult::kContinue;

		const auto baseObject = ref->GetBaseObject();

		auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

		if (baseFormID == 0) return RE::BSContainer::ForEachResult::kContinue;

		for (const auto& formID : globals::baseFormsWithAttachedLights) {

			//logger::debug("Tried to match base form id: {} against: {}", baseFormID, formID);

			if (baseFormID == formID) {
				logger::debug("baseForm ref that needs reinitializing found");

			//	RE::ObjectRefHandle handle(ref);

					//if (auto ref = handle.get()) {

						auto root = ref->Get3D(); 

						if (!root) return RE::BSContainer::ForEachResult::kContinue;

						auto bsFadeNode = root->AsNode();

						if (!bsFadeNode) return RE::BSContainer::ForEachResult::kContinue;

						//surf children for light
						for (auto& child : bsFadeNode->GetChildren()) {
							if (!child) continue;

							// exclude non relight lights
							auto name = std::string_view(child->name.c_str());
							if (name.size() < 2 || name[0] != 'R' || name[1] != 'L')
								continue;

							RE::NiPointLight* light = netimmerse_cast<RE::NiPointLight*>(child.get());

							if (!light) continue;

							auto it = LightData::configIDToJsonCfg.find(light->GetLightRuntimeData().unk138);

							if (it == LightData::configIDToJsonCfg.end()) {
								logger::warn("attempted to reinitialize light {} but its config ID {} wasent found for ref {:08X}", name, ref->GetFormID(), light->GetLightRuntimeData().unk138);
								continue;
							}

							const auto& config = it->second;

								auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
								if (!ssNode) {
									logger::warn("ShadowSceneNode[0] is null!");
									continue;
								}
								bool bsLightExists = false;

								for (RE::NiPointer<RE::BSShadowLight> bsLight : ssNode->activeShadowLights) {

									if (!bsLight)
										continue;

									if (bsLight->light.get() == light) {
										logger::debug("shadow light {} with ID {} exists already for ref {:08X} skipping reinitialization", light->name, static_cast<void*>(light), ref->GetFormID());
										bsLightExists = true;
										break;
									}
								}

								if (!bsLightExists) {
									logger::debug("reintializing light {} for ref {:08X} is shadow = {}", light->name, ref->GetFormID(), config.shadowLight);

									//reset light data incase user had changed them since then
									//LightData::setNiPointLightDataFromCfg(light, config);
									auto p = LightData::makeLightParams(config);
									auto reattachedBSLight = ssNode->AddLight(light, p);

									if (!reattachedBSLight) return RE::BSContainer::ForEachResult::kContinue;

									uint32_t mask = config.flags;

									if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kCandle)) {
										reattachedBSLight->unk060 = 1;
										//return RE::BSContainer::ForEachResult::kContinue;
									}

									if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kChandelier)) {
										reattachedBSLight->unk060 = 2;
										//return RE::BSContainer::ForEachResult::kContinue;
									}

									if (mask & static_cast<uint32_t>(LIGHT_FLAGS::kFire)) {
										reattachedBSLight->unk060 = 3;
									}
								}

						}
			}
		}
	});
	
		std::scoped_lock refsLock(globals::refsWithAttachedLightsMutex);
		{ globals::refsWithAttachedLights.clear(); }
	
		std::scoped_lock mergedLock(globals::mergedRefsMutex);
		{ globals::mergedRefs.clear(); }
}

//used to merge a light with same ref base object within a set distance to help prevent flickering. 
//we have to push this to be finalized later because ray casting (used to stop meriging through walls) isent ready until later
void LightManager::fillPendingMerges(RE::TESObjectREFR* refA,
	RE::NiPointLight* light, const LightConfig& cfg, RE::NiNode* refA_root) {

	if (!refA || !light) return;

	// attach light to mesh
	LightManager::attachLightUsingAttachPath(cfg, refA_root, light, refA->GetFormID());

	PendingMerge p;

	p.winningConfig = cfg; 

	p.light = RE::NiPointer<RE::NiPointLight>(light); 

	p.refA = refA->GetHandle();

	p.refARoot = refA_root;

	int potentialMergeCount = 0;

	 p.refALightName = cfg.nodeName.empty() ? extractMeshName(cfg.meshPath) : cfg.nodeName;

	bool increasedMergeDistance = false;

	RE::TES::GetSingleton()->ForEachReferenceInRange(refA, globals::lightMergeSeekingDistance, [&](RE::TESObjectREFR* otherRef) {
		if (!otherRef || otherRef == refA) return RE::BSContainer::ForEachResult::kContinue;
		if (potentialMergeCount >= globals::lightMergeMaxLights) return RE::BSContainer::ForEachResult::kStop;

		const RE::FormID refBFormID = otherRef->GetFormID();
		{
			std::lock_guard lock(globals::mergedRefsMutex);
			if (globals::mergedRefs.count(refBFormID) > 0) return RE::BSContainer::ForEachResult::kContinue;
		}
		{
			std::lock_guard lock(globals::refsWithAttachedLightsMutex);
			if (globals::refsWithAttachedLights.count(refBFormID) > 0) return RE::BSContainer::ForEachResult::kContinue;
		}

		auto base = otherRef->GetBaseObject();
		auto model = base ? base->As<RE::TESModel>() : nullptr;
		if (!model) return RE::BSContainer::ForEachResult::kContinue;

		std::string otherRefName = extractMeshName(model->GetModel());

		//priority grab bc we do partial searches which can bring up false positive matches
		std::string otherRefNameMatch = std::string(findPriorityMatch(otherRefName));

		if (!otherRefNameMatch.empty()) {

			//mutable
			toLower(otherRefNameMatch);

			 if (isExclude(otherRefName, otherRef)) return RE::BSContainer::ForEachResult::kContinue;

			bool looseMatch = false;

			auto cell = otherRef->GetParentCell();
			if (!cell) {
				logger::warn("no cell cant determine if should use exterior or interior configs");
				return RE::BSContainer::ForEachResult::kContinue;
			}

			bool isInterior = cell->IsInteriorCell();
			auto cfgs = findConfigsForNode(otherRefNameMatch, isInterior);

			if (cfgs.empty()) return RE::BSContainer::ForEachResult::kContinue;

			LightConfig otherRefCfg = cfgs[0]; 

			uint32_t refAflags = cfg.flags;

			uint32_t otherRefFlags = otherRefCfg.flags;

			if (refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kCandle)) {

				p.refALightName = "candle";

				if (!increasedMergeDistance && (refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kIncreasedMergeDistance) || otherRefFlags & static_cast<uint32_t>(LIGHT_FLAGS::kIncreasedMergeDistance))) {
					increasedMergeDistance = true;
					logger::debug("increased distance used");
				}
			}
				looseMatch = otherRefNameMatch.find(p.refALightName) != std::string::npos ||
					p.refALightName.find(otherRefNameMatch) != std::string::npos;

				// giant campfires should merge with fires
				if ((refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kGiantCampfire) &&
					otherRefFlags & static_cast<uint32_t>(LIGHT_FLAGS::kFire)) ||

					(refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kFire) &&
						otherRefFlags & static_cast<uint32_t>(LIGHT_FLAGS::kGiantCampfire)))
				{
					looseMatch = true;
				}
			
			//logger::debug("comparing refA {:08X} {} and refB {:08X} {}  for merge == {} distance={}",
			//	refA->GetFormID(), refALightName, refBFormID, otherRefNameMatch, looseMatch, distance);
			
			if (looseMatch) {

				//the final result of a merged light should  reflect a shadow light if 1 of the emrgies was a shadow light
				if (!cfg.shadowLight && cfgs[0].shadowLight) {
					p.winningConfig = cfgs[0];
				}

				float zDistanceToUse = increasedMergeDistance ? globals::fMaxZDiffToMergeIncreased :  globals::fMaxZDiffToMerge;

				auto distanceToUse = p.winningConfig.shadowLight ? globals::shadowLightMergeDistance : globals::lightMergeDistance;

				auto posA = refA->GetPosition();
				auto posB = otherRef->GetPosition();

				float dx = posA.x - posB.x;
				float dy = posA.y - posB.y;
				float dz = posA.z - posB.z;

				float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

				// no increased merge distance flag among mergies
				if (!increasedMergeDistance) {

					if (distance > distanceToUse) {

						return RE::BSContainer::ForEachResult::kContinue;
					}
				}
				
				float zDiff = std::abs(refA->GetPosition().z - otherRef->GetPosition().z);
				if (zDiff > zDistanceToUse) {
					logger::debug("refA {:08X} and refB {:08X} z distance {} too great, skipping merge for light {} ",
						refA->GetFormID(), refBFormID, zDiff, p.refALightName, otherRefName);
					return RE::BSContainer::ForEachResult::kContinue;
				}

				p.candidateHandles.push_back(otherRef->GetHandle());
				logger::debug("refA {:08X} and refB {:08X} with matched nodeName {} selected to merge. distance apart = {}",
					refA->GetFormID(), refBFormID, p.refALightName, distance);
				potentialMergeCount++;
			}
		}
		return RE::BSContainer::ForEachResult::kContinue;
		});

	// no merges so just attach now
	if (p.candidateHandles.empty()) {

		logger::debug("ref {:08X} has no merge candidates, attaching light", refA->GetFormID());
		LightData::setNiPointLightDataFromCfg(p.light.get(), p.winningConfig);

		p.light->name = "RL" + p.refALightName;
		LightManager::attachNiPointLightToShadowSceneNode(p.light.get(), p.winningConfig, refA);

		if (globals::enableDebugLightBulbs) AttachDebugMarker(refA_root, p.light.get()); 

		return;
	}


	for (const RE::ObjectRefHandle handle : p.candidateHandles) {
		auto ref = handle.get();
		if (ref) {
			std::lock_guard lock(globals::mergedRefsMutex);
			globals::mergedRefs.insert(ref.get()->GetFormID());
		}
	}

	// register for finilazation in update hook otherwise we cant ray cast to stop lights from merging through walls its too early in loading stage
	p.registeredAt = std::chrono::steady_clock::now();

	{
		std::lock_guard lock(LightManager::pendingMergesMutex);
		LightManager::pendingMerges.push_back(p);
	}
}

 void LightManager::finalizeMerge(PendingMerge& p, const std::vector<RE::ObjectRefHandle>& validMerges) {

	 auto light = p.light.get(); 

	 if (!light) return; 

	 LightData::setNiPointLightDataFromCfg(p.light.get(), p.winningConfig);

	 p.light->name = "RL" + p.refALightName;

	 auto refA = p.refA.get();
	 if (!refA) return;


	 if (globals::enableDebugLightBulbs) {
		 AttachDebugMarker(p.refARoot, light);
	 }

	 // no merges so just attach as is
	 if (validMerges.empty()) {
		 LightManager::attachNiPointLightToShadowSceneNode(p.light.get(), p.winningConfig, refA.get());
		 return;
	 }

		 // build positions array: refA first, then all merged refs
		 std::vector<RE::NiPoint3> positions;

		 positions.push_back(refA->GetPosition());
		 for (const RE::ObjectRefHandle refHandle : validMerges) {

			 auto otherRef = refHandle.get(); 

			 if (!otherRef) continue; 
			 positions.push_back(otherRef->GetPosition());
		 }

		 const int mergeCount = static_cast<int>(positions.size());  // includes refA

		 // increasing brightness or radius on meshes close to each other like fxfirewithemberslgos x fxfirewithembers light 
		 // makes it difficult to configure those cases for users so we dont increase if distance is small
		 bool increaseBrightness = false;

		 for (size_t i = 1; i < positions.size(); i++) {
			 if (positions[0].GetDistance(positions[i]) > 25.0f) {
				 increaseBrightness = true;
				 break;
			 }
		 }

		 //merge count 3 because pending merges includes ref a and we dont want to increase brightness for merges of only 2 light sources
		 if (increaseBrightness && mergeCount >= 3) {
			 float fadeMultiplier = 1.0f + globals::lightFadePerMerge * static_cast<float>(mergeCount - 2);
			 float radiusMultiplier = 1.0f + globals::lightRadiusPerMerge * static_cast<float>(mergeCount - 2);

			 // clamp to max
			 fadeMultiplier = std::min(fadeMultiplier, globals::lightFadeMax);
			 radiusMultiplier = std::min(radiusMultiplier, globals::lightRadiusMax);

			 p.light->fade *= fadeMultiplier;
			 p.light->radius *= radiusMultiplier;
		 }

		 RE::NiPoint3 worldMid{};
		 float sumX = 0, sumY = 0;
		 for (const auto& pos : positions) { sumX += pos.x; sumY += pos.y; }
		 worldMid.x = sumX / positions.size();
		 worldMid.y = sumY / positions.size();
		 worldMid.z = positions[0].z;

		 // iterate toward  median
		 for (int iter = 0; iter < 20; iter++) {
			 float numX = 0, numY = 0, denom = 0;
			 for (const auto& pos : positions) {
				 float dx = worldMid.x - pos.x;
				 float dy = worldMid.y - pos.y;
				 float dist = std::sqrt(dx * dx + dy * dy);
				 if (dist < 0.001f) continue; // skip if basically at this point
				 float w = 1.0f / dist;
				 numX += pos.x * w;
				 numY += pos.y * w;
				 denom += w;
			 }
			 if (denom < 0.001f) break;
			 worldMid.x = numX / denom;
			 worldMid.y = numY / denom;
		 }

		float originalLocalZ =p.light->local.translate.z;

		 auto* parent = p.light->parent; 

		 if (!parent) {
			 logger::warn("light parent was null cant finish merging light"); 
			 return; 
		 }

		 RE::NiTransform invTransform = parent->world.Invert();
		 RE::NiPoint3 localMid = invTransform * worldMid;
		 localMid.z += originalLocalZ;

		 auto refAFormID = refA->GetFormID();
		 std::string mergedRefsHex;

		 for (const auto& refHandle : validMerges) {
			 auto otherRef = refHandle.get();
			 if (!otherRef) continue;
			 char buf[16];
			 sprintf_s(buf, "%08X", otherRef->GetFormID());
			 if (!mergedRefsHex.empty()) mergedRefsHex += ", ";
			 mergedRefsHex += buf;
		 }

		 logger::debug(
			 "[LightMerge:BEFORE] '{}' merging {} refs\n"
			 "  refA {:08X} merged with refs [{}]\n"
			 "  originalLocalZ {}\n"
			 "  local.translate {}",
			 p.light->name.c_str(), mergeCount,
			 refAFormID,
			 mergedRefsHex,
			 originalLocalZ,
			 p.light->local.translate
		 );

		 p.light->local.translate = localMid;


			 RE::NiUpdateData updateData{};
			 updateData.time = 0.0f;
			 updateData.flags = RE::NiUpdateData::Flag::kDirty;
			 parent->UpdateTransformAndBounds(updateData);
		 

			 logger::debug(
				 "[LightMerge:AFTER] '{}' merged {} refs\n"
				 "  refA {:08X} merged with refs [{}]\n"
				 "  local.translate {}",
				 p.light->name.c_str(), mergeCount,
				 refAFormID,
				 mergedRefsHex,
				 p.light->local.translate
			 );

	 LightManager::attachNiPointLightToShadowSceneNode(p.light.get(), p.winningConfig, refA.get());
}

void LightManager::AttachDebugMarker(RE::NiNode* a_node, RE::NiLight* light)
{
	if (!a_node || !light) {
		logger::debug("AttachDebugMarker: null a_node or light");
		return;
	}
	if (!a_node->parent) {
		logger::debug("AttachDebugMarker: a_node {} has no parent, skipping", a_node->name.c_str());
		return;
	}
	RE::NiPointer<RE::NiNode> loadedModel;
	constexpr RE::BSModelDB::DBTraits::ArgsType args{};
	if (const auto error = Demand("marker_light.nif", loadedModel, args); error == RE::BSResource::ErrorCode::kNone) {
		if (const auto clonedModel = loadedModel->Clone()) {
			loadedModel.reset();
			auto clonedModelAsNode = clonedModel->AsNode();
			if (!clonedModelAsNode) {
				logger::debug("AttachDebugMarker: could not cast cloned model as node, skipping");
				return;
			}
			a_node->AttachChild(clonedModelAsNode);
			clonedModelAsNode->local.translate = light->local.translate;
			logger::debug("AttachDebugMarker: attached marker to node {} at translate ({}, {}, {})",
				a_node->name.c_str(), light->local.translate.x, light->local.translate.y, light->local.translate.z);
		}
		else {
			logger::debug("AttachDebugMarker: clone failed for debug light marker");
		}
	}
	else {
		logger::debug("AttachDebugMarker: failed to load marker_light.nif");
	}
}

RE::NiPointLight* LightManager::cloneNiPointLight(RE::NiPointLight* niPointLight) {

	if (!niPointLight) {
		logger::warn("no ni point light to clone!");
		return nullptr;
	}

	auto cloneAsNiAv = niPointLight->Clone();
	if (!cloneAsNiAv) {
		logger::error("Failed to clone NiNode");
		return nullptr;
	}

	auto niPointLightClone = netimmerse_cast<RE::NiPointLight*>(cloneAsNiAv);

	return niPointLightClone;
}

void LightManager::ComputeClosestLights(RE::BSLight* outLights[7], RE::BSLightingShaderProperty* p)
{
	auto* pass = p->renderPassList.head;
	if (!pass || !pass->geometry)
		return;
	auto& center = pass->geometry->worldBound.center;

	const float triRadius = pass->geometry->worldBound.radius;

	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode)
		return;
	auto& rt = ssNode->GetRuntimeData();
	struct Candidate
	{
		RE::BSLight* light;
		float dist;
	};
	std::vector<Candidate> candidates;
	auto gatherLights = [&](auto& container)
		{
			for (auto& l : container)
			{
				if (!l) continue;
				auto* light = l.get();
				if (!light || !light->light) continue;

				auto& pos = light->light->world.translate;
				float dx = pos.x - center.x;
				float dy = pos.y - center.y;
				float dz = pos.z - center.z;
				float distXY2;

				if (light->unk060 == 2) {

					dz *= 0.33; 
					distXY2 = dx * dx + dy * dy + dz * dz;
				}
				else {
					distXY2 = dx * dx + dy * dy + dz * dz;
				}


				if (triRadius < 700) {
					switch (light->unk060)
					{
					case 1:

						if (dz > 200.0f)
							continue;
					{
						if (distXY2 > globals::minCandleCoverage * globals::minCandleCoverage) continue;
						break;
					}
					case 2:
					{
						if (std::abs(dz) > 350.0f)
							continue;

						if (distXY2 > globals::minChandelierCoverage * globals::minChandelierCoverage) continue;
						break;
					}
					case 3:
					{
						if (distXY2 > globals::minFireCoverage * globals::minFireCoverage) continue;
						break;
					}
					default:

						break;
					}
				}
				candidates.push_back({ light, distXY2 });
			}
     };

	gatherLights(rt.activeLights);
	gatherLights(rt.activeShadowLights);

	std::sort(candidates.begin(), candidates.end(),
		[](const auto& a, const auto& b)
		{
			return a.dist < b.dist;
		});

	int maxCandles = (triRadius < 350.0f) ? 4 : 6;

	int outIndex = 0;
	int candleCount = 0;

	for (int i = 0; i < static_cast<int>(candidates.size()) && outIndex < 7; i++)
	{
		auto* light = candidates[i].light;

		if (light->unk060 == 1) {
			if (candleCount < maxCandles) {
				outLights[outIndex++] = light;
				candleCount++;
				continue;
			}

			int farthestCandleIndex = -1;
			float farthestCandleDist = -1.0f;

			for (int j = 0; j < outIndex; j++) {
				if (outLights[j] && outLights[j]->unk060 == 1 && outLights[j]->light) {
					auto& selectedPos = outLights[j]->light->world.translate;
					float sdx = selectedPos.x - center.x;
					float sdy = selectedPos.y - center.y;
					float selectedDist = sdx * sdx + sdy * sdy;

					if (selectedDist > farthestCandleDist) {
						farthestCandleDist = selectedDist;
						farthestCandleIndex = j;
					}
				}
			}

			if (farthestCandleIndex != -1 && candidates[i].dist < farthestCandleDist) {
				outLights[farthestCandleIndex] = light;
			}

			continue;
		}

		outLights[outIndex++] = light;
	}

	for (int i = outIndex; i < 7; i++)
		outLights[i] = nullptr;
}
