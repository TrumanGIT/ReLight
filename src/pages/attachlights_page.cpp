#include "pages/attachlights_page.h"
#include "../lightdata.h"
#include "../utility.h"
#include "../LightManager.h"
#include "../forms.h"
#include "../ini.h"
#include "../lightData.h"

enum class AttachLightStep
{
    SelectTarget,
    AlreadyHasLight,
    ChooseTemplateType,
    ChooseTemplate,
    ChooseScope,
    Done,
    LightRemoved
};

void __stdcall RenderAttachRemove()
{

    auto centerNextItem = [&](float estimatedWidth) {
        float startX = ImGuiMCP::GetCursorPosX();

        ImGuiMCP::ImVec2 avail = ImGuiMCP::GetContentRegionAvail();

        ImGuiMCP::SetCursorPosX(startX + (avail.x - estimatedWidth) * 0.5f);
        };

    static AttachLightStep step = AttachLightStep::SelectTarget;

    //TODO:: put this in a struct or something
    static bool createNewTemplate = false;
    static bool multiLight = false;
    static bool refLight = false;
    bool attachedDebugMarker = false;

    static RE::FormID formID = 0x0;
    static RE::FormID baseFormID = 0x0;
    static std::string meshPath{};
    static std::string jsonFilePath{};
    static std::string menuCategory{};
    static std::string modName{};
    static std::string menuName{};
    static std::string matched{};
    static RE::NiLight* niLight = nullptr;
    static RE::FormID lastSelected = 0;
    static RE::FormID previewRef = 0;
    static int previewSelectedIndex = -1;
    static std::vector<std::tuple<std::variant<RE::FormID, std::string>, LightConfig, bool>> configDisplay;
    static std::unordered_set<std::string> seenMenuNames;
    static int selectedIndex = -1;
    static std::vector<LightConfig> selectedCfgs;
    static std::size_t entryCount = 0;

    static RE::TESObject* baseObject = nullptr;
    static RE::TESModel* model = nullptr;

    static char menuNameBuffer[128]{};
    static bool menuNameBufferInitialized = false;
    static char menuCategoryBuffer[128]{};
    static bool menuCategoryBufferInitialized = false;


    static LightConfig newCfg;

    auto resetState = [&]() {
        createNewTemplate = false;
        multiLight = false;
        refLight = false;
        attachedDebugMarker = false;

        meshPath.clear();
        jsonFilePath.clear();
        menuCategory.clear();
        menuName.clear();
        modName.clear();

        niLight = nullptr;
        baseObject = nullptr;


        previewRef = 0;
        previewSelectedIndex = -1;

        configDisplay.clear();
        seenMenuNames.clear();
        selectedIndex = -1;
        selectedCfgs.clear();
        matched.clear();

        entryCount = 0;
        formID = 0x0;
        baseFormID = 0x0;
        newCfg = LightConfig{};
        step = AttachLightStep::SelectTarget;

        menuNameBufferInitialized = false;
        menuNameBuffer[0] = '\0';
        menuCategoryBufferInitialized = false;
        menuCategoryBuffer[0] = '\0';
        };

    auto selected = RE::Console::GetSelectedRef().get();

    if (!selected) {
        resetState();
        ImGuiMCP::Dummy({ 0.0f, 50.0f });
        centerNextItem(350.0f);
        ImGuiMCP::Text("Click on an object in the console to continue.");
        return;
    }

    RE::TESFile* refOriginFile = selected->GetDescriptionOwnerFile();
    modName = refOriginFile ? refOriginFile->fileName : "";

    baseObject = selected->GetBaseObject();
    if (!baseObject) {
        return;
    }

    formID = selected->GetFormID();
    baseFormID = baseObject->GetFormID();

    model = baseObject->As<RE::TESModel>();
    if (!model) {
        return;
    }

    meshPath = extractMeshName(model->GetModel());
    toLower(meshPath);


    if (selected->GetFormID() != lastSelected) {

        resetState();

        lastSelected = selected->GetFormID();
        return;
    }


    switch (step)
    {
    case AttachLightStep::SelectTarget:
    {

        auto niAVObject = selected->Get3D();

        if (!niAVObject) return;

        auto niNode = niAVObject->AsNode();

        if (!niNode) return;

        step = LightManager::HasRelightLight(niNode) ?
            AttachLightStep::AlreadyHasLight :
            AttachLightStep::ChooseTemplateType;
        break;
    }

    case AttachLightStep::AlreadyHasLight:
    {
        ImGuiMCP::Dummy({ 0.0f, 50.0f });
        centerNextItem(470.0f);
        ImGuiMCP::Text("Object Selected in the console already has a ReLight light.");

        ImGuiMCP::Spacing();
        ImGuiMCP::Dummy({ 0.0f, 20.0f });
        centerNextItem(400.0f);

        if (RenderYellowButton("Add another light")) {

            multiLight = true;

            auto multiLightCfgs = LightData::findConfigsByFormID(formID, globals::currentCellIsInterior, false);

            if (multiLightCfgs && !multiLightCfgs->empty()) {

                logger::info("Add another light: found existing ref ID config {:08X}", selected->GetFormID());

                auto existingCfg = multiLightCfgs->front();

                entryCount = CountJsonEntriesInFile(existingCfg.configPath);
                jsonFilePath = existingCfg.configPath;
                menuCategory = existingCfg.menuCategory;
                menuName = StripTrailingIdentifier(existingCfg.menuName);

                auto root = selected->Get3D();

                if (!root) break;

                auto rootAsNode = root->AsNode();

                if (!rootAsNode) break;

                newCfg = existingCfg;
                newCfg.configID = globals::nextID++;
                newCfg.menuCategory = menuCategory;
                newCfg.menuName = std::format("{} [{}]", menuName, entryCount);
                logger::info("new menuName {}", newCfg.menuName);
                if (!newCfg.menuCategory.empty()) {
                    logger::info("menuCategory applied is ", newCfg.menuCategory);
                }

                LightData::configIDToJsonCfg[newCfg.configID] = newCfg;
                LightData::defaultConfigs[newCfg.configID] = newCfg;

                niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);

                refLight = true;
                formID = selected->GetFormID();

                SKSE::GetTaskInterface()->AddTask([]() {
                    LightData::ResetTriLightCache();
                    });

                UpdateRefRootTransforms(selected);
            }

            // else its a base ID match
            else {

                refLight = false;

                auto* baseCfgs = LightData::findConfigsByFormID(baseFormID, globals::currentCellIsInterior, true);

                if (!baseCfgs || baseCfgs->empty()) {
                    logger::warn("attach another light: no base ID config found for ref {:08X}, base {:08X}",
                        selected->GetFormID(), baseFormID);
                    break;
                }

                selectedCfgs = *baseCfgs;

                menuCategory = selectedCfgs[0].menuCategory;
                menuName = selectedCfgs[0].menuName;
                jsonFilePath = selectedCfgs[0].configPath;
                entryCount = CountJsonEntriesInFile(selectedCfgs[0].configPath);

                matched = forms::BuildFormIDAndModName(baseFormID, modName);

                newCfg = selectedCfgs[0];

                newCfg.menuCategory = menuCategory;

                std::string finalMenuName =
                    std::format("{} [{}]", StripTrailingIdentifier(newCfg.menuName), entryCount);

                newCfg.menuName = finalMenuName;
                newCfg.configID = globals::nextID++;

                auto root = selected->Get3D();

                if (!root) break;

                auto rootAsNode = root->AsNode();

                if (!rootAsNode) break;

                niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);

                LightData::configIDToJsonCfg[newCfg.configID] = newCfg;
                LightData::defaultConfigs[newCfg.configID] = newCfg;

                SKSE::GetTaskInterface()->AddTask([]() {
                    LightData::ResetTriLightCache();
                    });

                UpdateRefRootTransforms(selected);
            }

            step = AttachLightStep::Done;
            break;
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("You can edit new light in Light Editor as Torch [1], Torch [2] ect.");
        }

        ImGuiMCP::SameLine();

        if (RenderRedButton("Add to exclusion list")) {

            std::string refIDandModName = forms::BuildFormIDAndModName(formID, modName);

            if (!ini::AppendMenuExcludedRefToINI("Data/SKSE/Plugins/ReLight.ini", refIDandModName)) {
                logger::error("Failed to append excluded ref {}", refIDandModName);
            }

            RE::ObjectRefHandle handle = selected->GetHandle();

            SKSE::GetTaskInterface()->AddTask([handle]() {
                int lightsRemoved = 0;

                if (auto ref = handle.get()) {
                    auto a_root = ref->Get3D();
                    if (!a_root) {
                        return;
                    }

                    auto node = a_root->AsNode();
                    if (!node) {
                        return;
                    }

                    std::vector<RE::NiAVObject*> childrenToDetach;
                    std::vector<RE::NiLight*> niLights;

                    for (const auto& childNode : node->GetChildren()) {
                        if (!childNode) {
                            continue;
                        }

                        auto name = std::string_view(childNode->name.c_str());

                        // Relight point lights have RL prefix
                        if (name.size() < 2 || name[0] != 'R' || name[1] != 'L') {
                            continue;
                        }

                        auto* light = netimmerse_cast<RE::NiLight*>(childNode.get());
                        if (!light) {
                            continue;
                        }

                        // collect ni point lights in the ref
                        childrenToDetach.push_back(childNode.get());
                        niLights.push_back(light);
                    }

                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (!ssNode) {
                        logger::warn("ShadowSceneNode[0] is null!");
                        return;
                    }

                    std::vector<RE::NiPointer<RE::BSLight>> bsLightsToRemove;

                    // try to find its matching bs light and remove
                    for (const auto& bsLight : ssNode->activeLights) {
                        if (!bsLight || !bsLight->light) {
                            continue;
                        }

                        for (auto* light : niLights) {
                            if (bsLight->light.get() == light) {
                                bsLightsToRemove.push_back(bsLight);
                                break;
                            }
                        }
                    }

                    for (const auto& bsLight : ssNode->activeShadowLights) {
                        if (!bsLight || !bsLight->light) {
                            continue;
                        }

                        for (auto* light : niLights) {
                            if (bsLight->light.get() == light) {
                                bsLightsToRemove.push_back(bsLight);
                                break;
                            }
                        }
                    }

                    for (const auto& bsLight : bsLightsToRemove) {
                        if (!bsLight || !bsLight->light) {
                            continue;
                        }

                        logger::debug(
                            "BSLight with name {} for ref {:08X} has been removed from ShadowSceneNode",
                            bsLight->light->name,
                            ref->GetFormID());

                        ssNode->RemoveLight(bsLight);
                    }

                    // finally remove nilight from mesh geometry aswell 
                    for (auto* child : childrenToDetach) {
                        if (!child) {
                            continue;
                        }

                        node->DetachChild(child);
                        ++lightsRemoved;
                    }

                    logger::info("Removed {} lights for ref {:08X}", lightsRemoved, ref->GetFormID());
                }
                });

            step = AttachLightStep::LightRemoved;
            break;
        }
        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip(
                "Adds to exclude by refID section in RELight.ini file, preventing object from getting a Relight\n"
                "TIP: Can also use to change a automated light into a seperate light you can edit by itself in the light editor.\n"
                "Just push this button, then when attaching a new light select 'this object only'"
            );
        }

        break;
    }

    case AttachLightStep::ChooseTemplateType:
    {
        ImGuiMCP::Dummy({ 0.0f, 50.0f });
        centerNextItem(430.0f);
        ImGuiMCP::Text("      Attaching light to object selected in console.\nCreate new light template or add to existing template?");


        ImGuiMCP::Spacing();
        ImGuiMCP::Dummy({ 0.0f, 20.0f });
        centerNextItem(630.0f);

        if (RenderYellowButton("Add to a existing template")) {
            createNewTemplate = false;
            step = AttachLightStep::ChooseTemplate;
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("Adding to a existing template keeps your config folder uncluttered.");
        }

        ImGuiMCP::SameLine();

        if (RenderYellowButton("Create a new template")) {
            createNewTemplate = true;
            newCfg = LightConfig{};
            step = AttachLightStep::ChooseScope;
        }

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("Better if you want to control this light seperatly.");
        }
        ImGuiMCP::SameLine();

        if (RenderRedButton("Add to exclusion list")) {

            std::string refIDandModName = forms::BuildFormIDAndModName(formID, modName);

            if (!ini::AppendMenuExcludedRefToINI("Data/SKSE/Plugins/ReLight.ini", refIDandModName)) {
                logger::error("Failed to append excluded ref {}", refIDandModName);
            }

            RE::ObjectRefHandle handle = selected->GetHandle();

            SKSE::GetTaskInterface()->AddTask([handle]() {
                int lightsRemoved = 0;

                if (auto ref = handle.get()) {
                    auto a_root = ref->Get3D();
                    if (!a_root) {
                        return;
                    }

                    auto node = a_root->AsNode();
                    if (!node) {
                        return;
                    }

                    std::vector<RE::NiAVObject*> childrenToDetach;
                    std::vector<RE::NiLight*> niLights;

                    for (const auto& childNode : node->GetChildren()) {
                        if (!childNode) {
                            continue;
                        }

                        auto name = std::string_view(childNode->name.c_str());

                        // Relight point lights have RL prefix
                        if (name.size() < 2 || name[0] != 'R' || name[1] != 'L') {
                            continue;
                        }

                        auto* light = netimmerse_cast<RE::NiLight*>(childNode.get());
                        if (!light) {
                            continue;
                        }

                        // collect ni point lights in the ref
                        childrenToDetach.push_back(childNode.get());
                        niLights.push_back(light);
                    }

                    auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
                    if (!ssNode) {
                        logger::warn("ShadowSceneNode[0] is null!");
                        return;
                    }

                    std::vector<RE::NiPointer<RE::BSLight>> bsLightsToRemove;

                    // try to find its matching bs light and remove
                    for (const auto& bsLight : ssNode->activeLights) {
                        if (!bsLight || !bsLight->light) {
                            continue;
                        }

                        for (auto* light : niLights) {
                            if (bsLight->light.get() == light) {
                                bsLightsToRemove.push_back(bsLight);
                                break;
                            }
                        }
                    }

                    for (const auto& bsLight : ssNode->activeShadowLights) {
                        if (!bsLight || !bsLight->light) {
                            continue;
                        }

                        for (auto* light : niLights) {
                            if (bsLight->light.get() == light) {
                                bsLightsToRemove.push_back(bsLight);
                                break;
                            }
                        }
                    }

                    for (const auto& bsLight : bsLightsToRemove) {
                        if (!bsLight || !bsLight->light) {
                            continue;
                        }

                        logger::debug(
                            "BSLight with name {} for ref {:08X} has been removed from ShadowSceneNode",
                            bsLight->light->name,
                            ref->GetFormID());

                        ssNode->RemoveLight(bsLight);
                    }

                    // finally remove nilight from mesh geometry aswell 
                    for (auto* child : childrenToDetach) {
                        if (!child) {
                            continue;
                        }

                        node->DetachChild(child);
                        ++lightsRemoved;
                    }

                    logger::info("Removed {} lights for ref {:08X}", lightsRemoved, ref->GetFormID());
                }
                });

            step = AttachLightStep::LightRemoved;
            break;
        }
        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip(
                "Adds to exclude by refID section in RELight.ini file, preventing object from getting a Relight\n"
                "TIP: Can also use to change a automated light into a seperate light you can edit by itself in the light editor.\n"
                "Just push this button, then when attaching a new light select 'this object only'"
            );
        }

        break;
    }

    case AttachLightStep::ChooseTemplate:
    {
        centerNextItem(120.0f);
        ImGuiMCP::Text("Select a template.");

        ImGuiMCP::Spacing();

        if (configDisplay.empty()) {

            seenMenuNames.clear();

            auto tryAddBase = [&](RE::FormID key, const std::vector<LightConfig>& cfgVec, bool isExterior) {
                if (cfgVec.empty()) return;
                const auto& cfg = cfgVec[0];

                // A hex ID isn't a usable template name, so don't list nameless base configs
                if (cfg.menuName.empty()) {
                    logger::debug("Template picker: skipping nameless base config {:08X} ({})", key, cfg.configPath);
                    return;
                }

                if (seenMenuNames.insert(toLowerImmut(cfg.menuName)).second) {
                    configDisplay.push_back({ key, cfg, isExterior });
                }
                };

            auto tryAddMesh = [&](const std::string& key, const std::vector<LightConfig>& cfgVec, bool isExterior) {
                if (cfgVec.empty()) return;

                // Copy so the fallback name is what actually gets displayed and sorted
                LightConfig cfg = cfgVec[0];
                if (cfg.menuName.empty()) {
                    cfg.menuName = key;   // the mesh path is a readable name
                }

                if (seenMenuNames.insert(toLowerImmut(cfg.menuName)).second) {
                    configDisplay.push_back({ key, cfg, isExterior });
                }
                };

            for (auto& [key, cfgVec] : LightData::baseFormIDToJsonCfg)
                tryAddBase(key, cfgVec, false);
            for (auto& [key, cfgVec] : LightData::baseFormIDToJsonCfgExteriors)
                tryAddBase(key, cfgVec, true);

            for (auto& [key, cfgVec] : LightData::meshPathToJsonCfg)
                tryAddMesh(key, cfgVec, false);
            for (auto& [key, cfgVec] : LightData::meshPathToJsonCfgExteriors)
                tryAddMesh(key, cfgVec, true);

            std::sort(configDisplay.begin(), configDisplay.end(),
                [](const auto& a, const auto& b)
                {
                    return compareLightNames(
                        std::get<1>(a).menuName.c_str(),
                        std::get<1>(b).menuName.c_str()
                    );
                });
        }

        for (int i = 0; i < static_cast<int>(configDisplay.size()); i++) {
            const auto& [key, cfg, isExterior] = configDisplay[i];

            if (ImGuiMCP::Selectable(cfg.menuName.c_str(), selectedIndex == i)) {
                selectedIndex = i;
            }
        }

        if (selectedIndex == -1) {
            centerNextItem(60.0f);
            if (RenderRedButton("Cancel")) {
                resetState();
            }
            break;
        }
        const auto& [selectedKey, selectedCfg, isExterior] = configDisplay[selectedIndex];
        selectedCfgs.clear();

        if (std::holds_alternative<RE::FormID>(selectedKey)) {
            auto formKey = std::get<RE::FormID>(selectedKey);
            auto& map = isExterior
                ? LightData::baseFormIDToJsonCfgExteriors
                : LightData::baseFormIDToJsonCfg;
            if (auto it = map.find(formKey); it != map.end())
                selectedCfgs = it->second;
        }
        else {
            auto meshKey = std::get<std::string>(selectedKey);
            auto& map = isExterior
                ? LightData::meshPathToJsonCfgExteriors
                : LightData::meshPathToJsonCfg;
            if (auto it = map.find(meshKey); it != map.end())
                selectedCfgs = it->second;
        }

        if (selectedCfgs.empty()) {
            centerNextItem(220.0f);
            logger::error("Selected config was empty or not found.");
            step = AttachLightStep::ChooseTemplateType;
            selectedIndex = -1;
            break;
        }

        centerNextItem(170.0f);

        if (RenderYellowButton("Confirm")) {
            step = AttachLightStep::ChooseScope;
        }

        ImGuiMCP::SameLine();

        if (RenderRedButton("Cancel")) {
            resetState();
        }

        break;
    }
    case AttachLightStep::ChooseScope:
    {
        ImGuiMCP::Dummy({ 0.0f, 50.0f });
        centerNextItem(290.0f);
        ImGuiMCP::Text("This object only, or all objects like it?");

        ImGuiMCP::Spacing();
        ImGuiMCP::Dummy({ 0.0f, 20.0f });

        centerNextItem(330.0f);

        if (RenderYellowButton("This object only")) {

            refLight = true;

            if (createNewTemplate) {

                auto refFormIDandModName = forms::BuildFormIDAndModName(formID, modName);

                newCfg.refFormIDsAndModNames.push_back(refFormIDandModName);
                newCfg.menuName = refFormIDandModName;
                newCfg.configPath = BuildConfigPath(refFormIDandModName);
                newCfg.jsonIndex = 0;
                newCfg.configID = globals::nextID++;
                newCfg.startingFade = newCfg.brightness;
                auto root = selected->Get3D();

                if (!root) break;

                auto rootAsNode = root->AsNode();

                if (!rootAsNode) break;

                niLight = LightManager::AttachLight(newCfg, rootAsNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                LightData::configIDToJsonCfg[newCfg.configID] = newCfg;

                UpdateRefRootTransforms(selected);

                SKSE::GetTaskInterface()->AddTask([]() {
                    LightData::ResetTriLightCache();
                    });


                ini::RemoveFromIniExcludeRefID(selected, refFormIDandModName);
            }
            else {
                auto a_root = selected->Get3D();
                if (!a_root) {
                    logger::warn("Could not load this object's 3D cannnot attach light.");
                    break;
                }

                auto attachNode = a_root->AsNode();
                if (!attachNode) {
                    logger::warn("Could not load this object's node cannnot attach light.");
                    break;
                }

                for (const auto& cfg : selectedCfgs) {

                    niLight = LightManager::AttachLight(cfg, attachNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                }

                UpdateRefRootTransforms(selected);

                SKSE::GetTaskInterface()->AddTask([]() {
                    LightData::ResetTriLightCache();
                    });
            }
            step = AttachLightStep::Done;
            break;
        }

        ImGuiMCP::SameLine();

        if (RenderYellowButton("All like this")) {
            refLight = false;

            RE::TESFile* baseOriginFile = baseObject->GetDescriptionOwnerFile();

            if (baseOriginFile)
                modName = baseOriginFile->fileName;

            std::string baseIDandModName = forms::BuildFormIDAndModName(baseFormID, modName);

            if (createNewTemplate) {

                newCfg.baseFormIDsAndModNames.push_back(baseIDandModName);
                newCfg.menuName = meshPath;
                newCfg.configPath = BuildConfigPath(meshPath);
                newCfg.jsonIndex = 0;
                newCfg.configID = globals::nextID++;
                newCfg.startingFade = newCfg.brightness;

                auto a_root = selected->Get3D();
                if (!a_root) {
                    logger::warn("Could not load this object's 3D cannnot attach light.");
                    break;
                }

                auto attachNode = a_root->AsNode();
                if (!attachNode) {
                    logger::warn("Could not load this object's node cannnot attach light.");
                    break;
                }

                niLight = LightManager::AttachLight(newCfg, attachNode, selected, meshPath, baseFormID, attachedDebugMarker);
                LightData::configIDToJsonCfg[newCfg.configID] = newCfg;


                SKSE::GetTaskInterface()->AddTask([]() {
                    LightData::ResetTriLightCache();
                    });

                UpdateRefRootTransforms(selected);

                step = AttachLightStep::Done;
                break;
            }

            auto a_root = selected->Get3D();
            if (!a_root) {
                ImGuiMCP::Text("Could not load this object's 3D.");
                break;
            }

            auto attachNode = a_root->AsNode();
            if (!attachNode) {
                ImGuiMCP::Text("Could not load this object's node.");
                break;
            }

            if (previewRef != selected->GetFormID() || previewSelectedIndex != selectedIndex) {
                previewRef = selected->GetFormID();
                previewSelectedIndex = selectedIndex;

                for (const auto& cfg : selectedCfgs) {
                    niLight = LightManager::AttachLight(cfg, attachNode, selected, meshPath, selected->GetFormID(), attachedDebugMarker);
                }

                SKSE::GetTaskInterface()->AddTask([]() {
                    LightData::ResetTriLightCache();
                    });

                UpdateRefRootTransforms(selected);
            }

            step = AttachLightStep::Done;
        }

        ImGuiMCP::SameLine();

        if (RenderRedButton("Cancel")) {
            resetState();
            step = AttachLightStep::SelectTarget;
        }

        break;
    }

    case AttachLightStep::Done:
    {
        ImGuiMCP::Dummy({ 0.0f, 50.0f });
        centerNextItem(520.0f);
        ImGuiMCP::Text("Light attached. You MUST confirm before saving in the light editor.");

        ImGuiMCP::Spacing();
        ImGuiMCP::Dummy({ 0.0f, 20.0f });

        bool showMenuNameBox =
            createNewTemplate &&
            !multiLight;

        bool showMenuCategoryBox = createNewTemplate &&
            !multiLight;

        if (showMenuNameBox && !menuNameBufferInitialized) {
            std::string initialName;
            std::string initialCategory = "";

            if (!createNewTemplate) {
                initialName = selectedCfgs[0].menuName;
            }
            else {
                initialName = newCfg.menuName;
            }

            std::strncpy(menuNameBuffer, initialName.c_str(), sizeof(menuNameBuffer) - 1);
            menuNameBuffer[sizeof(menuNameBuffer) - 1] = '\0';

            menuNameBufferInitialized = true;


            std::strncpy(menuCategoryBuffer, initialCategory.c_str(), sizeof(menuCategoryBuffer) - 1);
            menuCategoryBuffer[sizeof(menuCategoryBuffer) - 1] = '\0';

            menuCategoryBufferInitialized = true;
        }

        if (showMenuNameBox) {
            ImGuiMCP::SetCursorPosX(280.0f);
            ImGuiMCP::Text("Set Menu Name: ");
            ImGuiMCP::SameLine();
            // Dummy added so both inputs for name and category are vertically aligned
            ImGuiMCP::Dummy(ImGuiMCP::ImVec2(108.0f, 0.0f));
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(250.0f);

            ImGuiMCP::InputText(
                "##TemplateName",
                menuNameBuffer,
                sizeof(menuNameBuffer));
        }

        if (showMenuCategoryBox) {
            ImGuiMCP::SetCursorPosX(280.0f);
            ImGuiMCP::Text("Set Category Name (optional): ");
            ImGuiMCP::SameLine();
            ImGuiMCP::SetNextItemWidth(250.0f);

            ImGuiMCP::InputText(
                "##TemplateCategory",
                menuCategoryBuffer,
                sizeof(menuCategoryBuffer));
        }

        ImGuiMCP::Dummy({ 0.0f, 20.0f });

        centerNextItem(180.0f);

        if (RenderYellowButton("Confirm")) {

            if (multiLight) {
                entryCount = CountJsonEntriesInFile(newCfg.configPath);
                std::string finalMenuName =
                    std::format("{} [{}]", StripTrailingIdentifier(newCfg.menuName), entryCount);
                std::string finalMenuCategory = newCfg.menuCategory;

                if (refLight) {
                    if (!AppendNewConfigEntryFromLight(
                        jsonFilePath,
                        static_cast<std::uint16_t>(entryCount),
                        finalMenuCategory,
                        finalMenuName,
                        niLight,
                        forms::BuildFormIDAndModName(formID, modName),
                        "",
                        newCfg,
                        true,
                        formID,
                        0,                // baseFormID unused here
                        true)) {
                        logger::error("Failed to append ref multi-light config");
                    }
                }
                else {
                    if (!AppendNewConfigEntryFromLight(
                        jsonFilePath,
                        static_cast<std::uint16_t>(entryCount),
                        finalMenuCategory,
                        finalMenuName,
                        niLight,
                        "",
                        matched,
                        newCfg,
                        false,
                        formID,
                        baseFormID,
                        false)) {
                        logger::error("Failed to append base ID multi-light config");
                    }

                    RefreshNearbyObjectsByBase(selected, baseFormID);

                    // disable original otherwise duplicate light on original
                    selected->Disable();
                    selected->Enable(false);

                }

                globals::baseFormsWithAttachedLights.emplace(baseFormID);
                step = AttachLightStep::SelectTarget;
                break;
            }

            if (!createNewTemplate) {

                if (selectedCfgs.empty()) {
                    logger::warn("Failed to update config file because no template was selected.");
                    resetState();
                    step = AttachLightStep::SelectTarget;
                    break;
                }

                const auto& filePath = selectedCfgs[0].configPath;

                if (!refLight) {

                    RE::TESFile* baseOriginFile = baseObject->GetDescriptionOwnerFile();

                    if (baseOriginFile)
                        modName = baseOriginFile->fileName;

                    std::string baseIDandModName = forms::BuildFormIDAndModName(baseFormID, modName);

                    if (AddFormIDToAllJsonEntries(filePath, baseIDandModName, true)) {
                        logger::info("Added base ID to existing template successfully");
                    }
                    else {
                        logger::warn("Failed to update config file.");
                    }

                    newCfg = selectedCfgs[0];
                    if (std::find(
                        newCfg.baseFormIDsAndModNames.begin(),
                        newCfg.baseFormIDsAndModNames.end(),
                        baseIDandModName) == newCfg.baseFormIDsAndModNames.end())
                    {
                        newCfg.baseFormIDsAndModNames.push_back(baseIDandModName);
                    }

                    LightData::AddConfigToMaps(newCfg, refLight, baseFormID);
                    globals::baseFormsWithAttachedLights.emplace(baseFormID);
                    RefreshNearbyObjectsByBase(selected, baseFormID);
                    resetState();
                    break;
                }

                // refid light
                LightConfig refCfg = selectedCfgs[0];

                std::string refFormIDAndModName = forms::BuildFormIDAndModName(formID, modName);
                ini::RemoveFromIniExcludeRefID(selected, refFormIDAndModName);
                refCfg.configPath = filePath;
                refCfg.jsonIndex = static_cast<std::uint16_t>(CountJsonEntriesInFile(filePath));
                refCfg.menuName = selectedCfgs[0].menuName;
                refCfg.refFormIDsAndModNames.push_back(refFormIDAndModName);

                if (refCfg.refFormIDsAndModNames.empty()) {
                    logger::error("Failed to build ref ID for selected object");
                    resetState();
                    break;
                }

                AddFormIDToAllJsonEntries(refCfg.configPath, refFormIDAndModName, false);

                globals::baseFormsWithAttachedLights.emplace(baseFormID);
                resetState();
                break;
            }
            else {
                if (!multiLight) {
                    newCfg.menuName = menuNameBuffer;
                    newCfg.menuCategory = menuCategoryBuffer;

                    // This gives the new configuration the same file path as their name in the menu
                    newCfg.configPath = BuildConfigPath(newCfg.menuName);
                }

                if (!saveNewConfiguration(newCfg)) {
                    logger::error("Failed to save new template");
                }

                LightData::AddConfigToMaps(newCfg, refLight, refLight ? formID : baseFormID);
                globals::baseFormsWithAttachedLights.emplace(baseFormID);

                if (!refLight) {
                    RefreshNearbyObjectsByBase(selected, baseFormID);
                }
            }

            step = AttachLightStep::SelectTarget;
            break;
        }

        ImGuiMCP::SameLine();

        if (RenderRedButton("Cancel")) {
            RE::ObjectRefHandle handle = selected->GetHandle();

            if (!selectedCfgs.empty()) {
                LightData::configIDToJsonCfg.erase(newCfg.configID);
            }

            SKSE::GetTaskInterface()->AddTask([handle]() {
                if (auto ref = handle.get()) {
                    ref->Disable();
                    ref->Enable(false);
                }
                });

            if (multiLight) {
                LightData::configIDToJsonCfg.erase(newCfg.configID);
                LightData::defaultConfigs.erase(newCfg.configID);
            }

            resetState();
            step = AttachLightStep::SelectTarget;
            break;
        }

        break;
    }

    case AttachLightStep::LightRemoved:
    {
        ImGuiMCP::Dummy({ 0.0f, 50.0f });
        centerNextItem(120.0f);
        ImGuiMCP::Text("Lights removed.");

        ImGuiMCP::Dummy({ 0.0f, 20.0f });
        ImGuiMCP::Spacing();

        centerNextItem(50.0f);
        if (RenderYellowButton("Okay")) {
            step = AttachLightStep::SelectTarget;
            break;
        }

        break;
    }
    }
}