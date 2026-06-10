#pragma once

#include "LightManager.h"
#include "Utility.h"
#include "global.h"

//when we attach ni poiny lights to static objects
struct Load3D {

    static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x6A };

    static void Install();
};

// used for torches weapons / armors
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

// used for scripted fires like castle volkihar that only turn on when activated
struct Activate {

    static bool thunk(
        RE::TESObjectACTI* a_this,
        RE::TESObjectREFR* a_targetRef,
        RE::TESObjectREFR* a_activatorRef,
        std::uint8_t a_arg3,
        RE::TESBoundObject* a_object,
        std::int32_t a_targetCount);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x37 };

    static void Install();
};

// for spells like candlelight
namespace ReferenceEffect
{
    template <class T>
    struct Init
    {
        static bool thunk(T* a_this)
        {
            auto result = func(a_this);

            if (result) {
                if constexpr (std::is_same_v<T, RE::ModelReferenceEffect>) {
                    //logger::info("ModelReferenceEffect::Init fired a_this={}", static_cast<void*>(a_this));

                    // must delay or no artobject3d
                    SKSE::GetTaskInterface()->AddTask([a_this]() {
                        if (!a_this) {
                            logger::debug("Deferred ModelReferenceEffect task: a_this was null");
                            return;
                        }

                        RE::NiAVObject* node = a_this->artObject3D.get();
                        if (!node) {
                            logger::debug("Deferred ModelReferenceEffect task: artObject3D still null");
                            return;
                        }

                        globals::magicLightAttachNode = FindObjectByNameRecursive(node, "AttachLight");
                        if (!globals::magicLightAttachNode) {
                            logger::debug("Deferred task: AttachLight not found in ModelReferenceEffect tree");
                            return;
                        }

                        // must delay quite a while so we deal with this in the everyframe.h player update hook
                        globals::magicLightQueued.store(true); 

                        logger::debug(
                            "Deferred task: AttachLight found name={} ptr={}",
                            globals::magicLightAttachNode->name.c_str(),
                            static_cast<void*>(globals::magicLightAttachNode));
                        });
                }
            }

            return result;
        }

        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{ 0x36 };

        static void Install()
        {
            func = REL::Relocation<std::uintptr_t>(T::VTABLE[0])
                .write_vfunc(idx, thunk);

            logger::info("Hooked {}::Init", typeid(T).name());
        }
    };
}