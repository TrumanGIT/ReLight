#include "LightManager.h"
#include "Utility.h"
#include "config.hpp"


// ATTACH LIGHTS DURING LOAD3D() HOOK, ANY EARLIER AND LIGHTS SPAWN AT CELL ORIGIN BC WORLD POSITION DATA ISENT LAODED?

RE::NiAVObject* Load3D::thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
{
	if (!a_this || a_backgroundLoading == false) {
		logger::warn("Load3D called with null a_this or bg loading = true (light were trying to reinitialize) skipping light attachment");
		return func(a_this, a_backgroundLoading);
	}

	//logger::info("load3D called");
	auto niAVObject = func(a_this, a_backgroundLoading);
	if (!niAVObject) {
		logger::warn("no ni node casted from niav object from load3d hook");
		return niAVObject;
	}

	//helps filter out a few things we dont want to touch (fog, mist)
	auto a_root = niAVObject->AsNode();
	if (!a_root) {
		logger::warn("no ni node casted from niav object in load3d");
		return niAVObject;
	}

	if (LightManager::processByFilePath(a_this, a_root)) return niAVObject;

	// grab name of NiNode (usually 1:1 with mesh names)

	// some nodes have 2 config names in their nodename. for example we need to prioritize candlechangdelier01 to use chandelier lights over candle lights.
	const RE::BSFixedString nodeNameMatch = findPriorityMatch(a_root->name);

	if (!nodeNameMatch.empty()) {
		if (isExclude(a_root->name, a_root)) return niAVObject;

		LightManager::processByNodeName(a_root, nodeNameMatch, a_this);
		return  niAVObject;
	}

	if (LightManager::dummyHandler(a_this, a_root->name, a_root)) {
		return niAVObject;
	}

	return niAVObject;
}

void Load3D::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::TESObjectREFR::VTABLE[0])
		.write_vfunc(idx, thunk);
	logger::info("Hooked TESObjectREFR::Load3D");
}


//ATTACH LIGHTS AT CORRECT MESH INDEX, USEFULL FOR TORCHES WHERE LIGHT MUST BE INSERTED TO SPECIFIC SPOT
void LightManager::attachLightUsingAttachPath(
	const LightConfig& cfg,
	RE::NiNode* root,
	RE::NiPointLight* light)
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

	logger::debug("attached light to node {}", finalNodeName);

	finalNode->AttachChild(light);
}

// ATTACH NI POINT LIGHT TO SHADOW SCENE NODE TO GET BS LIGHT IN RETURN. BS LIGHT IS THE LIGHT YOU VISUALLY SEE RENDERED IN GAME
//BS LIGHT READS ITS NI POINT LIGHT DATA EVERY FRAME
void LightManager::attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg) {

	//logger::info("attempting to create NiPointLight BSlight and attach to ShadowSceneNode");

	if (!niPointLight) {
		logger::error("createShadowSceneNode: niPointLight is null");
		return;
	}

	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params = LightData::makeLightParams(cfg);

	logger::debug("Light paramaters for {}", niPointLight->name);

	LightData::printLightParams(params);

	auto* shadowSceneNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

	if (!shadowSceneNode) {
		logger::warn("no shadow scene node to grab in (createShadowSceneNode()");
		return;
	}
	RE::BSLight* BsLight = shadowSceneNode->AddLight(niPointLight, params);

	if (!BsLight) {
		logger::info("no BSLight created in (createShadowSceneNode() for {}", niPointLight->name);
		return;
	}
}

 bool LightManager::processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root) {

	if (LightData::meshPathToJsonCfg.empty()) return false;

	const auto baseObject = a_this->GetBaseObject();

	if (!baseObject) return true;

	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return true;

	auto currentModel = bm->GetModel();

	for (const auto& [meshPath, cfg] : LightData::meshPathToJsonCfg) {

		if (meshPath != currentModel) continue;

		const auto baseFormID = baseObject->GetFormID();

		globals::baseFormsWithAttachedLights.emplace(baseFormID);

		logger::debug("file path match found: {}", currentModel);

		auto ui = RE::UI::GetSingleton();

		if (ui && ui->IsMenuOpen("InventoryMenu")) {
			//logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
			return true;
		}

		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for mesh '{}')", currentModel);
			continue;
		}

		LightData::setNiPointLightDataFromCfg(cloneLight, cfg);

		/// TODO:: if not in priority list in ini file, this causes name to be RL only need to fix that
		std::string temp = "RL" + std::string(cfg.nodeName.c_str());
		cloneLight->name = temp.c_str();

		LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight);

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg);
		return true;

	}

	return false;
}

