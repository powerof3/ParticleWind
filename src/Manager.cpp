#include "Manager.h"

Particle::Particle(float a_windStrength) :
	windStrength(a_windStrength)
{}

Particle::TYPE Particle::GetType(RE::NiParticleSystem* a_particleSystem)
{
	// skip if existing wind modifier is found
	// iterate from end, BSWindModifier is one of the last modifiers in list
	// tbd : write iterators for NiTList in clib
	auto tail = a_particleSystem->modifierList.tail;
	while (tail != nullptr) {
		if (const auto& element = tail->element) {
			if (netimmerse_cast<RE::BSWindModifier*>(element.get())) {
				return TYPE::kNone;
			}
		}
		tail = tail->prev;
	}

	const auto effect = a_particleSystem->properties[RE::BSGeometry::States::kEffect].get();
	const auto effectProp = effect ? netimmerse_cast<RE::BSEffectShaderProperty*>(effect) : nullptr;

	if (const auto effectMaterial = effectProp ? static_cast<RE::BSEffectShaderMaterial*>(effectProp->material) : nullptr) {
		const auto& sourceTexture = effectMaterial->sourceTexturePath;
		if (sourceTexture.contains("smoke")) {
			return TYPE::kSmoke;
		}
		if (sourceTexture.contains("steam")) {
			return TYPE::kSteam;
		}
		if (sourceTexture.contains("candleflame")) {
			return TYPE::kCandle;
		}
		if (sourceTexture.contains("fire") || sourceTexture.contains("flame")) {
			return TYPE::kFire;
		}
		if (sourceTexture.contains("iceshards") || sourceTexture.contains("spark")) {
			return TYPE::kSparks;
		}
		if (sourceTexture.contains("enbl") || sourceTexture.contains("glow") || sourceTexture.contains("fxwhiteflare")) {
			return TYPE::kNone;
		}
	}

	return TYPE::kMisc;
}

void Manager::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_ParticleWind.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, dumpGoodNifs, "Debug", "DumpGoodParticles", ";Dump processed particle systems and their types to log");
	ini::get_value(ini, dumpBadNifs, "Debug", "DumpBadParticles", ";Dump skipped particle systems to log");

	ini::get_value(ini, candle.windStrength, "Wind Strength", "Candle", nullptr);
	ini::get_value(ini, fire.windStrength, "Wind Strength", "Fire", nullptr);
	ini::get_value(ini, smoke.windStrength, "Wind Strength", "Smoke", nullptr);
	ini::get_value(ini, sparks.windStrength, "Wind Strength", "Sparks", nullptr);
	ini::get_value(ini, steam.windStrength, "Wind Strength", "Steam", nullptr);
	ini::get_value(ini, misc.windStrength, "Wind Strength", "Misc", nullptr);

	(void)ini.SaveFile(path);
}

void Manager::LoadOverrides()
{
	logger::info("{:*^30}", "CONFIGS");

	std::vector<std::string> configs;

	const std::filesystem::path folderPath{ R"(Data\ParticleWind)" };
	const std::filesystem::directory_entry directory{ folderPath };
	if (!directory.exists()) {
		std::filesystem::create_directory(folderPath);
	}
	const std::filesystem::directory_iterator directoryIt{ folderPath };

	for (const auto& entry : directoryIt) {
		if (entry.exists()) {
			if (const auto& path = entry.path(); !path.empty() && path.extension() == ".ini"sv) {
				configs.push_back(entry.path().string());
			}
		}
	}

	if (configs.empty()) {
		logger::info("No configs found within {}...", folderPath.string());
		return;
	}

	std::ranges::sort(configs);

	logger::info("{} configs found in {}", configs.size(), folderPath.string());

	for (auto& path : configs) {
		logger::info("\tINI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetAllowKeyOnly();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("	couldn't read INI");
			continue;
		}

		if (const auto values = ini.GetSection("Whitelist"); values && !values->empty()) {
			logger::info("\t\t{} whitelist entries", values->size());
			for (const auto& key : *values | std::views::keys) {
				std::string entry{ key.pItem };
				string::trim(entry);
				SanitizePath(entry);

				whiteList.emplace(entry);
			}
		}

		if (const auto values = ini.GetSection("Override"); values && !values->empty()) {
			logger::info("\t\t{} override entries", values->size());
			for (const auto& key : *values | std::views::keys) {
				std::string entry{ key.pItem };
				string::trim(entry);
				auto splitEntry = string::split(entry, "|");

				auto& nifPath = splitEntry[0];
				SanitizePath(nifPath);

				auto& particleEntries = splitEntry[1];
				SanitizeString(particleEntries);

				auto windStrength = string::to_num<float>(splitEntry[2]);

			    auto particlesEntry = string::split(particleEntries, ",");
				for (auto& particle : particlesEntry) {
					overrides[nifPath].emplace(particle, windStrength);
				}
			}
		}
	}
}

