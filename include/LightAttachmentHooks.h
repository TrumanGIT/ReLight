#pragma once

#include "LightManager.h"
#include "Utility.h"
#include "global.h"

//attach light to spells explosions effects and the likes, no light merging and exact mesh paths required
namespace ObjectReference
{
    template <class T>
    struct Load3D
    {
        static RE::NiAVObject* thunk(T* a_this, bool a_backgroundLoading);

        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr std::size_t idx{ 0x6A };

        static void Install();
    };

     void InstallLoad3DHooks();
}

// hook ShaderReferenceEffect::Init vfunc to attach lights to reference effects
namespace ReferenceEffect
{
inline TESBoundObject* GetReferenceEffectBase(const TESObjectREFRPtr& a_ref, const ReferenceEffect* a_referenceEffect)
	{
		if (const auto weapController = skyrim_cast<WeaponEnchantmentController*>(a_referenceEffect->controller)) {
			return weapController->lastWeapon;
		}

		if (auto modelEffect = a_referenceEffect->As<ModelReferenceEffect>()) {
			return modelEffect->artObject;
		}
		if (auto shaderReferenceEffect = a_referenceEffect->As<ShaderReferenceEffect>(); shaderReferenceEffect && shaderReferenceEffect->wornObject) {
			return shaderReferenceEffect->wornObject;
		}

		return a_ref->GetBaseObject();
	}

    NiAVObject* GetReferenceAttachRoot(ReferenceEffect* a_referenceEffect)
	{
		if (const auto weapController = skyrim_cast<WeaponEnchantmentController*>(a_referenceEffect->controller)) {
			if (!weapController->shader) {  // missing nullptr check in GetAttachRoot -> crash
				return nullptr;
			}
		}
		return a_referenceEffect->GetAttachRoot();
	}

    template <class T>
    struct Init
    {
        static bool thunk(T* a_this);

        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr std::size_t idx{ 0x36 };

        static void Install();
    };
}

//attach light to static objects, allows light merging and partial mesh path search
struct TESObjectREFRLoad3D {

    static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x6A };

    static void Install();
};


//PO3's hook used to disable and or edit vanilla / modded esp,esm,esl plugin lights
struct TESObjectLIGH_GenDynamic {
    static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static RE::NiPointLight* magicLightThunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
        bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

    static inline REL::Relocation<decltype(thunk)> func;

    static inline REL::Relocation<decltype(magicLightThunk)> magicLightFunc;

    static void Install();

    static void MagicLightThunkInstall();
};

// used for scripted fires like castle volkihar that only turn on when activated
struct Activate {

    static bool thunk(
        RE::TESObjectACTI* a_this,
        RE::TESObjectREFR* a_targetRef,
        RE::TESObjectREFR* a_activatorRef,
        std::uint8_t a_arg3,
        RE::TESBoundObject* a_object,
        std::int32_t a_targetCount);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x37 };

    static void Install();
};
