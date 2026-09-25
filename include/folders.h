#pragma once

#include <cstdint>
#include <string>

// Brightness and reach per folder under Data\SKSE\Plugins\Relight\Configs\ - a light add-on that keeps its configs in
// its own folder gets its own pair of sliders, and a player can tune it without touching anyone else's lights.
// Saved to Data\SKSE\Plugins\Relight\FolderMultipliers.ini (written by the menu, one [folder] section each).
namespace Folders
{
	inline constexpr std::uint16_t kNone = 0;  // a config directly in Configs\ is never scaled

	struct Scale
	{
		float brightness = 1.0f;  // multiplies fade
		float reach = 1.0f;       // multiplies radius; divides the ISL cutoff by reach squared
	};

	void          Load();                                     // once, before parseTemplates()
	std::uint16_t IndexOf(const std::string& a_configPath);  // while parsing: the config's folder, registered on first sight
	const Scale&  Get(std::uint16_t a_folder);
	void          RenderMenu();                               // sliders for every folder, applied live, saved on release
}
