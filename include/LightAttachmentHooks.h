#pragma once

#include <xbyak/xbyak.h>
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


template <class T, std::size_t BYTES>
inline void hook_function_prologue(std::uintptr_t a_src)
{
    struct Patch : Xbyak::CodeGenerator
    {
        Patch(std::uintptr_t a_originalFuncAddr, std::size_t a_originalByteLength)
        {
            // Hook returns here. Execute the restored bytes and jump back to the original function.
            for (size_t i = 0; i < a_originalByteLength; ++i) {
                db(*reinterpret_cast<std::uint8_t*>(a_originalFuncAddr + i));
            }

            jmp(ptr[rip]);
            dq(a_originalFuncAddr + a_originalByteLength);
        }
    };

    Patch p(a_src, BYTES);
    p.ready();

    auto& trampoline = SKSE::GetTrampoline();
    trampoline.write_branch<5>(a_src, T::thunk);

    auto alloc = trampoline.allocate(p.getSize());
    std::memcpy(alloc, p.getCode(), p.getSize());

    T::func = reinterpret_cast<std::uintptr_t>(alloc);
}
