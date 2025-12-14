#pragma once
#include "SKSEMenuFramework.h"

namespace UI {

    void Register();
    void __stdcall RenderSettings();
    void __stdcall RenderLightEditor();
    void saveSettingsToIni(); 
    void getAllLights(); 
    void restoreLightToDefaults(RE::NiPointer<RE::NiLight> selectedLight); 
    inline MENU_WINDOW reLightMenuWindow;

};