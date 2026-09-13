#pragma once

#include "logger.hpp"

namespace ini {

	void IniParser();

	bool saveSettingsToIni(); 

	bool RemoveMenuExcludedRefFromINI(const std::string& iniPath, const std::string& refIDAndModName);

	bool AppendMenuExcludedRefToINI(const std::string& iniPath, const std::string& refIDAndModName);

	void RemoveFromIniExcludeRefID(RE::TESObjectREFR* ref, std::string& refIDandModName); 

}