#pragma once
#include "SKSEMenuFramework.h"
#include "logger.hpp"

namespace UI {

    void Register();
    void OnMenuEvent(SKSEMenuFramework::Model::EventType eventType);


    inline MENU_WINDOW reLightMenuWindow;
  
}
