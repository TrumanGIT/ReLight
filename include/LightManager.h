#pragma once

#include "logger.hpp"
#include "LightData.h"


struct LightManager : RE::BSTEventSink<RE::BGSActorCellEvent> {

    struct PendingMerge {

        RE::ObjectRefHandle refA;
        RE::NiPointer<RE::NiPointLight> light;
        LightConfig winningConfig;
        std::string refALightName; 
        std::vector<RE::ObjectRefHandle> candidateHandles;
        std::chrono::steady_clock::time_point registeredAt;

    };

    static inline std::vector<PendingMerge> pendingMerges;
    static inline std::mutex pendingMergesMutex;

    /*/struct OurEventSink : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
        OurEventSink() = default;
        OurEventSink(const OurEventSink&&) = delete;
        OurEventSink& operator=(const OurEventSink&) = delete;
        OurEventSink& operator=(OurEventSink&&) = delete;


    public:
        static OurEventSink* GetSingleton() {
            static OurEventSink singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* event,
            RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) {



            return RE::BSEventNotifyControl::kContinue;
        }

        static void TESCellFullyLoadedEventInstall() {
            auto* eventSink = OurEventSink::GetSingleton();

            auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            eventSourceHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(eventSink);
        }
    };*/

    	static LightManager* GetSingleton()
    {
        static LightManager singleton;
        return &singleton;
    }

    static void attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::TESObjectREFR* a_this);

    static void registerEventSink();

    static void attachLightUsingAttachPath(const LightConfig& cfg, RE::NiNode* root, RE::NiPointLight* light, RE::FormID refFormID);

   static bool processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root);

   static void processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this);

   static bool dummyHandler(RE::TESObjectREFR* a_this, const RE::BSFixedString& nodeName, RE::NiNode* a_root);

  static void reinitializeLightsWithinRange(RE::PlayerCharacter* player); 

  static void fillPendingMerges(RE::TESObjectREFR* a_this,
      RE::NiPointLight* childLight, const LightConfig& cfg, RE::NiNode* a_root);

  static void finalizeMerge(PendingMerge& p, std::vector<RE::ObjectRefHandle> validMerges);

  static void AttachDebugMarker(RE::NiNode* a_node, RE::NiLight* light);




private:
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

};



