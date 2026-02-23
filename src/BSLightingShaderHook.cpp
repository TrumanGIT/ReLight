#include "BSLightingShaderHook.h"
#include "global.h"
#include "everyframe.h"
#include "utility.h"

// BS tri shapes are passed through this hook 
//its not a great solution because were fighting the engine, not solving the problem.
// this hook is also called ALOT, this might be overloading this hook
// only played with this it needs alot of work
void BSLightingShader_SetupGeometry::thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags)
{
   if (!globals::enableHookToRemoveLightsFromBSTriShapes) return func(This, Pass, RenderFlags);

   if (Pass && Pass->geometry) {
       auto geometry = Pass->geometry;
       auto lightingShader = geometry->lightingShaderProp_cast();
       if (lightingShader && lightingShader->lightData) {
           auto& lightList = lightingShader->lightData->lights;
           auto didSomething = false;

           if (lightList.size() > 7) {
                didSomething = true; 

               int beforeCount = lightList.size();
               logger::debug("BEFORE cleanup: {} lights", beforeCount);

               // get the position and the model radius for comparing
               auto& geomPos = geometry->world.translate;
               float geomRadius = geometry->modelBound.radius;

               // iterate through the light list on the geomatry
               for (int i = lightList.size() - 1; i >= 0; i--) {
                   auto light = lightList[i];
                   if (!light || !light->light.get()) continue;

                   auto nilight = light->light.get();

                   //get light radius to compare
                   float lightRadius = nilight->GetLightRuntimeData().radius.x;
                   float dist = nilight->world.translate.GetDistance(geomPos);

                   // compare here. 
                   if (dist > (lightRadius + geomRadius) * globals::lightOverlapMinOnTriShapeMult) {
                       lightList.erase(lightList.begin() + i);
                   }
               }

               // If still over 7, keep only closest 7
               if (lightList.size() > 7) {
                   logger::debug("Still {} lights after overlap check, limiting to 7 closest", lightList.size());

                   std::vector<std::pair<float, RE::BSLight*>> lightDistances;
                   for (auto light : lightList) {
                       if (!light || !light->light.get()) continue;
                       float dist = light->light->world.translate.GetDistance(geomPos);
                       lightDistances.push_back({ dist, light });
                   }

                   std::sort(lightDistances.begin(), lightDistances.end());

                   lightList.clear();
                   for (int i = 0; i < 7; i++) {
                       lightList.push_back(lightDistances[i].second);
                   }
               }

               if (didSomething)  logger::debug("AFTER cleanup: {} lights", lightList.size());
           }

           func(This, Pass, RenderFlags);

       }

   }


}
void BSLightingShader_SetupGeometry::Install() {

	func = REL::Relocation<std::uintptr_t>(RE::VTABLE_BSLightingShader[0]).write_vfunc(0x6, thunk);
}

__int64 BSShaderPropertyLightData_AttachLight::thunk(RE::BSShaderPropertyLightData* This, RE::BSLight* BSLight)
{
    if (!This || !BSLight) return reinterpret_cast<decltype(&thunk)>(func)(This, BSLight);

    if (This->lights.size() >= 7) {
    
        return -1; 




        /*    if (Pass && Pass->geometry) {
                auto geometry = Pass->geometry;
                auto lightingShader = geometry->lightingShaderProp_cast();
                if (lightingShader && lightingShader->lightData) {
                    auto& lightList = lightingShader->lightData->lights;

                    if (lightList.size() > 7) {
                        int beforeCount = lightList.size();
                        logger::info("BEFORE cleanup: {} lights", beforeCount);

                        auto worldSceneGraph = GetWorldSceneGraph();
                        //logger::debug("worldSceneGraph loaded");
                        auto worldCamera = ((RE::BSSceneGraph*)worldSceneGraph)->GetRuntimeData().camera.get();

                        //store a score of how inside the camera the light is
                        std::vector<std::pair<float, RE::BSLight*>> visibleLights;

                        // get the position and the model radius for comparing
                        auto& geomPos = geometry->world.translate;
                        float geomRadius = geometry->modelBound.radius;

                          // iterate through the light list on the bs tri shape
                        for (int i = lightList.size() - 1; i >= 0; i--) {
                            auto light = lightList[i];
                            if (!light || !light->light.get()) continue;

                            auto nilight = light->light.get();

                            bool visible = true;

                            float lightRadius = nilight->GetLightRuntimeData().radius.x;
                            float coord[4] = {
                       nilight->world.translate.x,
                       nilight->world.translate.y,
                       nilight->world.translate.z,
                       lightRadius
                            };

                            float minOut[3]{}, maxOut[3]{};

                            NiCamera_unk_CalculateFrustumOverlap(worldCamera, coord, minOut, maxOut, globals::frustumOverlapTolerance);

                            float frustumScore = 0;

                            for (int v = 0; v < 3; v++) {
                                if (maxOut[v] <= minOut[v]) {
                                    visible = false;
                                    lightList.erase(lightList.begin() + i);


                                }
                                logger::info("  Removed light: minOut[{}]={}, maxOut[{}]={}",
                                    v, minOut[v], v, maxOut[v]);
                                break;
                                float range = maxOut[v] - minOut[v];
                                frustumScore += range;
                            }

                            if (visible) {
                                visibleLights.emplace_back(frustumScore, light);
                            }

                        }

                        // If still over 7, keep the 7 furtherst inside the frustrum (camera view)
                        if (lightList.size() > 7) {
                            logger::info("Still {} lights after overlap check, limiting to 7 closest", lightList.size());

                            std::sort(visibleLights.begin(), visibleLights.end());

                            lightList.clear();
                            for (int i = 0; i < 7; i++) {
                                lightList.push_back(visibleLights[i].second);
                            }
                        }

                        logger::info("AFTER cleanup: {} lights", lightList.size());
                    }

                    func(This, Pass, RenderFlags);

                    // checking to see if the light list changed before and after (it doesent here but does elsewhere)
                    if (Pass && Pass->geometry) {
                        auto lightingShader = Pass->geometry->lightingShaderProp_cast();
                        if (lightingShader && lightingShader->lightData) {
                            logger::info("AFTER func(): {} lights", lightingShader->lightData->lights.size());
                        }
                    }
                }

            }*/


    }


   // logger::info("attach light called {} : {}", This->lights.size(), BSLight->light->name.c_str() );
    return reinterpret_cast<decltype(&thunk)>(func)(This, BSLight); 
}


void BSShaderPropertyLightData_AttachLight::Install() {
    REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(100990, 107777) };

    auto& trampoline = SKSE::GetTrampoline();
    hook_function_prologue<BSShaderPropertyLightData_AttachLight, 5>(target.address());

}

