#pragma once

class Particle
{
public:
	enum class TYPE
	{
		kNone = 0,
		kMisc,
		kCandle,
		kFire,
		kSmoke,
		kSparks,
		kSteam
	};

	explicit Particle(float a_windStrength);

	static TYPE GetType(RE::NiParticleSystem* a_particleSystem);

	// member
	float windStrength;
};

class Manager
{
public:
	[[nodiscard]] static Manager* GetSingleton()
	{
		static Manager singleton;
		return std::addressof(singleton);
	}

	static std::string& SanitizePath(std::string& a_string);

	float GetParticleWind(const std::string& a_nifPath, RE::NiParticleSystem* a_particleSystem);

private:
	Manager();

	Manager(const Manager&) = delete;
	Manager(Manager&&) = delete;
	~Manager() = default;

	Manager& operator=(const Manager&) = delete;
	Manager& operator=(Manager&&) = delete;

	void LoadSettings();
	void LoadOverrides();

	// members
	Particle candle{ 5.0f };
	Particle fire{ 5.0f };
	Particle smoke{ 5.0f };
	Particle sparks{ 5.0f };
	Particle steam{ 5.0f };
	Particle misc{ 5.0f };

	Set<std::string> whiteList{};

	Map<Particle::TYPE, std::string> types{
		{ Particle::TYPE::kNone, "None" },
		{ Particle::TYPE::kCandle, "Candle" },
		{ Particle::TYPE::kFire, "Fire" },
		{ Particle::TYPE::kSmoke, "Smoke" },
		{ Particle::TYPE::kSparks, "Sparks" },
		{ Particle::TYPE::kSteam, "Steam" },
		{ Particle::TYPE::kMisc, "Misc" },
	};
};