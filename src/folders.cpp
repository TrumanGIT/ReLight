#include "folders.h"

#include "SKSEMenuFramework.h"
#include "LightData.h"
#include "Utility.h"

namespace Folders
{
	namespace
	{
		struct Folder
		{
			std::string name;  // as on disk, shown in the menu and written to the ini
			Scale       scale;
		};

		std::vector<Folder>                    folders(1);  // [0] = kNone, always 1.0 / 1.0
		std::unordered_map<std::string, Scale> saved;       // lower-case folder name -> what the ini said

		std::filesystem::path IniPath()
		{
			const auto root = std::filesystem::path(REL::Module::get().filename()).parent_path();
			return root / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / "FolderMultipliers.ini";
		}

		std::string Slashes(std::string a_path)
		{
			std::ranges::replace(a_path, '\\', '/');
			return a_path;
		}

		void Save()
		{
			std::ofstream out(IniPath(), std::ios::trunc);
			if (!out) {
				logger::error("could not write {}", IniPath().string());
				return;
			}
			out << "; ReLight - brightness and reach for each folder in Configs\\. Written by the in-game menu.\n";
			for (std::size_t i = 1; i < folders.size(); ++i) {
				saved[toLowerImmut(folders[i].name)] = folders[i].scale;
			}
			for (const auto& [name, scale] : saved) {  // folders not installed this session keep their values
				out << std::format("\n[{}]\nbrightness={:.2f}\nreach={:.2f}\n", name, scale.brightness, scale.reach);
			}
		}

		// what the folder's sliders change on lights already in the scene; flickering and fading lights take the new
		// brightness on their next frame through the update hooks
		void ApplyLive(std::uint16_t a_folder)
		{
			auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
			if (!ssNode) {
				return;
			}
			auto apply = [a_folder](auto& a_lights) {
				for (auto& light : a_lights) {
					if (!light || !light->light) {
						continue;
					}
					auto&      rt = light->light->GetLightRuntimeData();
					const auto it = LightData::configIDToJsonCfg.find(rt.unk138);
					if (it == LightData::configIDToJsonCfg.end() || it->second.folder != a_folder) {
						continue;
					}
					const auto& cfg = it->second;
					const auto* ref = light->light->GetUserData();
					const float scale = ref ? (std::min)(ref->GetScale(), 1.0f) : 1.0f;
					const float mult = cfg.isPluginLight ? globals::vanillaBrightnessModifier : globals::brightnessModifier;
					rt.fade = cfg.brightness * scale * mult * Get(a_folder).brightness;
					rt.radius = LightData::getNiPointLightRadius(cfg, scale);
					if (globals::islInstalled) {
						LightData::setOverlayData(light->light.get(), cfg);
					}
				}
			};
			auto& rt = ssNode->GetRuntimeData();
			apply(rt.activeLights);
			apply(rt.activeShadowLights);
		}
	}

	void Load()
	{
		std::ifstream in(IniPath());
		std::string   line, section;
		while (std::getline(in, line)) {
			line = trim(line);
			if (line.empty() || line[0] == ';') {
				continue;
			}
			if (line.front() == '[' && line.back() == ']') {
				section = toLowerImmut(line.substr(1, line.size() - 2));
				continue;
			}
			const auto eq = line.find('=');
			if (eq == std::string::npos || section.empty()) {
				continue;
			}
			const auto  key = toLowerImmut(trim(line.substr(0, eq)));
			const float value = std::strtof(trim(line.substr(eq + 1)).c_str(), nullptr);
			if (key == "brightness") {
				saved[section].brightness = std::clamp(value, 0.1f, 2.0f);
			} else if (key == "reach") {
				saved[section].reach = std::clamp(value, 0.5f, 2.0f);
			}
		}
		logger::info("folder multipliers: {} folder(s) read from {}", saved.size(), IniPath().string());
	}

	std::uint16_t IndexOf(const std::string& a_configPath)
	{
		auto       base = Slashes(GetConfigDir());
		const auto path = Slashes(a_configPath);
		if (!base.ends_with('/')) {
			base += '/';
		}
		if (path.size() <= base.size() || toLowerImmut(path.substr(0, base.size())) != toLowerImmut(base)) {
			return kNone;
		}
		const auto slash = path.find('/', base.size());
		if (slash == std::string::npos) {
			return kNone;  // straight in Configs\, no folder
		}
		const auto name = path.substr(base.size(), slash - base.size());
		const auto key = toLowerImmut(name);
		for (std::size_t i = 1; i < folders.size(); ++i) {
			if (toLowerImmut(folders[i].name) == key) {
				return static_cast<std::uint16_t>(i);
			}
		}
		const auto found = saved.find(key);
		folders.push_back({ name, found != saved.end() ? found->second : Scale{} });
		return static_cast<std::uint16_t>(folders.size() - 1);
	}

	const Scale& Get(std::uint16_t a_folder)
	{
		return a_folder < folders.size() ? folders[a_folder].scale : folders[kNone].scale;
	}

	void RenderMenu()
	{
		if (folders.size() <= 1 || !ImGuiMCP::CollapsingHeader("Brightness and Reach per Config Folder")) {
			return;
		}
		for (std::size_t i = 1; i < folders.size(); ++i) {
			auto&      folder = folders[i];
			const auto index = static_cast<std::uint16_t>(i);
			ImGuiMCP::PushID(static_cast<int>(i));
			ImGuiMCP::TextDisabled("%s", folder.name.c_str());
			bool changed = ImGuiMCP::SliderFloat("Brightness", &folder.scale.brightness, 0.1f, 2.0f, "%.2f");
			bool released = ImGuiMCP::IsItemDeactivatedAfterEdit();
			changed |= ImGuiMCP::SliderFloat("Reach", &folder.scale.reach, 0.5f, 2.0f, "%.2f");
			released |= ImGuiMCP::IsItemDeactivatedAfterEdit();
			if (changed) {
				ApplyLive(index);
			}
			if (released) {
				Save();
			}
			ImGuiMCP::PopID();
		}
	}
}
