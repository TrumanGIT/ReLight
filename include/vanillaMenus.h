#pragma once

// inherit from class so we can use "this" in return
struct InventoryMenu : RE::InventoryMenu
{
    RE::UI_MESSAGE_RESULTS thunk(RE::UIMessage& a_message);

    using ProcessMessage_t = decltype(&RE::InventoryMenu::ProcessMessage);
    static inline REL::Relocation<ProcessMessage_t> func;

    static constexpr std::size_t idx{ 0x4 };

    static void Install();
};

// inherit from class so we can use "this" in return
struct CraftingMenu : RE::CraftingMenu
{
    RE::UI_MESSAGE_RESULTS thunk(RE::UIMessage& a_message);

    using ProcessMessage_t = decltype(&RE::CraftingMenu::ProcessMessage);
    static inline REL::Relocation<ProcessMessage_t> func;

    static constexpr std::size_t idx{ 0x4 };

    static void Install();
};
