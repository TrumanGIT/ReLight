
#include "plugin.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "hooks.h"
#include "Functions.h"

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
         Initialize();
        dataHasLoaded = true; 
      //   assignClonedNodesToBank(); working on renewing this function
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
    ReadMasterListAndFillExcludes(); // need to change this func to read from ini file
    parseTemplates();
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    Hooks::Install(); 

    return true;
}
