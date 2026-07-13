#pragma once

namespace forms {

	bool ContainsEditorID(
		const std::string& edid,
		const std::vector<std::string>& keywords);

	bool isLightPluginFormID(RE::FormID formID);

	std::uint32_t getLightPluginLocalFormID(RE::FormID formID);

	std::uint32_t getLocalFormID(RE::FormID formID);

	bool isExcludedRef(const RE::TESObjectREFR* ref);

	bool isExcludedBaseID(const RE::TESObjectREFR* ref);

	bool isExclude(
		const std::string& meshPath,
		RE::TESObjectREFR* ref);

	std::string BuildRefIDAndModName(RE::TESObjectREFR* ref); 

	const RE::TESFile* ResolveTESFileWithFallback(
		RE::TESDataHandler* dataHandler,
		const std::string& modName); 

}