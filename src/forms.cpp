#include "forms.hpp"
#include "logger.hpp"
#include "global.h"
#include "utility.h"

namespace forms {

	 bool ContainsEditorID(
		const std::string& edid,
		const std::vector<std::string>& keywords)
	{

		if (edid.empty()) {
			return false;
		}

		std::string lowerEdid = toLowerImmut(edid);  

		for (const auto& keyword : keywords) {
				//logger::debug("comparing vanilla light {} to {}", lowerEdid, keyword);

			if (lowerEdid.contains(keyword)) {

				// special exception
				if (edid.contains("solitudeinnsunlightshadow")) {
					return false;
				}

				logger::info("Matched editorID '{}' with keyword '{}'", lowerEdid, keyword);
				return true;
			}
		}

		return false;
	}

	 bool isLightPluginFormID(RE::FormID formID)
	{
		return (formID & 0xFF000000) == 0xFE000000;
	}

	 std::uint32_t getLightPluginLocalFormID(RE::FormID formID)
	{
		return formID & 0x00000FFF;
	}

	 std::uint32_t getLocalFormID(RE::FormID formID)
	{
		if (isLightPluginFormID(formID)) {
			return formID & 0x00000FFF;  // ESL/light plugin
		}

		return formID & 0x00FFFFFF;      // normal plugin
	}

	 bool isExcludedRef(const RE::TESObjectREFR* ref)
	{
		if (!ref) {
			return false;
		}

		RE::FormID runtimeFormID = ref->GetFormID();

		auto rawIndex = (runtimeFormID & 0xFF000000) >> 24;

		bool isLight = rawIndex == 0xFE;

		if (!isLight) {
			runtimeFormID &= 0x00FFFFFF;
		}

		if (globals::excludedRefFormIDs.contains(runtimeFormID)) {
			logger::debug("excluded ref runtime formID 0x{:08X} skipping light attachment",
				static_cast<std::uint32_t>(runtimeFormID));
			return true;
		}

		return false;
	}

	 bool isExcludedBaseID(const RE::TESObjectREFR* ref)
	{
		if (!ref) {
			return false;
		}

		auto baseObject = ref->GetBaseObject();

		if (!baseObject) return false;

		RE::FormID runtimeFormID = baseObject->GetFormID();

		auto rawIndex = (runtimeFormID & 0xFF000000) >> 24;

		bool isLight = rawIndex == 0xFE;

		if (!isLight) {
			runtimeFormID &= 0x00FFFFFF;
		}

		// old behavior still works
		if (globals::excludedBaseFormIDs.contains(runtimeFormID)) {
			logger::debug("excluded Base runtime formID 0x{:08X} skipping light attachment",
				static_cast<std::uint32_t>(runtimeFormID));
			return true;
		}

		return false;
	}

	 bool isExclude(const std::string& meshPath, RE::TESObjectREFR* ref)
	{
		// Exact matches in exclusion list
		for (const auto& exclude : globals::meshPathExclusionList) {
			if (meshPath == exclude) {
				logger::debug("isExclude: '{}' matched exact mesh exclude '{}' skipping light attachment", meshPath, exclude);
				return true;
			}
		}

		// Partial matches in exclusion list
		for (const auto& exclude : globals::meshPathExclusionListPartialMatch) {
			if (meshPath.contains(exclude)) {
				logger::debug("isExclude: '{}' matched partial mesh exclude '{}' skipping light attachment", meshPath, exclude);
				return true;
			}
		}

		if (isExcludedRef(ref)) {
			return true;
		}

		if (isExcludedBaseID(ref)) {
			return true;
		}

		return false;
	}

	 std::string BuildFormIDAndModName(RE::FormID formID, const std::string& modName)
	 {
		 if (formID == 0) {
			 logger::error("BuildFormIDAndModName: formID is 0");
			 return "";
		 }

		 if (modName.empty()) {
			 logger::error("BuildFormIDAndModName: modName is empty, 0x{:X}", formID);
			 return "";
		 }

		 auto rawIndex = (formID & 0xFF000000) >> 24;
		 bool isLight = rawIndex == 0xFE;

		 if (isLight) {
			 // Light plugin: use the local FormID (lower 12 bits)
			 formID &= 0x00000FFF;
			 logger::info("Built light plugin ref string: 0x{:03X}~{}", formID, modName);
			 return std::format("0x{:03X}~{}", formID, modName);
		 }
		 else {
			 // Normal plugin: strip the load order index
			 formID &= 0x00FFFFFF;
		 }

		 logger::info("Built non-light plugin ref string: 0x{:X}~{}", formID, modName);
		 return std::format("0x{:05X}~{}", formID, modName);
	 }


	  // I think the idea was 0004502~embersxd.esp did not work for 0004502~skyrim.esp so i just use the base game ids as a fall back
	   const RE::TESFile* ResolveTESFileWithFallback(
		  RE::TESDataHandler* dataHandler,
		  const std::string& modName, bool isLightPlugin, bool isPluginLight)
	  {


		  static const std::array<std::string, 4> kVanillaFallbacks =
		  {
			  "Skyrim.esm",
			  "Update.esm",
			  "Dawnguard.esm",
			  "Dragonborn.esm"
		  };

		  if (!dataHandler)
			  return nullptr;

		  // 1. Try exact mod name
		  if (auto* file = dataHandler->LookupLoadedModByName(modName))
			  return file;

		  if (auto* file = dataHandler->LookupLoadedLightModByName(modName))
			  return file;

		  // light plugins shouldent use fall backs as it will cause unintended matches
		  if (isLightPlugin || isPluginLight) return nullptr;

		  // 2. Fallback to vanilla masters
		  for (const auto& master : kVanillaFallbacks)
		  {
			  if (auto* file = dataHandler->LookupLoadedModByName(master))
				  return file;

			  if (auto* file = dataHandler->LookupLoadedLightModByName(master))
				  return file;
		  }

		  return nullptr;
	  }

}