// some nodes are called dummy this is to take care of them.
 bool LightManager::dummyHandler(RE::TESObjectREFR* a_this, const RE::BSFixedString& nodeName, RE::NiNode* a_root)
{

	if (!nodeName.contains("dummy")) return false;

	logger::debug("dummy found");

	static const std::unordered_map<std::string, std::string> dummyMeshPaths = {
	{ "Clutter\\Ruins\\RuinsFloorCandleLampMidOn.nif", "ruinsfloorcandlelampmidon" },
	{ "Clutter\\Ruins\\RuinsFloorCandleLampMidOn02.nif", "ruinsfloorcandlelampmidon" },
	{ "Clutter\\Ruins\\RuinsFloorCandleLampSmOn.nif", "ruinsfloorcandlelampsmon" },
	{ "Clutter\\Ruins\\RuinsFloorCandleLampSmOn02.nif", "ruinsfloorcandlelampsmon" },

	{ "Clutter\\Imperial\\ImpChandellierCandle01.nif", "chandel" },
	{ "Clutter\\Imperial\\ImpChandellierCandle01USKP.nif", "chandel" },

	{ "Clutter\\Common\\CandleLanternwithCandle01.nif", "candle" },

	{ "DynDOLOD\\LOD\\Clutter\\CandleLanternHandleDown_DynDOLOD_LOD.nif", "candle" },
	{ "DynDOLOD\\LOD\\Clutter\\CandleLanternwithCandle01_DynDOLOD_LOD.nif", "candle" },

	{ "DynDOLOD\\LOD\\Clutter\\ImpChandellierCandle01_DynDOLOD_LOD.nif", "chandel" },

	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampMidOn_DynDOLOD_LOD.nif", "ruinsfloorcandlelampmidon" },
	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampMidOn02_DynDOLOD_LOD.nif", "ruinsfloorcandlelampmidon" },
	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampSmOn_DynDOLOD_LOD.nif", "ruinsfloorcandlelampsmon" },
	{ "DynDOLOD\\LOD\\Clutter\\RuinsFloorCandleLampSmOn02_DynDOLOD_LOD.nif", "ruinsfloorcandlelampsmon" },
	};

	auto baseObject = a_this->GetBaseObject();

	if (!baseObject) return true;

	const auto baseFormID = baseObject->GetFormID();

	const auto bm = baseObject->As<RE::TESModel>();
	if (!bm) return true;

	const auto currentModel = bm->GetModel();

	logger::debug("dummy found, Model = {}", currentModel);

	auto it = dummyMeshPaths.find(currentModel);
	if (it != dummyMeshPaths.end()) {

		globals::baseFormsWithAttachedLights.emplace(baseFormID);
		logger::debug("node: {} with baseFormID: {}  emplaced in set", nodeName.c_str(), baseFormID);

		const std::string& match = it->second;

		auto cfg = findConfigsForNode(match)[0];

		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}')", nodeName);
			return true;
		}

		LightData::setNiPointLightDataFromCfg(cloneLight, cfg);

		cloneLight->name = "RL" + cfg.nodeName;

		LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight);

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg);

		logger::debug("dummy match found for path {} and got light with node Name: {}", currentModel, cfg.nodeName);
		return true;
	}

	// already returned early if not a dummy, therefor might as well skip this object as it wouldent get light anyway
	return true;
}


 void LightManager::processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this) {

	// matched name
	std::string matchStr = match.c_str();

	auto ui = RE::UI::GetSingleton();

	if (ui && ui->IsMenuOpen("InventoryMenu")) {
		//logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
		return;
	}

	const auto baseObject = a_this->GetBaseObject();

	const auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

	if (baseFormID != 0) {
		globals::baseFormsWithAttachedLights.emplace(baseFormID);
		logger::debug("node: {} with baseFormID: {}  emplaced in set", matchStr, baseFormID);
	}

	if (globals::removeFakeGlowOrbs)
		glowOrbRemover(a_root);

	auto cfgs = findConfigsForNode(matchStr);

	for (const auto& cfg : cfgs) {
		auto cloneLight = cloneNiPointLight(PointLight::getMasterPointLight().node.get());

		if (!cloneLight) {
			logger::warn("Failed to clone NiPointLight for node '{}')", matchStr);
			continue;
		}

		LightData::setNiPointLightDataFromCfg(cloneLight, cfg);

		cloneLight->name = "RL" + cfg.nodeName;

		LightManager::attachLightUsingAttachPath(cfg, a_root, cloneLight);

		LightManager::attachNiPointLightToShadowSceneNode(cloneLight, cfg);

		//logger::info("attached {} light to {} ", cfg.nodeName, a_this->GetFormID());
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

	static float fLODFadeOutMultObjects;

	if (s_firstCellEvent) {
		s_firstCellEvent = false;
		globals::lastCellWasInterior = cell->IsInteriorCell();
		return RE::BSEventNotifyControl::kContinue;

		getObjectFadeMult(fLODFadeOutMultObjects);

		logger::debug("Users object fade ini setting = {}", fLODFadeOutMultObjects);

	}

	const bool currentCellIsInterior = cell->IsInteriorCell();

	if (globals::lastCellWasInterior != currentCellIsInterior) {
		logger::debug("player moved from exteiror to interior, or vice versa, reattaching lights");

		auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
		if (!ssNode) {
			logger::warn("ShadowSceneNode[0] is null!");
			return RE::BSEventNotifyControl::kContinue;
		}

		auto& rt = ssNode->GetRuntimeData();


		logger::debug("shaodow light list size after cleaning: {}", rt.activeLights.size());
		RE::TES::GetSingleton()->ForEachReferenceInRange(player, fLODFadeOutMultObjects, [](RE::TESObjectREFR* ref) {

			if (!ref) return RE::BSContainer::ForEachResult::kContinue;

			const auto baseObject = ref->GetBaseObject();

			auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

			if (baseFormID == 0) return RE::BSContainer::ForEachResult::kContinue;

			for (const auto& formID : globals::baseFormsWithAttachedLights) {

				//logger::debug("Tried to match base form id: {} against: {}", baseFormID, formID);

				if (baseFormID == formID) {
					logger::debug("baseForm ref that needs reinitializing found");

					RE::ObjectRefHandle handle(ref);
					SKSE::GetTaskInterface()->AddTask([handle]() {
						if (auto ref = handle.get()) {
							auto root = ref->Load3D(false);

							if (!root) return;

							auto bsFadeNode = root->AsNode();

							if (!bsFadeNode) return;

							//surf children for light
							for (auto& child : bsFadeNode->GetChildren()) {
								if (!child) continue;

								// exclude non relight lights
								const char* name = child->name.c_str();
								if (!name || name[0] != 'R' || name[1] != 'L')
									continue;

								RE::NiPointLight* light = netimmerse_cast<RE::NiPointLight*>(child.get());

								if (!light) continue;

								auto it = LightData::configIDToJsonCfg.find(light->GetLightRuntimeData().unk138);

								if (it == LightData::configIDToJsonCfg.end()) {
									logger::warn("attempted to reinitialize light but its config ID wasent found");
									continue;
								}

								const auto& config = it->second;

								// shadow lights are handled differently then non shadow lights
								if (config.shadowLight) {

									auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
									if (!ssNode) {
										logger::warn("ShadowSceneNode[0] is null!");
										continue;
									}
									bool bsLightExists = false;

									for (auto bsLight : ssNode->activeShadowLights) {

										bsLight->worldTranslate;

										if (bsLight->light.get() == light) {
											logger::info("shadow light {} with ID {} exists already for ref: {} skipping reinitialization", light->name, static_cast<void*>(light), ref->GetFormID());
											bsLightExists = true;
											break;
										}
									}

									if (!bsLightExists) {
										logger::info("reintiializing shadow light {} for ref {} ", light->name, ref->GetFormID());

										auto p = LightData::makeLightParams(config);
										ssNode->AddLight(light, p);
									}

								}
								else {
									ref->Disable();
									ref->Enable(false);
									logger::info("non shadow light: {} reinitialized for ref {}", light->name, ref->GetFormID());
								}
							}

						}
					});
				}
			}
		});
	}

	globals::lastCellWasInterior = currentCellIsInterior;

	return RE::BSEventNotifyControl::kContinue;
}

void LightManager::registerEventSink()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AsBGSActorCellEventSource()->AddEventSink(LightManager::GetSingleton());
		logger::info("BGSActorCellEvent sink registered");
	}
}



