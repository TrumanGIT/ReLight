
#include "plugin.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "hooks.h"
#include "Functions.h"
#include "menu.h"

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
       
         masterNiPointLight = LightData::createNiPointLight();

        if (!masterNiPointLight) {
            logger::critical("masterNiPointLight was not created succesfully, backing out of mod intialization");
            return; 
        }

        LightData::assignNiPointLightsToBank(masterNiPointLight);
        Hooks::Install();
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
    return true;
}
