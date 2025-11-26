#include "Hooks.h"
#include "Functions.h"
#include "global.h"
#include <map>
#include <array>
#include <string>
#include <unordered_set>


namespace Hooks {

    //Po3's
    RE::NiPointLight* TESObjectLIGH_GenDynamic::thunk(
        RE::TESObjectLIGH* light,
        RE::TESObjectREFR* ref,
        RE::NiNode* node,
        bool forceDynamic,
        bool useLightRadius,
        bool affectRequesterOnly)
    {
        
        if (!ref || !light)
            return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

        if (excludeLightEditorID(light))
            return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

        RE::FormID refFormID = ref->GetFormID();
        if (!refFormID)
            return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);

        // get the name of the mod owning the light
        const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
        std::string modName = refOriginFile ? refOriginFile->fileName : "";

		toLower(modName);

        if (should_disable_light(light, ref, modName))
            return nullptr;

         // if whitelisted, change rgb values to whatever we want    
        //doint want to change color of lights that change color based on time of day. 
        if (modName.find("window shadows ultimate") == std::string::npos)
        {
            light->data.color.red = red;
            light->data.color.green = green;
            light->data.color.blue = blue;

            //  logger::info("Changed color for light {:X} from {}", formID, modName);
        }

        return func(light, ref, node, forceDynamic, useLightRadius, affectRequesterOnly);
    }

    void TESObjectLIGH_GenDynamic::Install() {
        std::array targets{
            std::make_pair(RELOCATION_ID(17206, 17603), 0x1D3),  // TESObjectLIGH::Clone3D
            std::make_pair(RELOCATION_ID(19252, 19678), 0xB8),   // TESObjectREFR::AddLight
        };

        for (const auto& [address, offset] : targets) {
            REL::Relocation<std::uintptr_t> target{ address, offset };
            write_thunk_call<TESObjectLIGH_GenDynamic>(target.address());
        }

        logger::info("Installed TESObjectLIGH::GenDynamic patch");
    }

    //Po3's
    void PostCreate::thunk(
        RE::TESModelDB::TESProcessor* a_this,
        const RE::BSModelDB::DBTraits::ArgsType& a_args,
        const char* a_nifPath,
        RE::NiPointer<RE::NiNode>& a_root,
        std::uint32_t& a_typeOut)
    {
        auto ui = RE::UI::GetSingleton();

        if (!dataHasLoaded || !a_root) {
            return func(a_this, a_args, a_nifPath, a_root, a_typeOut);
        }

        if (ui && ui->IsMenuOpen("InventoryMenu")) {
            //logger::info("Inventory menu is open, skipping PostCreate processing"); // do we even need that? 
            return func(a_this, a_args, a_nifPath, a_root, a_typeOut);
        }

        std::string nodeName = a_root->name.c_str();  // grab name of NiNode (usually 1:1 with mesh names)
        toLower(nodeName);

		//TO DO:: Rprobobly remove this function all together (no more specfic meshes / partial but just partial)
        if (cloneAndAttachNodesForSpecificMeshes(nodeName, a_root, a_nifPath))
            return func(a_this, a_args, a_nifPath, a_root, a_typeOut);

        auto match = matchedKeyword(nodeName);

        if (!match.empty() || nodeName.find("nortmphallbgc") != std::string::npos || nodeName.find("norcathallsm") != std::string::npos || nodeName.find("scene") != std::string::npos) {

            if (isExclude(nodeName, a_nifPath, a_root.get())) return func(a_this, a_args, a_nifPath, a_root, a_typeOut);

            if (handleSceneRoot(a_nifPath, a_root, nodeName)) //TO DO:: Replace with ni point light instead of ni node
                return func(a_this, a_args, a_nifPath, a_root, a_typeOut);

            if (removeFakeGlowOrbs) 
                glowOrbRemover(a_root.get());

            //TO DO:: Replace with ni point light instead of ni node
            if (TorchHandler(nodeName, a_root)) 
               return func(a_this, a_args, a_nifPath, a_root, a_typeOut);
            //TO DO:: Replace with ni point light instead of ni node
            if (applyCorrectNordicHallTemplate(nodeName, a_root)) 
                return func(a_this, a_args, a_nifPath, a_root, a_typeOut);
            //TO DO:: Replace with ni point light instead of ni node
            RE::NiPointer<RE::NiNode> nodePtr = getNextNodeFromBank(match); 
            if (nodePtr) { // scene is apart of the nodebank but we do not want to attach nodes for scene. 
                a_root->AttachChild(nodePtr.get());
                return func(a_this, a_args, a_nifPath, a_root, a_typeOut);
                //    logger::info("attached light to keyword mesh {}", nodeName);
            }
        
        }

        dummyHandler(a_root.get(), nodeName); //TO DO:: Replace with ni point light instead of ni node

        // Always call original func if nothing handled
        return func(a_this, a_args, a_nifPath, a_root, a_typeOut);

    }

    void PostCreate::Install() {
        // Get TESProcessor's vtable
        REL::Relocation<std::uintptr_t> vtable(RE::TESModelDB::TESProcessor::VTABLE[0]);

        // Replace the vfunc at index 'size' with our thunk
        func = vtable.write_vfunc(size, thunk);

        logger::info("Installed TESModelDB::TESProcessor hook");
    }

    void Install() {
        SKSE::AllocTrampoline(1 << 8);
        TESObjectLIGH_GenDynamic::Install();
        //  ReplaceTextureOnObjectsHook::Install();
        PostCreate::Install();
    }

}