// TOODO:: put this in the event sink above
void LightManager::reinitializeLightsWithinRange(RE::PlayerCharacter* player) {

	logger::debug("reinitializing lights within range");
	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode) {
		logger::warn("ShadowSceneNode[0] is null cant reinitialize lights");
		return;
	}
	auto& rt = ssNode->GetRuntimeData();

	static float fLODFadeOutMultObjects;

	getObjectFadeMult(fLODFadeOutMultObjects);
	
	RE::TES::GetSingleton()->ForEachReferenceInRange(player, fLODFadeOutMultObjects, [](RE::TESObjectREFR* ref) {

		if (!ref) return RE::BSContainer::ForEachResult::kContinue;

		const auto baseObject = ref->GetBaseObject();

		auto baseFormID = baseObject ? baseObject->GetFormID() : 0;

		if (baseFormID == 0) return RE::BSContainer::ForEachResult::kContinue;

		for (const auto& formID : globals::baseFormsWithAttachedLights) {

			//logger::debug("Tried to match base form id: {} against: {}", baseFormID, formID);

			if (baseFormID == formID) {
				logger::debug("baseForm ref that needs reinitializing found");

				RE::ObjectRefHandle handle(ref);
				SKSE::GetTaskInterface()->AddTask([handle]() {
					if (auto ref = handle.get()) {
						auto root = ref->Load3D(false);

						if (!root) return;

						auto bsFadeNode = root->AsNode();

						if (!bsFadeNode) return;

						//surf children for light
						for (auto& child : bsFadeNode->GetChildren()) {
							if (!child) continue;

							// exclude non relight lights
							const char* name = child->name.c_str();
							if (!name || name[0] != 'R' || name[1] != 'L')
								continue;

							RE::NiPointLight* light = netimmerse_cast<RE::NiPointLight*>(child.get());

							if (!light) continue;

							auto it = LightData::configIDToJsonCfg.find(light->GetLightRuntimeData().unk138);

							if (it == LightData::configIDToJsonCfg.end()) {
								logger::warn("attempted to reinitialize light but its config ID wasent found");
								continue;
							}

							const auto& config = it->second;

							// shadow lights are handled differently then non shadow lights
							if (config.shadowLight) {

								auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
								if (!ssNode) {
									logger::warn("ShadowSceneNode[0] is null!");
									continue;
								}
								bool bsLightExists = false;

								for (auto bsLight : ssNode->activeShadowLights) {

									bsLight->worldTranslate;

									if (bsLight->light.get() == light) {
										logger::info("shadow light {} with ID {} exists already for ref: {} skipping reinitialization", light->name, static_cast<void*>(light), ref->GetFormID());
										bsLightExists = true;
										break;
									}
								}

								if (!bsLightExists) {
									logger::info("reintiializing shadow light {} for ref {} ", light->name, ref->GetFormID());

									auto p = LightData::makeLightParams(config);
									ssNode->AddLight(light, p);
								}

							}
							else {
								ref->Disable();
								ref->Enable(false);
								logger::info("non shadow light: {} reinitialized for ref {}", light->name, ref->GetFormID());
							}
						}

					}
				});
			}
		}
	});

}