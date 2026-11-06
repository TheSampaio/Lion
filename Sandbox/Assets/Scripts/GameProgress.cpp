#include "GameProgress.h"

#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace Lion;

namespace
{
	constexpr std::array<GameProgress::Achievement, GameProgress::kAchievementCount> kAchievements = {{
		{ "FIRST_CONTACT", "FIRST CONTACT", "Destroy your first brick.", "Sprites/Brickout/achievement-first.png" },
		{ "CENTURY", "CENTURY", "Destroy 100 bricks.", "Sprites/Brickout/achievement-century.png" },
		{ "CHAIN_REACTION", "CHAIN REACTION", "Reach an 8x combo.", "Sprites/Brickout/achievement-combo.png" },
		{ "POWER_ON", "POWER ON", "Collect your first power-up.", "Sprites/Brickout/power-life.png" },
		{ "WAVE_RIDER", "WAVE RIDER", "Fire a shockwave.", "Sprites/Brickout/power-shockwave.png" },
		{ "CIRCUIT_BREAKER", "CIRCUIT BREAKER", "Clear your first level.", "Sprites/Brickout/achievement-clear.png" },
		{ "HIGH_VOLTAGE", "HIGH VOLTAGE", "Earn 100,000 total points.", "Sprites/Brickout/achievement-score.png" },
		{ "DEEP_RUN", "DEEP RUN", "Clear 25 unique levels.", "Sprites/Brickout/achievement-depth.png" },
		{ "STAYING_POWER", "STAYING POWER", "Play for one hour.", "Sprites/Brickout/achievement-time.png" },
		{ "MASTER_CIRCUIT", "MASTER CIRCUIT", "Clear all 100 levels.", "Sprites/Brickout/achievement-master.png" },
	}};

	std::string EnvironmentValue(const char8* name)
	{
#ifdef LN_PLATFORM_WIN
		char8* value = nullptr;
		size_t length = 0;
		_dupenv_s(&value, &length, name);
		const std::string result = value ? value : "";
		std::free(value);
		return result;
#else
		const char8* value = std::getenv(name);
		return value ? value : "";
#endif
	}

	std::filesystem::path SavePath()
	{
		const std::string overrideRoot = EnvironmentValue("LION_BRICKOUT_SAVE_ROOT");
		if (!overrideRoot.empty())
			return std::filesystem::path(overrideRoot) / "progress.save";

#ifdef LN_PLATFORM_WIN
		const std::string root = EnvironmentValue("LOCALAPPDATA");
		const std::filesystem::path base = !root.empty()
			? std::filesystem::path(root)
			: std::filesystem::temp_directory_path();
		return base / "Lion" / "Brickout" / "progress.save";
#else
		const std::string root = EnvironmentValue("HOME");
		const std::filesystem::path base = !root.empty()
			? std::filesystem::path(root) / ".local" / "share"
			: std::filesystem::temp_directory_path();
		return base / "lion" / "brickout" / "progress.save";
#endif
	}

	int32 ParseValue(const std::string& value, int32 fallback)
	{
		try
		{
			return std::stoi(value);
		}
		catch (const std::exception&)
		{
			return fallback;
		}
	}

	uint64 ParseUnsigned(const std::string& value, uint64 fallback)
	{
		try
		{
			return std::stoull(value);
		}
		catch (const std::exception&)
		{
			return fallback;
		}
	}
}

