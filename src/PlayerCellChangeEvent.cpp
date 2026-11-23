#include "PlayerCellChangeEvent.h"
#include "LightManager.h"
#include "utility.h"

//USED TO REINITIALIZE LIGHTS CLEANED BY THE ENGINE. USUALLY WHEN GOING FROM INTERIOR TO EXTERIOR, BACK TO INTERIOR AND VICE VERSA
RE::BSEventNotifyControl CellChangeEvent::ProcessEvent(const RE::BGSActorCellEvent* event,
	RE::BSTEventSource<RE::BGSActorCellEvent>*) {


	if (!event || event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
		return RE::BSEventNotifyControl::kContinue;
	}

	auto player = RE::PlayerCharacter::GetSingleton();

	if (!player) return RE::BSEventNotifyControl::kContinue;

	static bool s_firstCellEvent = true;

	auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(event->cellID);
	if (!cell) {
		return RE::BSEventNotifyControl::kContinue;
	}

	// set the current cell is interior or not
	globals::currentCellIsInterior = cell->IsInteriorCell();

	if (s_firstCellEvent) {

		s_firstCellEvent = false;

		logger::info("player is in interior on startup: {}", globals::currentCellIsInterior);
		globals::lastCellWasInterior = cell->IsInteriorCell();

		globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
		globals::cellFullyLoaded.store(true);

		//this is the games default brightness setting, its not related to lights
		auto newBrightness = globals::currentCellIsInterior ? globals::interiorBrightness : globals::exteriorBrightness;

		SetBaseGameBrightness(newBrightness);
		

		return RE::BSEventNotifyControl::kContinue;
	}

	// player changes from interor to exterior or vice versa, must reinitialize lights
	if (globals::lastCellWasInterior != globals::currentCellIsInterior) {

		globals::secondAfterCellFullyLoaded.store(false);
		globals::cellFullyLoaded.store(false);

		logger::debug(" new cell detected.. islightaffectingsurface hook stopped");

		LightManager::reinitializeLightsWithinRange(player);

		LightManager::ReinitializeEnchantmentLights(player);

		globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
		globals::cellFullyLoaded.store(true);

		auto newBrightness = globals::currentCellIsInterior ? globals::interiorBrightness : globals::exteriorBrightness;

		SetBaseGameBrightness(newBrightness);

		LightData::triLightCacheGeneration.fetch_add(1);
	}

	// player changes from interor to another interior, must reinitialize otherwise engine cleans the lights
	if (globals::currentCellIsInterior && globals::lastCellWasInterior) {

		// not a second after cell fully laoded so reset
		globals::secondAfterCellFullyLoaded.store(false);
		globals::cellFullyLoaded.store(false);

		logger::debug("new cell detected.. islightaffectingsurface hook stopped");

		LightManager::reinitializeLightsWithinRange(player);

		LightManager::ReinitializeEnchantmentLights(player);

		// start timer and say cell fully loaded is true
		globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
		globals::cellFullyLoaded.store(true);

		LightData::triLightCacheGeneration.fetch_add(1);
	}

	//set the prev cell after finished evaluating new cell
	globals::lastCellWasInterior = globals::currentCellIsInterior;

	return RE::BSEventNotifyControl::kContinue;
}

void CellChangeEvent::registerEventSink()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AsBGSActorCellEventSource()->AddEventSink(CellChangeEvent::GetSingleton());
		logger::info("BGSActorCellEvent sink registered");
	}
}