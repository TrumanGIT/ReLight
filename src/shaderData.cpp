#include "shaderData.h"


void BSLightingShader_SetupGeometry::thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags)
{
    if (Pass && Pass->geometry) {
        auto geometry = Pass->geometry;
        auto lightingShader = geometry->lightingShaderProp_cast();
        if (lightingShader && lightingShader->lightData) {
            auto& lightList = lightingShader->lightData->lights;

            if (lightList.size() > 7) {
                int beforeCount = lightList.size();
                logger::info("BEFORE cleanup: {} lights", beforeCount);

                auto& geomPos = geometry->world.translate;
                float geomRadius = geometry->modelBound.radius;

                // First remove non-overlapping
                for (int i = lightList.size() - 1; i >= 0; i--) {
                    auto light = lightList[i];
                    if (!light || !light->light.get()) continue;

                    auto nilight = light->light.get();
                    float lightRadius = nilight->GetLightRuntimeData().radius.x;
                    float dist = nilight->world.translate.GetDistance(geomPos);

                    if (dist > (lightRadius + geomRadius)) {
                        lightList.erase(lightList.begin() + i);
                    }
                }

                // If still over 7, keep only closest 7
                if (lightList.size() > 7) {
                    logger::info("Still {} lights after overlap check, limiting to 7 closest", lightList.size());

                    std::vector<std::pair<float, RE::BSLight*>> lightDistances;
                    for (auto light : lightList) {
                        if (!light || !light->light.get()) continue;    `c`
                        float dist = light->light->world.translate.GetDistance(geomPos);
                        lightDistances.push_back({ dist, light });
                    }

                    std::sort(lightDistances.begin(), lightDistances.end());

                    lightList.clear();
                    for (int i = 0; i < 7; i++) {
                        lightList.push_back(lightDistances[i].second);
                    }
                }

                logger::info("AFTER cleanup: {} lights", lightList.size());
            }

            func(This, Pass, RenderFlags);

            // Check if it got rebuilt
            if (Pass && Pass->geometry) {
                auto lightingShader = Pass->geometry->lightingShaderProp_cast();
                if (lightingShader && lightingShader->lightData) {
                    logger::info("AFTER func(): {} lights", lightingShader->lightData->lights.size());
                }
            }
        }

    }
}
void BSLightingShader_SetupGeometry::Install() {

	func = REL::Relocation<std::uintptr_t>(RE::VTABLE_BSLightingShader[0]).write_vfunc(0x6, thunk);
}