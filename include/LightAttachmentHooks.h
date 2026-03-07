#pragma once


// correct timing to attach lights because world position data is loaded, earlier = lights show up at cell origin 0,0,0
struct Load3D {

    static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x6A };

    static void Install();
};


struct AddonNodes
{
    static void thunk(
        RE::NiAVObject* a_clonedNode,
        RE::NiAVObject* a_node,
        std::int32_t a_slot,
        RE::TESObjectREFR* a_actor,
        RE::BSTSmartPointer<RE::BipedAnim>& a_bipedAnim);

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install();
};


