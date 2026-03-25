#include "utility.h"
#include "RE.h"

// not used 
__int64 BSShaderPropertyLightData_AttachLight::thunk(RE::BSShaderPropertyLightData* a_this, RE::BSLight* light)
{
    if (!a_this || !light) return reinterpret_cast<decltype(&thunk)>(func)(a_this, light);

    if (a_this->lights.size() >= 7) return -1; 

    // if (light->unk060 == 1) return -1

  // logger::info("attach light called on tri with total lights: {} with name {} ", a_this->lights.size(), light->light->name.c_str());
    return reinterpret_cast<decltype(&thunk)>(func)(a_this, light);
}

void BSShaderPropertyLightData_AttachLight::Install() {
    REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(100990, 107777) };

    //diauto& trampoline = SKSE::GetTrampoline();
//    hook_function_prologue<BSShaderPropertyLightData_AttachLight, 5>(target.address());

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
//    hook_function_prologue<BSLight_AddFadeNode, 5>(target.address());
}

