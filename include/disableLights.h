#pragma once

#include "global.h"
#include <unordered_set>
#include "LightData.h"

// meh321s hook from intellightent
struct BSLightingShaderProperty_IsLightAffectingSurface
{
    static bool thunk(RE::BSLightingShaderProperty* p, RE::BSLight* light);
    static inline REL::Relocation<decltype(thunk)> func;
    static void Install();
};


// Used to shut off enchantment lights
class WeaponSheatheEventHandler : public RE::BSTEventSink<SKSE::ActionEvent>
{
public:
    static WeaponSheatheEventHandler* GetSingleton()
    {
        static WeaponSheatheEventHandler singleton;
        return std::addressof(singleton);
    }


    static void Install();

    RE::BSEventNotifyControl ProcessEvent(
        const SKSE::ActionEvent* a_event,
        RE::BSTEventSource<SKSE::ActionEvent>* a_eventSource) override;
};;

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

 bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, std::string& edid, std::string& modName, bool disableDynamicForms = false);