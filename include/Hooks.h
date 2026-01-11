#pragma once

namespace Hooks {

    //Doodlums hook found in RE Discord
    
    
          struct PlayerCharacter_Update {

              static void thunk(RE::PlayerCharacter* player, float delta);

              static inline REL::Relocation<decltype(thunk)> func;

              static void Install();
            };



    //PO3
    struct TESObjectLIGH_GenDynamic {
        static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
            bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

        static inline REL::Relocation<decltype(thunk)> func;
        static void Install();
    };

    
    struct Load3D {

        static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading);

        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr std::size_t idx{ 0x6A };

        static void Install();
    };

 /*  //PO3
    struct UpdateActivateParents
    {
        static void thunk(RE::TESObjectCELL* a_cell);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();

    };
    */

    void Install(); 

}

