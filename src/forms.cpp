#include "forms.hpp"
#include "logger.hpp"
#include "global.h"

namespace forms {

	 bool ContainsEditorID(
		const std::string& edid,
		const std::vector<std::string>& keywords)
	{
		if (edid.empty()) {
			return false;
		}

		for (const auto& keyword : keywords) {
			//	logger::debug("comparing vanilla light {} to {}", edid, keyword);

			if (edid.contains(keyword)) {

				// special exception
				if (edid.contains("solitudeinnsunlightshadow")) {
					return false;
				}

				logger::info("Matched editorID '{}' with keyword '{}'", edid, keyword);
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

	  std::string BuildFormIDAndModName(RE::TESObjectREFR* ref, bool baseID)
	 {
		 if (!ref) {
			 return "";
		 }

		 const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
		 std::string modName = refOriginFile ? refOriginFile->fileName : "";

		 if (modName.empty()) {
			 logger::warn("BuildRefIDAndModName: ref {:08X} has no owning file", ref->GetFormID());
			 return "";
		 }


		 RE::FormID runtimeID = 0x0; 
		 if (baseID) {
			 auto baseObject = ref->GetBaseObject();
			 if (!baseObject) return "";
			 runtimeID = baseObject->GetFormID();
		 }

		 else {
			 runtimeID = ref->GetFormID();
		 }

		 if (refOriginFile->IsLight()) {
			 // Best option if available in your CommonLib version
			 const auto localID = ref->GetLocalFormID();

			 logger::info(
				 "Built light plugin ref string: 0x{:X}~{} (runtime: 0x{:08X})",
				 static_cast<std::uint32_t>(localID),
				 modName,
				 static_cast<std::uint32_t>(runtimeID));

			 return std::format("0x{:X}~{}", static_cast<std::uint32_t>(localID), modName);
		 }

		 else {
			 runtimeID &= 0x00FFFFFF;
		 }

		 logger::info(
			 "Built non-light plugin ref string: 0x{:X}~{}",
			 static_cast<std::uint32_t>(runtimeID),
			 modName);

		 return std::format("0x{:X}~{}", static_cast<std::uint32_t>(runtimeID), modName);
	 }


	  // I think the idea was 0004502~embersxd.esp did not work for 0004502~skyrim.esp so i just use the base game ids as a fall back
	   const RE::TESFile* ResolveTESFileWithFallback(
		  RE::TESDataHandler* dataHandler,
		  const std::string& modName)
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