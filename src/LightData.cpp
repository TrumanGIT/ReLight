
#include <cstdint>
#include "global.h"
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "config.hpp"
#include "LightData.h"
#include <string>
#include <vector>
#include  "Functions.h"

// Members (config to json id is for faster lookups, used in flicker logic) 
std::map<uint64_t, LightConfig> LightData::configIDToJsonCfg;

std::unordered_map<std::string, LightConfig> LightData::meshPathToJsonCfg;
std::unordered_map<std::string, std::vector<LightConfig>> LightData:: nodeNameToJsonCfg;
// at runtime save a copy of each tempaltes settings so we can restore to defaults later
std::unordered_map<uint64_t, LightConfig> LightData::defaultConfigs;


bool LightData::shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref)
{
	if (!ref || !light || ref->IsDynamicForm()) {
		return false;
	}

	if (excludeLightEditorID(light)) return false;

	auto player = RE::PlayerCharacter::GetSingleton();

	if (IsInSoulCairnOrApocrypha(player)) {
		logger::debug("player is in apocrypha or soul cairn so we should not disable light");
		return false;
	}
	 
	return true;
}

// Try to exclude light by editorID.
bool LightData::excludeLightEditorID(const RE::TESObjectLIGH* light) {

	std::string edid = clib_util::editorID::get_editorID(light);

	if (!edid.empty()) {
		for (const auto& group : globals::keywordLightGroups) {
			if (containsAll(edid, group)) {
				logger::info("Excluding light by editorID: {}", edid);
				return true;
			}
		}
	}
	return false;
}

/* // were not using tesobject ligh flags anymore need to reimplement this.
template <class T>
REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t>
parseLightFlags(const T& obj)
{
	REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> flags;

	for (const auto& flagStr : obj.flags) {
		auto it = kLightFlagMap.find(flagStr);
		if (it != kLightFlagMap.end()) {
			flags.set(it->second);
		}
	}
	return flags;
}*/

RE::NiPoint3 LightData::getNiPointLightRadius(const LightConfig& cfg)
{
	return RE::NiPoint3(cfg.radius, cfg.radius, cfg.radius);
}

void LightData::setNiPointLightAmbientAndDiffuse(RE::NiLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::warn("nullptr passed to set ni point light ambient and diffuse");
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	// supposedly main color of light 
	data.diffuse.red = cfg.diffuseColor[0] / 255.0f;
	data.diffuse.green = cfg.diffuseColor[1] / 255.0f;
	data.diffuse.blue = cfg.diffuseColor[2] / 255.0f;

	// idk about ambient after a quick google search it seems ambient is usually a fraction of diffuse
	// we could prolly research futher and get better results but for now good enough
	data.ambient.red = data.diffuse.red * cfg.ambientRatio;
	data.ambient.green = data.diffuse.green * cfg.ambientRatio;
	data.ambient.blue = data.diffuse.blue * cfg.ambientRatio;
}

void LightData::setNiPointLightPos(RE::NiLight* niPointLight, const LightConfig& cfg)
{
	if (!niPointLight) {
		logger::warn("nullptr passed to set ni point light ambient and diffuse");
		return;
	}
	niPointLight->local.translate.x = cfg.position[0];
	niPointLight->local.translate.y = cfg.position[1];
	niPointLight->local.translate.z = cfg.position[2];
}


/*not used
void LightData::setRelightFlag(RE::TESObjectLIGH* ligh)
{
	if (!ligh) {
		logger::warn("nullptr passed to set ni point light ambient and diffuse");
		return;
	}

	auto rawPtr = reinterpret_cast<std::uint32_t*>(&ligh->data.flags);
	*rawPtr |= (1u << 15); // set 15th bit (an unused flag to identify our lights like ISL) 
}*/


void LightData::setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg) {


	if (!niPointLight) {
		logger::error("light nullptr for node {}", cfg.nodeName);
		return;
	}


	if (auto* overlay = Overlay::Get(niPointLight)) {

		overlay->size = cfg.size; // isl
		overlay->cutoffOverride = cfg.cutoffOverride; // isl 
		overlay->fade = cfg.fade;
		overlay->radius = cfg.radius;
		overlay->lighFormId = 0;
		overlay->unk138 = static_cast<std::uint32_t>(cfg.configID); 
	}
}

