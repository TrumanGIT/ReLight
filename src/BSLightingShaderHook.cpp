#include "BSLightingShaderHook.h"

#include "everyframe.h"
#include "utility.h"
#include <unordered_set>

bool BSLightingShaderProperty_IsLightAffectingSurface::thunk(
    RE::BSLightingShaderProperty* p,
    RE::BSLight* light)
{
    if (!p || !light) return false;

    if (!light->affectLand)
    {
        const auto& flags = p->flags;
        if (flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kMultiTextureLandscape) ||
            flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kNoLODLandBlend))
        {
            return false;
        }
    }

    //exit in exteriors
    if (!globals::currentCellIsInterior || !globals::enableLightFlickerPreventionMeasures) return true;

    auto pass = p->renderPassList.head;
    if (!pass || !pass->geometry) return false;


    auto geometry = pass->geometry;
    const auto& triCenter = geometry->worldBound.center;
    const float triRadius = geometry->worldBound.radius;
  

    const auto& lightPos = light->light->world.translate;


    const float dist = triCenter.GetDistance(lightPos);

    //  distance-only reject

   /* float lightRadius = std::max({
        light->light->radius.x,
        light->light->radius.y,
        light->light->radius.z
        });*/

        // chandeliers
     if (light->unk060 == 1) {

        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);

        // Ignore Z almost entirely for candles
        const float distXY = std::sqrt(dx * dx + dy * dy);


        if (distXY > globals::gMinChandelierCoverage)
            return false;

        const auto& thisLightPos = light->light->world.translate;
        float thisDistance = triCenter.GetDistance(thisLightPos);
        float thisRadius = light->light->radius.Length();
        float thisCoverage = thisRadius - (thisDistance - triRadius);

        auto thisConfigID = light->light->unk138;
        auto* ss = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

        for (auto& otherLight : ss->GetRuntimeData().activeShadowLights) {
            if (!otherLight || !otherLight->light)
                continue;
            if (otherLight->light == light->light)
                continue;
            if (otherLight->light->unk138 != thisConfigID)
                continue;

            float zDiff = std::abs(thisLightPos.z - otherLight->light->world.translate.z);

            if (zDiff > globals::g_maxShadowCompeteDistance)
                continue;

            float otherDistance = triCenter.GetDistance(otherLight->light->world.translate);
            float otherCoverage = otherLight->light->radius.Length() - (otherDistance - triRadius);

            logger::debug(
                "light {:p} dist {:.2f} vs other {:p} dist {:.2f}",
                static_cast<void*>(light),
                thisDistance,
                static_cast<void*>(otherLight.get()),
                otherDistance
            );

            if (otherCoverage > thisCoverage) {
                return false;
            }

        }

    }

     //candles
    else if (light->unk060 == 2)
    {
        auto isWallMesh = false;

        RE::TESBoundObject* baseRef = nullptr;

        if (p->fadeNode && p->fadeNode->userData)
            baseRef = p->fadeNode->userData->data.objectReference;

        if (baseRef) {
            auto curBaseFormID = baseRef->GetFormID();
            isWallMesh = globals::wallMeshes.contains(curBaseFormID);
        }
   
        const float dx = std::abs(lightPos.x - triCenter.x);
        const float dy = std::abs(lightPos.y - triCenter.y);

        const float distXY = std::sqrt(dx * dx + dy * dy);

        float coverageThreshold = globals::gMinCandleCoverage;

        if (isWallMesh && triRadius < globals::maxWallSizeForStrictLightBounds) {
          
            coverageThreshold = globals::minCandleCoverageWall;
        }


        if (distXY > coverageThreshold)
            return false;
    }

  
     //fires
    else if (light->unk060 == 3) {
     
         auto isWallMesh = false;

         RE::TESBoundObject* baseRef = nullptr;

         if (p->fadeNode && p->fadeNode->userData)
             baseRef = p->fadeNode->userData->data.objectReference;

         if (baseRef) {
             auto curBaseFormID = baseRef->GetFormID();
             isWallMesh = globals::wallMeshes.contains(curBaseFormID);
         }

         const float dx = std::abs(lightPos.x - triCenter.x);
         const float dy = std::abs(lightPos.y - triCenter.y);

         const float distXY = std::sqrt(dx * dx + dy * dy);

         float coverageThreshold = globals::gMinFireCoverage;

         if (isWallMesh && triRadius < globals::maxWallSizeForStrictLightBounds) {

             coverageThreshold = globals::gMinFireCoverageWall;
         }


         if (distXY > coverageThreshold)
             return false;
     }

    else {
        if (dist > globals::globalCoverage)
            return false;
    
    }
    return true;
}

void BSLightingShaderProperty_IsLightAffectingSurface::Install()
{
    auto& trampoline = SKSE::GetTrampoline();

    REL::Relocation<std::uintptr_t> target{
        RELOCATION_ID(98902, 105550)
    };

    func = trampoline.write_branch<5>(
        target.address(),
        thunk
    );

    logger::info("BSLightingShaderProperty_IsLightAffectingSurface hook installed");
}

// not used 
__int64 BSShaderPropertyLightData_AttachLight::thunk(RE::BSShaderPropertyLightData* a_this, RE::BSLight* light)
{
    if (!a_this || !light) return reinterpret_cast<decltype(&thunk)>(func)(a_this, light);


    // if (light->unk060 == 1) return -1

    // logger::info("attach light called on tri with total lights: {} with name {} ", a_this->lights.size(), light->light->name.c_str());
    return reinterpret_cast<decltype(&thunk)>(func)(a_this, light);
}

void BSShaderPropertyLightData_AttachLight::Install() {
    REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(100990, 107777) };

    auto& trampoline = SKSE::GetTrampoline();
    hook_function_prologue<BSShaderPropertyLightData_AttachLight, 5>(target.address());

}

// not used 
__int64 BSLight_AddFadeNode::thunk(RE::BSLight* light, RE::NiAVObject* root)
{
    if (!root) return reinterpret_cast<decltype(&thunk)>(func)(light, root);

    auto geometry = root->AsGeometry();
    if (!geometry) return reinterpret_cast<decltype(&thunk)>(func)(light, root);

    auto shader = geometry->lightingShaderProp_cast();
    if (!shader) return reinterpret_cast<decltype(&thunk)>(func)(light, root);

    auto* head = shader->renderPassList.head;
    if (!head) return reinterpret_cast<decltype(&thunk)>(func)(light, root);

    if (!shader->lightData) return reinterpret_cast<decltype(&thunk)>(func)(light, root);

    auto totalShadowLights = head->numShadowLights;
    auto totalNonShadowLights = shader->lightData->lights.size();
    auto totalLights = totalShadowLights + totalNonShadowLights;

    if (totalLights >= 7) return -1;

    return reinterpret_cast<decltype(&thunk)>(func)(light, root);
}

//101296	131cde0
//108283  141509760
void BSLight_AddFadeNode::Install() {
    REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(101296, 108283) };
    hook_function_prologue<BSLight_AddFadeNode, 5>(target.address());
}

