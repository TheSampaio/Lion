#pragma once

#include <Lion/Lion.h>

#include <array>

// Persistent Brickout progression and gameplay statistics. The save file lives in the user's
// application-data directory so packaged builds never attempt to write beside the executable.
class GameProgress final
{
public:
	static constexpr Lion::int32 kLevelCount = 5;

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

private:
	static inline bool sLoaded = false;
	static inline Lion::int32 sHighestUnlockedLevel = 1;
	static inline Lion::int32 sCompletedMask = 0;
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

	static void EnsureLoaded();
	static void Save();
};
