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
    inline RE::TESBoundObject* GetReferenceEffectBase(
        const RE::TESObjectREFRPtr& a_ref,
        const RE::ReferenceEffect* a_referenceEffect)
    {
        if (const auto weapController =
            skyrim_cast<RE::WeaponEnchantmentController*>(a_referenceEffect->controller)) {
            return weapController->lastWeapon;
        }

        if (auto shaderReferenceEffect =
            a_referenceEffect->As<RE::ShaderReferenceEffect>();
            shaderReferenceEffect && shaderReferenceEffect->wornObject) {
            return shaderReferenceEffect->wornObject;
        }

        return a_ref->GetBaseObject();
    }

    inline RE::NiAVObject* GetReferenceAttachRoot(
        RE::ReferenceEffect* a_referenceEffect)
    {
        if (const auto weapController =
            skyrim_cast<RE::WeaponEnchantmentController*>(a_referenceEffect->controller)) {
            if (!weapController->shader) {
                return nullptr;
            }
        }

        return a_referenceEffect->GetAttachRoot();
    }

    struct Init
    {
        static bool thunk(RE::ShaderReferenceEffect* a_this);

        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr std::size_t idx{ 0x36 };

        static void Install();
    };

    void Install();
}

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
