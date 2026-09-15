#include "GameProgress.h"

#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace Lion;

namespace
{
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
		else if (key == "completedMask") sCompletedMask = ParseValue(value, 0);
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

	sHighestUnlockedLevel = std::clamp(sHighestUnlockedLevel, 1, kLevelCount);
	sCompletedMask &= (1 << kLevelCount) - 1;
	sHighestCombo = std::clamp(sHighestCombo, 1, 16);
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
	sCompletedMask |= 1 << (level - 1);
	sHighestUnlockedLevel = std::max(sHighestUnlockedLevel, std::min(level + 1, kLevelCount));
	sLevelsCompleted++;
	sTotalScore += std::max(score, 0);
	sLevelHighScores[level - 1] = std::max(sLevelHighScores[level - 1], score);
	sPlayedSeconds += std::max(static_cast<int32>(std::round(playedSeconds)), 0);
	Save();
}

void GameProgress::EndAttempt(int32 level, int32 score, float32 playedSeconds)
{
	EnsureLoaded();
	level = std::clamp(level, 1, kLevelCount);
	sTotalScore += std::max(score, 0);
	sLevelHighScores[level - 1] = std::max(sLevelHighScores[level - 1], score);
	sPlayedSeconds += std::max(static_cast<int32>(std::round(playedSeconds)), 0);
	Save();
}

void GameProgress::RegisterBrickDestroyed()
{
	EnsureLoaded();
	sBricksDestroyed++;
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
}

void GameProgress::RegisterCombo(int32 combo)
{
	EnsureLoaded();
	sHighestCombo = std::max(sHighestCombo, std::clamp(combo, 1, 16));
}

void GameProgress::RegisterShockwave()
{
	EnsureLoaded();
	sShockwavesFired++;
}

int32 GameProgress::GetHighestUnlockedLevel() { EnsureLoaded(); return sHighestUnlockedLevel; }
int32 GameProgress::GetCompletedLevelCount() { EnsureLoaded(); return std::popcount(static_cast<uint32>(sCompletedMask)); }
bool GameProgress::IsLevelUnlocked(int32 level) { EnsureLoaded(); return level >= 1 && level <= sHighestUnlockedLevel; }
bool GameProgress::IsLevelCompleted(int32 level) { EnsureLoaded(); return level >= 1 && level <= kLevelCount && (sCompletedMask & (1 << (level - 1))) != 0; }
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

void GameProgress::EnsureLoaded()
{
	if (!sLoaded)
		Load();
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

	stream << "version=1\n";
	stream << "highestUnlocked=" << sHighestUnlockedLevel << '\n';
	stream << "completedMask=" << sCompletedMask << '\n';
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
