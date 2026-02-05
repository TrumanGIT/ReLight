
#include "plugin.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "hooks.h"
#include "Functions.h"
#include "menu.h"
#include "global.h"
#include "LightData.h"



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
      //  isPlayerInInteriorCell();

        break;
    }
    case SKSE::MessagingInterface::kNewGame:
    {
      //  isPlayerInInteriorCell();
        break;
    }
    case SKSE::MessagingInterface::kDataLoaded:
    {
        // create master point light. must clone it or crash idk why
        PointLight::getMasterPointLight();
        LightData::registerEventSink();
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
   hasInverseSquareLighting();
    return true;
}
