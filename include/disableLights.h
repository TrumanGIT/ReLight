#pragma once

#include "global.h"
#include <unordered_set>
#include "LightData.h"

//PO3's hook used to disable all lights tot start with a clean base
struct TESObjectLIGH_GenDynamic {
    static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static RE::NiPointLight* magicLightThunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static inline REL::Relocation<decltype(thunk)> func;

    static inline REL::Relocation<decltype(magicLightThunk)> magicLightFunc;

    static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, std::string& edid, std::string& modName);
    static void Install();
};

// meh321s hook from intellightent
struct BSLightingShaderProperty_IsLightAffectingSurface
{
    static bool thunk(RE::BSLightingShaderProperty* p, RE::BSLight* light);
    static inline REL::Relocation<decltype(thunk)> func;
    static void Install();
};

// for plants like dragontongue or deathbell so when player picks it the light goes away. 
struct TreeActivateHook
{
	static void Install();

private:
    static bool Activate(
        RE::TESObjectTREE* a_this,
        RE::TESObjectREFR* a_targetRef,
        RE::TESObjectREFR* a_activatorRef,
        std::uint8_t a_arg3,
        RE::TESBoundObject* a_object,
        std::int32_t a_targetCount);

	static inline REL::Relocation<decltype(Activate)> func;
};