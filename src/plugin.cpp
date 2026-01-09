
#include "plugin.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "hooks.h"
#include "Functions.h"
#include "menu.h"

//MUST RESET LIGHTS ON CELL CHANGE (im not sure why yet
struct OurEventSink : public RE::BSTEventSink<RE::TESCellReadyToApplyDecalsEvent> {
    OurEventSink() = default;
    OurEventSink(const OurEventSink&&) = delete;
    OurEventSink& operator=(const OurEventSink&) = delete;
    OurEventSink& operator=(OurEventSink&&) = delete;


public:
    static OurEventSink* GetSingleton() {
        static OurEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellReadyToApplyDecalsEvent* event,
        RE::BSTEventSource<RE::TESCellReadyToApplyDecalsEvent>*) {

        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        logger::info("event sink fired");

        auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
        if (!ssNode) {
            logger::warn("ShadowSceneNode[0] is null!");
            return RE::BSEventNotifyControl::kContinue;
        }

        auto& rt = ssNode->GetRuntimeData();


        for (auto& light : rt.activeLights) {
            if (!light) continue;
            auto lightName = light->light->name;

            auto& currentRt = light->light->GetLightRuntimeData();

            auto* currentIslRt = ISL_Overlay::Get(light->light.get());

            if (!currentIslRt) {
                logger::warn("no selected ISL runtime data in skse menu");
                return RE::BSEventNotifyControl::kContinue;
            }

            currentIslRt->initialized = false;

            logger::debug("light :{}  fade:{}  starting fade:{}, radius: {}, flickerIntensity: {}, FlickerPerSecond{} ", lightName, currentRt.fade, currentIslRt->startingFade, currentRt.radius, currentIslRt->flickerIntensity, currentIslRt->flickersPerSecond);

           
        }
        return RE::BSEventNotifyControl::kContinue;
    }
   
};



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
        initialize(); 
       // LightData::assignNiPointLightsToBank(masterNiPointLight);
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

    auto* eventSink = OurEventSink::GetSingleton();

    auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    eventSourceHolder->AddEventSink<RE::TESCellReadyToApplyDecalsEvent>(eventSink);

    return true;
}
