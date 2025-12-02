#pragma once
#include "SKSEMenuFramework.h"

namespace UI {

    void Register();
    void __stdcall Render();
    void __stdcall RenderWindow();
    void saveSettingsToIni(); 
    inline MENU_WINDOW reLightMenuWindow;

};