
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
        // create master point light. must clone it or crash idk why
        LightData::masterNiPointLight = NiPointLight::NiPointLight();
        SKSE::AllocTrampoline(1 << 8);
        TESObjectLIGH_GenDynamic::Install();
        Load3D::Install();
        PlayerCharacter_Update::Install();
        AddonNodes::Install();
        BSLightingShaderProperty_IsLightAffectingSurface::Install();
        // EVENT SINK IS USED TO REINITIALIZE LIGHTS CLEANED BY THE ENGINE 
        LightManager::registerEventSink();
//      LightManager::OurEventSink::TESCellFullyLoadedEventInstall(); 
        getObjectFadeMult();
        logger::debug("Users ini setting fLodFadeOutMultObjects setting = {}", globals::fLODFadeOutMultObjects);
     
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
    iniParser();
    parseTemplates();
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    UI::Register();
   hasInverseSquareLighting();
    return true;
}
