#include "vanillaMenus.h"
#include "global.h"


RE::UI_MESSAGE_RESULTS InventoryMenu::thunk(RE::UIMessage& a_message)
{
	switch (a_message.type.get())
	{
	case RE::UI_MESSAGE_TYPE::kShow:
		globals::unDesiredMenuOpen.store(true);
		logger::debug("inventory menu hook fired menu OPEN");
		break;

	case RE::UI_MESSAGE_TYPE::kHide:
		globals::unDesiredMenuOpen.store(false);
		logger::debug("inventory menu hook fired menu CLOSED");
		break;
	}

	return func(this, a_message);
}

void InventoryMenu::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::VTABLE_InventoryMenu[0])
		.write_vfunc(idx, &InventoryMenu::thunk);
	logger::info("Hooked TESObjectREFR::Load3D");
}

RE::UI_MESSAGE_RESULTS CraftingMenu::thunk(RE::UIMessage& a_message)
{
	switch (a_message.type.get())
	{
	case RE::UI_MESSAGE_TYPE::kShow:
		globals::unDesiredMenuOpen.store(true);
		logger::debug("crafting menu hook fired menu OPEN");
		break;

	case RE::UI_MESSAGE_TYPE::kHide:
		globals::unDesiredMenuOpen.store(false);
		logger::debug("crafting menu hook fired menu CLOSED");
		break;
	}

	return func(this, a_message);
}

void CraftingMenu::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::VTABLE_CraftingMenu[0])
		.write_vfunc(idx, &CraftingMenu::thunk);
	logger::info("Hooked Crafting Menu");
}

