#include "pages/attachlights_page.h"
#include "pages/settings_page.h"
#include "pages/lightmerge_page.h"
#include "pages/flickerprevention_page.h"
#include "pages/lighteditor_page.h"
#include "menu.h"

namespace logger = SKSE::log;

namespace UI {


    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        SKSEMenuFramework::SetSection("ReLight");

        SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);

        SKSEMenuFramework::AddSectionItem("Light Editor", RenderLightEditor);

        SKSEMenuFramework::AddSectionItem("Attach Lights", RenderAttachRemove);

        SKSEMenuFramework::AddSectionItem("Light Merge", RenderLightMergeMenu);

        SKSEMenuFramework::AddSectionItem("Light Flicker Prevention", RenderLightFlickerPreventionMenu);

        SKSEMenuFramework::AddEvent(OnMenuEvent, 0);

        logger::info("registered Relights skse menu");

    }

    void OnMenuEvent(SKSEMenuFramework::Model::EventType eventType)
    {
        if (eventType == SKSEMenuFramework::Model::EventType::kCloseMenu) {
            logger::info("skse menu closed");

            // dont hold vanilla lights in ni pointer
            pluginLights.clear();

            //reset lines in playerupdate hook or they dont clear (race condition?) 
            globals::skseMenuClosed.store(true);
        }
    }
 
 }
