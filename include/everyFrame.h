#pragma once

#include "logger.hpp"
#include "random.h"
#include "LightData.h"
#include "disableLights.h"

// hook into player update so we can update light flicker data every fram
struct PlayerCharacter_Update {

    static void thunk(RE::PlayerCharacter* player, float delta);

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install();
};


// used in flicker calcs
inline float getRandomFloat(const float& min, const float& max, uint32_t rngState)
{
	return min + (max - min) * Random::rand(rngState);
}

// generic type argument probly not needed both shadow light list and non shadow light list same array type proboblly
template <class T>
static void ApplyLightFlicker(T& lights, float delta)
{
	for (auto& light : lights) {
		if (!light)
			continue;

		const char* name = light->light->name.c_str();
		if (!name || name[0] != 'R' || name[1] != 'L')
			continue;

		// free float used as flicker timer
		auto& scale = light->light->local.scale;
		auto& rt = light->light->GetLightRuntimeData();

		auto it = LightData::configIDToJsonCfg.find(rt.unk138);
		if (it == LightData::configIDToJsonCfg.end())
			continue;

		const auto& dataExt = it->second;

		uint32_t seed =
			static_cast<uint32_t>(
				reinterpret_cast<std::uintptr_t>(light->light.get()) & 0xFFFFFFFF);

		const float r = getRandomFloat(-0.1f, 0.1f, seed);

		scale += delta * (1.0f - r) * std::numbers::pi_v<float>;
		rt.fade =
			dataExt.startingFade +
			std::sin(scale * dataExt.flickersPerSecond) * dataExt.flickerIntensity;
	}
}

// used to get the world camera. 
inline RE::SceneGraph* GetWorldSceneGraph()
{
	// 143258B48
	static REL::VariantID uid(528087, 415032, 0);
	return *((RE::SceneGraph**)uid.address());
}