void GameProgress::Load()
{
	if (sLoaded)
		return;

	sLoaded = true;
	std::ifstream stream(SavePath());

	if (!stream)
		return;

	std::string line;
	while (std::getline(stream, line))
	{
		const size_t separator = line.find('=');
		if (separator == std::string::npos)
			continue;

		const std::string key = line.substr(0, separator);
		const std::string value = line.substr(separator + 1);
		if (key == "highestUnlocked") sHighestUnlockedLevel = ParseValue(value, 1);
		else if (key == "completedMask")
		{
			const uint32 legacyMask = static_cast<uint32>(ParseValue(value, 0));
			for (int32 index = 0; index < 32 && index < kLevelCount; ++index)
				sCompletedLevels.set(index, (legacyMask & (1u << index)) != 0);
		}
		else if (key == "completedLevels")
		{
			for (int32 index = 0; index < kLevelCount && index < static_cast<int32>(value.size()); ++index)
				sCompletedLevels.set(index, value[value.size() - 1 - index] == '1');
		}
		else if (key == "achievements") sAchievementMask = ParseUnsigned(value, 0);
		else if (key == "totalScore") sTotalScore = ParseValue(value, 0);
		else if (key == "sessionsPlayed") sSessionsPlayed = ParseValue(value, 0);
		else if (key == "levelsCompleted") sLevelsCompleted = ParseValue(value, 0);
		else if (key == "bricksDestroyed") sBricksDestroyed = ParseValue(value, 0);
		else if (key == "ballsLost") sBallsLost = ParseValue(value, 0);
		else if (key == "powersCollected") sPowersCollected = ParseValue(value, 0);
		else if (key == "highestCombo") sHighestCombo = ParseValue(value, 1);
		else if (key == "shockwavesFired") sShockwavesFired = ParseValue(value, 0);
		else if (key == "playedSeconds") sPlayedSeconds = ParseValue(value, 0);
		else if (key.rfind("levelScore", 0) == 0)
		{
			const int32 level = ParseValue(key.substr(10), 0);
			if (level >= 1 && level <= kLevelCount)
				sLevelHighScores[level - 1] = ParseValue(value, 0);
		}
	}
	stream.close();

	sHighestUnlockedLevel = std::clamp(sHighestUnlockedLevel, 1, kLevelCount);
	sHighestCombo = std::clamp(sHighestCombo, 1, 16);
	EvaluateAchievements();
}

void GameProgress::StartSession()
{
	EnsureLoaded();
	sSessionsPlayed++;
	Save();
}

void GameProgress::CompleteLevel(int32 level, int32 score, float32 playedSeconds)
{
	EnsureLoaded();
	level = std::clamp(level, 1, kLevelCount);
	sCompletedLevels.set(level - 1);
	sHighestUnlockedLevel = std::max(sHighestUnlockedLevel, std::min(level + 1, kLevelCount));
	sLevelsCompleted++;
	sTotalScore += std::max(score, 0);
	sLevelHighScores[level - 1] = std::max(sLevelHighScores[level - 1], score);
	sPlayedSeconds += std::max(static_cast<int32>(std::round(playedSeconds)), 0);
	EvaluateAchievements();
	Save();
}

void GameProgress::EndAttempt(int32 level, int32 score, float32 playedSeconds)
{
	EnsureLoaded();
	level = std::clamp(level, 1, kLevelCount);
	sTotalScore += std::max(score, 0);
	sLevelHighScores[level - 1] = std::max(sLevelHighScores[level - 1], score);
	sPlayedSeconds += std::max(static_cast<int32>(std::round(playedSeconds)), 0);
	EvaluateAchievements();
	Save();
}

void GameProgress::RegisterBrickDestroyed()
{
	EnsureLoaded();
	sBricksDestroyed++;
	EvaluateAchievements();
}

void GameProgress::RegisterBallLost()
{
	EnsureLoaded();
	sBallsLost++;
}

void GameProgress::RegisterPowerCollected()
{
	EnsureLoaded();
	sPowersCollected++;
	EvaluateAchievements();
}

void GameProgress::RegisterCombo(int32 combo)
{
	EnsureLoaded();
	sHighestCombo = std::max(sHighestCombo, std::clamp(combo, 1, 16));
	EvaluateAchievements();
}

void GameProgress::RegisterShockwave()
{
	EnsureLoaded();
	sShockwavesFired++;
	EvaluateAchievements();
}

void GameProgress::ResetAll()
{
	std::error_code error;
	std::filesystem::remove(SavePath(), error);
	sLoaded = true;
	sHighestUnlockedLevel = 1;
	sCompletedLevels.reset();
	sTotalScore = 0;
	sLevelHighScores.fill(0);
	sSessionsPlayed = 0;
	sLevelsCompleted = 0;
	sBricksDestroyed = 0;
	sBallsLost = 0;
	sPowersCollected = 0;
	sHighestCombo = 1;
	sShockwavesFired = 0;
	sPlayedSeconds = 0;
	sAchievementMask = 0;
	sRecentlyUnlockedAchievements.clear();
	Log::Console(LogLevel::Success, "[GameProgress] All Brickout progress was reset.");
}

