
#include "plugin.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "Utility.h"
#include "menu.h"
#include "global.h"
#include "LightData.h"
#include "LightManager.h"
#include "everyFrame.h"
#include "disableLights.h"
#include "LightAttachmentHooks.h"
#include "vanillaMenus.h"
#include "ini.hpp"


static void MessageHandler(SKSE::MessagingInterface::Message* msg) {
    switch (msg->type) {
    case SKSE::MessagingInterface::kPostLoad:
    {
        break;
    }
    case SKSE::MessagingInterface::kSaveGame:
    {
        break;
    }
    case SKSE::MessagingInterface::kPreLoadGame:
    {
        break;
    }
    case SKSE::MessagingInterface::kPostLoadGame:
    {
    
        break;
    }
    case SKSE::MessagingInterface::kNewGame:
    {
   
        break;
    }
    case SKSE::MessagingInterface::kDataLoaded:
    {
        ini::IniParser();

        //cs installed dont need flicker prevention
        if (globals::islInstalled) globals::enableLightFlickerPreventionMeasures = false;

        if (globals::disableISL == true) globals::islInstalled = false; 

        //parse json files
        parseTemplates();

        // create master point light. must clone it or crash idk why
        LightData::masterNiPointLight = NiPointLight::NiPointLight();

        // EVENT SINK IS USED TO REINITIALIZE LIGHTS CLEANED BY THE ENGINE 
        LightManager::registerEventSink();

        DebugAPI_IMPL::DebugOverlayMenu::Register();
        break;
    }
    default:
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
   setupLog(spdlog::level::info);
   logger::info("Relight Plugin is Loaded");
   SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
   UI::Register();
   hasInverseSquareLighting();
   SKSE::AllocTrampoline(1 << 8);
   TESObjectLIGH_GenDynamic::Install();
   Load3D::Install();
   PlayerCharacter_Update::Install();
   AddonNodes::Install();
   BSLightingShaderProperty_IsLightAffectingSurface::Install();
   InventoryMenu::Install(); 
   CraftingMenu::Install();
   ReferenceEffect::Init<RE::ModelReferenceEffect>::Install();
   TreeActivateHook::Install(); 
   Activate::Install(); 
   NiLightFlickerHook::Install(); 
   return true;
}

