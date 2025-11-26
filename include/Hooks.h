#pragma once

namespace Hooks {
    // Thiago99's
  /*  struct ReplaceTextureOnObjectsHook {
        static inline REL::Relocation<bool(RE::TESObjectREFR*)> originalFunction;
        static bool ShouldBackgroundClone(RE::TESObjectREFR* ref);

        static void Install();
    };*/

    //PO3's
    struct TESObjectLIGH_GenDynamic {
        static RE::NiPointLight* thunk(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, RE::NiNode* node,
                                       bool forceDynamic, bool useLightRadius, bool affectRequesterOnly);

        static inline REL::Relocation<decltype(thunk)> func;
        static void Install();
    };
   
    //PO3's
   struct PostCreate {
        static void thunk(RE::TESModelDB::TESProcessor* a_this, const RE::BSModelDB::DBTraits::ArgsType& a_args,
                          const char* a_nifPath, RE::NiPointer<RE::NiNode>& a_root, std::uint32_t& a_typeOut);

        static inline REL::Relocation<decltype(thunk)> func;  
        static inline std::size_t size{0x1};

        static void Install();
    };

void Install();  // no static or inline works because only calling and define it once so fine as is

}

