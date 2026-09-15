#pragma once

#include <Lion/Lion.h>

#include <array>
#include <bitset>
#include <deque>

// Persistent Brickout progression and gameplay statistics. The save file lives in the user's
// application-data directory so packaged builds never attempt to write beside the executable.
class GameProgress final
{
public:
	struct Achievement
	{
		const Lion::char8* id;
		const Lion::char8* title;
		const Lion::char8* description;
		const Lion::char8* icon;
	};

	static constexpr Lion::int32 kLevelCount = 100;
	static constexpr Lion::int32 kAchievementCount = 10;

	static void Load();
	static void StartSession();
	static void CompleteLevel(Lion::int32 level, Lion::int32 score, Lion::float32 playedSeconds);
	static void EndAttempt(Lion::int32 level, Lion::int32 score, Lion::float32 playedSeconds);
	static void RegisterBrickDestroyed();
	static void RegisterBallLost();
	static void RegisterPowerCollected();
	static void RegisterCombo(Lion::int32 combo);
	static void RegisterShockwave();

	static Lion::int32 GetHighestUnlockedLevel();
	static Lion::int32 GetCompletedLevelCount();
	static bool IsLevelUnlocked(Lion::int32 level);
	static bool IsLevelCompleted(Lion::int32 level);
	static Lion::int32 GetLevelHighScore(Lion::int32 level);
	static Lion::int32 GetTotalScore();
	static Lion::int32 GetSessionsPlayed();
	static Lion::int32 GetLevelsCompleted();
	static Lion::int32 GetBricksDestroyed();
	static Lion::int32 GetBallsLost();
	static Lion::int32 GetPowersCollected();
	static Lion::int32 GetHighestCombo();
	static Lion::int32 GetShockwavesFired();
	static Lion::int32 GetPlayedSeconds();
	static const Achievement& GetAchievement(Lion::int32 index);
	static bool IsAchievementUnlocked(Lion::int32 index);
	static Lion::int32 GetUnlockedAchievementCount();
	static Lion::int32 ConsumeRecentlyUnlockedAchievement();

private:
	static inline bool sLoaded = false;
	static inline Lion::int32 sHighestUnlockedLevel = 1;
	static inline std::bitset<kLevelCount> sCompletedLevels;
	static inline Lion::int32 sTotalScore = 0;
	static inline std::array<Lion::int32, kLevelCount> sLevelHighScores{};
	static inline Lion::int32 sSessionsPlayed = 0;
	static inline Lion::int32 sLevelsCompleted = 0;
	static inline Lion::int32 sBricksDestroyed = 0;
	static inline Lion::int32 sBallsLost = 0;
	static inline Lion::int32 sPowersCollected = 0;
	static inline Lion::int32 sHighestCombo = 1;
	static inline Lion::int32 sShockwavesFired = 0;
	static inline Lion::int32 sPlayedSeconds = 0;
	static inline Lion::uint64 sAchievementMask = 0;
	static inline std::deque<Lion::int32> sRecentlyUnlockedAchievements;

	static void EnsureLoaded();
	static void EvaluateAchievements();
	static void UnlockAchievement(Lion::int32 index);
	static void Save();
};
