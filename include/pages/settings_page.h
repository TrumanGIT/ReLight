#pragma once

#include "../SKSEMenuFramework.h"
#include "../config.h"
#include "../LightData.h"


void __stdcall RenderSettings();

inline void debugLogAllLights() {
	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode) {
		logger::warn("ShadowSceneNode[0] is null!");
		return;
	}

	auto& rt = ssNode->GetRuntimeData();

	for (auto& light : rt.activeLights) {
		if (!light) continue;

		std::string lightName = light->light->name.c_str();

		const auto& lightRt = light->light->GetLightRuntimeData();
		auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
		if (it == LightData::configIDToJsonCfg.end()) {
			logger::warn("Config ID {:08X} not found in config map", lightRt.unk138);
			continue;
		}

		const auto& cfg = it->second;

		logger::debug(
			"[Light] '{}'\n"
			"  brightness        {}\n"
			"  startingBrightness{}\n"
			"  radius            {}\n"
			"  flickerIntensity  {}\n"
			"  flickerRate       {}\n"
			"  flickerMovement       {}\n"
			"  worldPos          {}\n"
			"  unk060            {}\n"
			"  configID          {}",
			lightName,
			lightRt.fade,
			cfg.startingFade,
			lightRt.radius,
			cfg.flickerIntensity,
			cfg.flickersPerSecond,
			cfg.flickerAmplitude,
			light->light->world.translate,
			light->unk060,
			cfg.configID
		);
		float r = lightRt.diffuse.red;
		float g = lightRt.diffuse.green;
		float b = lightRt.diffuse.blue;

		logger::debug("  color (raw)       : R={:.3f}, G={:.3f}, B={:.3f}", r, g, b);
		logger::debug("  color (0-255)     : R={:.0f}, G={:.0f}, B={:.0f}", r * 255.0f, g * 255.0f, b * 255.0f);

		if (globals::islInstalled) {

			auto* islRt = Overlay::Get(light->light.get());

			if (!islRt)
				return;

			logger::debug("Light size {} light cuttoff {}", islRt->size, islRt->cutoffOverride);
			logger::debug("Flags: 0x{:08X}", islRt->flags);

		}
	}

	for (auto& light : rt.activeShadowLights) {
		if (!light) continue;

		std::string lightName = light->light->name.c_str();

		if (lightName[0] != 'R' || lightName[1] != 'L')
			continue;

		const auto& lightRt = light->light->GetLightRuntimeData();

		auto it = LightData::configIDToJsonCfg.find(lightRt.unk138);
		if (it == LightData::configIDToJsonCfg.end()) {
			logger::warn("Config ID {:08X} not found in config map", lightRt.unk138);
			continue;
		}

		const auto& cfg = it->second;

		logger::debug(
			"[Shadow Light] '{}'\n"
			"  brightness        {}\n"
			"  startingBrightness{}\n"
			"  radius            {}\n"
			"  flickerIntensity  {}\n"
			"  flickersPerSecond {}\n"
			"  worldPos          {}\n"
			"  unk060            {}\n"
			"  configID          {}",
			lightName,
			lightRt.fade,
			cfg.startingFade,
			lightRt.radius,
			cfg.flickerIntensity,
			cfg.flickersPerSecond,
			light->light->world.translate,
			light->unk060,
			cfg.configID
		);


		if (globals::islInstalled) {

			auto* islRt = Overlay::Get(light->light.get());

			if (!islRt)
				return;

			logger::debug("Shadow Light size {} light cuttoff {}", islRt->size, islRt->cutoffOverride);

		}
	}

	logger::debug("printing all refs in attached lights set");
	for (const auto formID : globals::refsWithAttachedLights) {


		logger::debug("ref {:08X}", formID);

	}
}

