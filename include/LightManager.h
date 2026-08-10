#pragma once

#include "logger.hpp"
#include "LightData.h"


struct LightManager : RE::BSTEventSink<RE::BGSActorCellEvent> {

    struct PendingMerge {

        RE::ObjectRefHandle refA;
        RE::NiNode* refARoot;
        RE::NiPointer<RE::NiPointLight> light;
        LightConfig winningConfig;
        std::string refALightName; 
        std::vector<RE::ObjectRefHandle> candidateHandles;
        std::chrono::steady_clock::time_point registeredAt;

    };

    static inline std::vector<PendingMerge> pendingMerges;
    static inline std::mutex pendingMergesMutex;

    	static LightManager* GetSingleton()
    {
        static LightManager singleton;
        return &singleton;
    }

  static bool HandleScriptedFires(RE::TESObjectREFR* a_this, RE::FormID baseFormID, std::string& meshname, bool isInterior);

  static void HandleDLC1VCDungeonScriptedFires(RE::TESObjectREFR* a_targetRef);

  static void HandleSkyHavenTempleScriptedFires(RE::TESObjectREFR* a_targetRef);

  static bool HasRelightLight(RE::NiAVObject* a_root);

  static void attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::TESObjectREFR* a_this);

  static void registerEventSink();

  static void attachLightUsingAttachPath(const LightConfig& cfg, RE::NiNode* root, RE::NiPointLight* light, RE::FormID refFormID);

  static bool processByFilePath(RE::TESObjectREFR* a_this, std::string meshName, RE::NiNode* a_root, bool isInterior, bool skipExcludes = false);

  static void reinitializeLightsWithinRange(RE::PlayerCharacter* player); 

  static void fillPendingMerges(RE::TESObjectREFR* a_this,
      RE::NiPointLight* childLight, const LightConfig& cfg, RE::NiNode* a_root);

  static void finalizeMerge(PendingMerge& p, const std::vector<RE::ObjectRefHandle>& validMerges);

  static void AttachDebugMarker(RE::NiNode* a_node, RE::NiLight* light);

  static RE::NiPointLight* cloneNiPointLight(RE::NiPointLight* niPointLight);

  // used for computing closest 7 lights to a tri shape for light flicker prevention
  static void ComputeClosestLights(RE::BSLight* outLights[7], RE::BSLightingShaderProperty* p); 



  //used for updating position 
  static void UpdateLightParent(RE::NiLight* light); 

  static RE::NiLight* AttachLight(
      const LightConfig& cfg,
      RE::NiNode* a_root,
      RE::TESObjectREFR* a_this,
      const std::string& meshName,
      RE::FormID refFormID,
      bool& attachedDebugMarker);

  private:
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

};