int32 GameProgress::GetHighestUnlockedLevel() { EnsureLoaded(); return sHighestUnlockedLevel; }
int32 GameProgress::GetCompletedLevelCount() { EnsureLoaded(); return static_cast<int32>(sCompletedLevels.count()); }
bool GameProgress::IsLevelUnlocked(int32 level) { EnsureLoaded(); return level >= 1 && level <= sHighestUnlockedLevel; }
bool GameProgress::IsLevelCompleted(int32 level) { EnsureLoaded(); return level >= 1 && level <= kLevelCount && sCompletedLevels.test(level - 1); }
int32 GameProgress::GetLevelHighScore(int32 level) { EnsureLoaded(); return level >= 1 && level <= kLevelCount ? sLevelHighScores[level - 1] : 0; }
int32 GameProgress::GetTotalScore() { EnsureLoaded(); return sTotalScore; }
int32 GameProgress::GetSessionsPlayed() { EnsureLoaded(); return sSessionsPlayed; }
int32 GameProgress::GetLevelsCompleted() { EnsureLoaded(); return sLevelsCompleted; }
int32 GameProgress::GetBricksDestroyed() { EnsureLoaded(); return sBricksDestroyed; }
int32 GameProgress::GetBallsLost() { EnsureLoaded(); return sBallsLost; }
int32 GameProgress::GetPowersCollected() { EnsureLoaded(); return sPowersCollected; }
int32 GameProgress::GetHighestCombo() { EnsureLoaded(); return sHighestCombo; }
int32 GameProgress::GetShockwavesFired() { EnsureLoaded(); return sShockwavesFired; }
int32 GameProgress::GetPlayedSeconds() { EnsureLoaded(); return sPlayedSeconds; }

const GameProgress::Achievement& GameProgress::GetAchievement(int32 index)
{
	return kAchievements[std::clamp(index, 0, kAchievementCount - 1)];
}

bool GameProgress::IsAchievementUnlocked(int32 index)
{
	EnsureLoaded();
	return index >= 0 && index < kAchievementCount && (sAchievementMask & (uint64{ 1 } << index)) != 0;
}

int32 GameProgress::GetUnlockedAchievementCount()
{
	EnsureLoaded();
	return static_cast<int32>(std::popcount(sAchievementMask));
}

int32 GameProgress::ConsumeRecentlyUnlockedAchievement()
{
	EnsureLoaded();
	if (sRecentlyUnlockedAchievements.empty())
		return -1;

	const int32 index = sRecentlyUnlockedAchievements.front();
	sRecentlyUnlockedAchievements.pop_front();
	return index;
}

void GameProgress::EnsureLoaded()
{
	if (!sLoaded)
		Load();
}

void GameProgress::EvaluateAchievements()
{
	if (sBricksDestroyed >= 1) UnlockAchievement(0);
	if (sBricksDestroyed >= 100) UnlockAchievement(1);
	if (sHighestCombo >= 8) UnlockAchievement(2);
	if (sPowersCollected >= 1) UnlockAchievement(3);
	if (sShockwavesFired >= 1) UnlockAchievement(4);
	if (sCompletedLevels.count() >= 1) UnlockAchievement(5);
	if (sTotalScore >= 100000) UnlockAchievement(6);
	if (sCompletedLevels.count() >= 25) UnlockAchievement(7);
	if (sPlayedSeconds >= 3600) UnlockAchievement(8);
	if (sCompletedLevels.count() >= kLevelCount) UnlockAchievement(9);
}

void GameProgress::UnlockAchievement(int32 index)
{
	if (index < 0 || index >= kAchievementCount)
		return;

	const uint64 bit = uint64{ 1 } << index;
	if ((sAchievementMask & bit) != 0)
		return;

	sAchievementMask |= bit;
	sRecentlyUnlockedAchievements.push_back(index);
	Save();
}

void GameProgress::Save()
{
	const std::filesystem::path path = SavePath();
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);

	std::ofstream stream(path, std::ios::trunc);
	if (!stream)
	{
		Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[GameProgress] Could not write '{}'.", path.string()));
		return;
	}

	stream << "version=2\n";
	stream << "highestUnlocked=" << sHighestUnlockedLevel << '\n';
	stream << "completedLevels=" << sCompletedLevels.to_string() << '\n';
	stream << "achievements=" << sAchievementMask << '\n';
	stream << "totalScore=" << sTotalScore << '\n';
	for (int32 index = 0; index < kLevelCount; ++index)
		stream << "levelScore" << index + 1 << '=' << sLevelHighScores[index] << '\n';
	stream << "sessionsPlayed=" << sSessionsPlayed << '\n';
	stream << "levelsCompleted=" << sLevelsCompleted << '\n';
	stream << "bricksDestroyed=" << sBricksDestroyed << '\n';
	stream << "ballsLost=" << sBallsLost << '\n';
	stream << "powersCollected=" << sPowersCollected << '\n';
	stream << "highestCombo=" << sHighestCombo << '\n';
	stream << "shockwavesFired=" << sShockwavesFired << '\n';
	stream << "playedSeconds=" << sPlayedSeconds << '\n';
}