Manager::Manager()
{
	LoadSettings();
	LoadOverrides();
}

void Manager::SanitizeString(std::string& a_string)
{
	std::ranges::transform(a_string, a_string.begin(),
		[](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
}

std::string& Manager::SanitizePath(std::string& a_string)
{
	SanitizeString(a_string);

	a_string = srell::regex_replace(a_string, srell::regex(R"(/+|\\+)"), R"(\)");
	a_string = srell::regex_replace(a_string, srell::regex(R"(^\\+)"), "");
	a_string = srell::regex_replace(a_string, srell::regex(R"(.*?[^\s]meshes\\|^meshes\\)", srell::regex::icase), "");

	return a_string;
}

float Manager::GetParticleWind(const std::string& a_nifPath, const std::string& a_nifName, RE::NiParticleSystem* a_particleSystem)
{
	static Map<std::string, Set<std::string>> goodNifs;

    auto oIt = overrides.find(a_nifPath);
	if (oIt == overrides.end() && a_nifPath != a_nifName) {
		oIt = overrides.find(a_nifName);
	}

	if (oIt != overrides.end()) {
		std::string particleSysName{ a_particleSystem->name.c_str() };
		SanitizeString(particleSysName);
		if (const auto pIt = oIt->second.find(particleSysName); pIt != oIt->second.end()) {
			if (dumpGoodNifs) {
				auto& path = oIt->first;
				if (!goodNifs.contains(path)) {
					logger::info("OVERRIDE : {}", path);
				}
				if (!goodNifs[path].contains(a_particleSystem->name.c_str())) {
					logger::info("\t{} : {}", a_particleSystem->name, pIt->second);
					goodNifs[path].emplace(a_particleSystem->name.c_str());
				}
			}
	        return pIt->second;
	    }
	}

	auto wIt = whiteList.find(a_nifPath);
	if (wIt == whiteList.end() && a_nifPath != a_nifName) {
		wIt = whiteList.find(a_nifName);
	}

	if (wIt != whiteList.end()) {
		if (const auto particleType = Particle::GetType(a_particleSystem); particleType != Particle::TYPE::kNone) {
			if (dumpGoodNifs) {
				auto& path = *wIt;
				if (!goodNifs.contains(path)) {
					logger::info("GOOD : {}", path);
				}
				if (!goodNifs[path].contains(a_particleSystem->name.c_str())) {
						logger::info("\t{} : {}", a_particleSystem->name, types[particleType]);
				    goodNifs[path].emplace(a_particleSystem->name.c_str());
				}
			}
			switch (particleType) {
			case Particle::TYPE::kCandle:
				return candle.windStrength;
			case Particle::TYPE::kFire:
				return fire.windStrength;
			case Particle::TYPE::kSmoke:
				return smoke.windStrength;
			case Particle::TYPE::kSparks:
				return sparks.windStrength;
			case Particle::TYPE::kSteam:
				return steam.windStrength;
			case Particle::TYPE::kMisc:
				return misc.windStrength;
			default:
				return 0.0f;
			}
		}
	}

	if (dumpBadNifs) {
        static Map<std::string, Set<std::string>> badNifs;
        if (!badNifs.contains(a_nifPath)) {
			logger::info("BAD : {}", a_nifPath);
		}
		if (!badNifs[a_nifPath].contains(a_particleSystem->name.c_str())) {
			badNifs[a_nifPath].emplace(a_particleSystem->name.c_str());
			logger::info("\t{}", a_particleSystem->name);
		}
	}

	return 0.0f;
}