void LightData::attachLightUsingAttachPath(
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

void LightData::setNiPointLightDataFromCfg(RE::NiLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::error("light nullptr for node {}", cfg.nodeName);
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	logger::debug(" Setting Light Data for {} from Configs", cfg.nodeName);

	data.fade = cfg.fade;
	data.radius = getNiPointLightRadius(cfg);

	data.unk138 = cfg.configID;

	logger::debug(" radius set to: {} ", cfg.radius);
	logger::debug(" fade set to: {} ", cfg.fade);
	logger::debug("config ID set to {}", cfg.configID); 

	setNiPointLightPos(niPointLight, cfg);

	logger::debug(" position set to: x:{} y:{} z:{} ", cfg.position[0], cfg.position[1], cfg.position[2]);

	setNiPointLightAmbientAndDiffuse(niPointLight, cfg);

	logger::debug(" diffuse color set to: r:{} g:{} b:{} ", cfg.diffuseColor[0], cfg.diffuseColor[1], cfg.diffuseColor[2]);

	if (globals::islInstalled) setOverlayData(niPointLight, cfg);

}
RE::ShadowSceneNode::LIGHT_CREATE_PARAMS LightData::makeLightParams(const LightConfig& cfg)
{
	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS p{};

	// Couldn't do it with a macro as not all config. variables can be used with LIGHT_CREATE_PARAMS.

	//Truman -  sounds good homie idk how to use that shit anyway xD

	p.dynamic = true;    // dynamic = game updates it every frame so yes
	p.shadowLight = cfg.shadowLight;   
	p.portalStrict = cfg.portalStrict; // idk 
	p.affectLand = cfg.affectLand; 
	p.affectWater = cfg.affectWater; 
	p.neverFades = cfg.neverFades; 

	p.fov = cfg.fov;   // idk
	p.falloff = cfg.falloff;    // idk 
	p.nearDistance = cfg.nearDistance; // idk
	p.depthBias = cfg.depthBias; // idk 

	p.sceneGraphIndex = 0;      // always use 0 

	p.restrictedNode = nullptr; //idk
	p.lensFlareData = nullptr; //idk 

	return p;
}

void LightData::attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg) {

	//logger::info("attempting to create NiPointLight BSlight and attach to ShadowSceneNode");

	if (!niPointLight) {
		logger::error("createShadowSceneNode: niPointLight is null");
		return;
	}

	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params = makeLightParams(cfg);

	logger::debug("Light paramaters for {}", niPointLight->name);

	printLightParams(params);

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

bool LightData::foundConfigForLight(const RE::NiLight* light) {
	
	for  (auto& [name, vectorOfConfigs] : LightData::nodeNameToJsonCfg) {
		for (auto& cfg : vectorOfConfigs) {
			if (light->unk138 == cfg.configID) {
				return true;
			}
		}
	}
	return false;
}

void LightData::updateConfigFromLight(LightConfig& cfg, RE::NiLight* niLight) {
	auto& rt = niLight->GetLightRuntimeData();
	auto& dataExt = LightData::configIDToJsonCfg[rt.unk138];

	cfg = dataExt; 

	cfg.radius = truncateDecimals(rt.radius.x, 2);
	cfg.fade = truncateDecimals(rt.fade, 2);

	cfg.position[0] = truncateDecimals(niLight->local.translate.x, 2);
	cfg.position[1] = truncateDecimals(niLight->local.translate.y, 2);
	cfg.position[2] = truncateDecimals(niLight->local.translate.z, 2);

	cfg.diffuseColor[0] = int(rt.diffuse.red * 255.0f);
	cfg.diffuseColor[1] = int(rt.diffuse.green * 255.0f);
	cfg.diffuseColor[2] = int(rt.diffuse.blue * 255.0f);

	cfg.flickerIntensity = truncateDecimals(dataExt.flickerIntensity, 2);
	cfg.flickersPerSecond = truncateDecimals(dataExt.flickersPerSecond, 2);

	if (globals::islInstalled) {

		if (auto* overlay = Overlay::Get(niLight)) {
			cfg.size = truncateDecimals(overlay->size, 2);
			cfg.cutoffOverride = truncateDecimals(overlay->cutoffOverride, 2);
		}
	}
	cfg.print();
}

RE::BSEventNotifyControl LightData::ProcessEvent(const RE::BGSActorCellEvent* event,
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

		/*for (auto& light : rt.activeShadowLights) {
			if (!light) {
				continue;
			}

			ssNode->RemoveLight(light);
		}*/

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

										if (bsLight->light.get() == light) {
											logger::info("shadow light {} with ID {} exists already for ref: {} skipping reinitialization", light->name, static_cast<void*>(light), ref->GetFormID());
											bsLightExists = true;
											break; 
										}
									}

									if (!bsLightExists) {
										logger::info("reintiializing shadow light {} with ID {} for ref {} ", light->name, static_cast<void*>(light), ref->GetFormID());

										auto p = LightData::makeLightParams(config);
										ssNode->AddLight(light, p);
									}

								}
								else {
									ref->Disable();
									ref->Enable(false);
									logger::info("non shadow light: {} with ID {} reinitialized for ref {}", light->name, static_cast<void*>(light), ref->GetFormID());
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

void LightData::registerEventSink()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AsBGSActorCellEventSource()->AddEventSink(LightData::GetSingleton());
		logger::info("BGSActorCellEvent sink registered");
	}
}