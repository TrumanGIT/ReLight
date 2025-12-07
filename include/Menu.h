#pragma once
#include "SKSEMenuFramework.h"

namespace UI {

    void Register();
    void __stdcall RenderSettings();
    void __stdcall RenderLightEditor();
    void __stdcall RenderWindow();
    void saveSettingsToIni(); 
    void getAllLights(); 
    inline MENU_WINDOW reLightMenuWindow;

};
