#pragma once

#include <Lion/Lion.h>

enum class GameText
{
	Play,
	Credits,
	Settings,
	Quit,
	Back,
	Sound,
	Resolution,
	VSync,
	Quality,
	Bloom,
	Vignette,
	MotionBlur,
	CameraShake,
	ColorMode,
	Language,
	On,
	Off,
	Low,
	Medium,
	High,
	None,
	Protanopia,
	Deuteranopia,
	Tritanopia,
	Paused,
	Resume,
	MainMenu,
	CircuitSuspended,
	PlayAgain,
	TotalScore,
	CircuitClear,
	SystemFailure,
	Score,
	Balls,
	Level,
	ExtraBall,
	Multiball,
	PressAny,
	Continue,
	LevelSelect,
	Statistics,
	ControlHints,
	Locked,
	Completed,
	HighScore,
	Combo,
	Shockwave,
	ShockwaveReady,
	ShockwaveFired,
	Bomb,
	WidePaddle,
	DuplicatePaddle,
	Sessions,
	LevelsCleared,
	BricksDestroyed,
	BallsLost,
	PowersCollected,
	HighestCombo,
	Shockwaves,
	PlayTime,
	Count,
};

// Session-wide presentation and accessibility settings shared by every authored scene.
class GameSettings final
{
public:
	static constexpr Lion::int32 kSettingCount = 11;

	static void Change(Lion::int32 setting, Lion::int32 direction);
	static void Apply(Lion::PostProcessingComponent& postProcessing);
	static std::string Label(Lion::int32 setting);
	static const char* Text(GameText text);
	static bool HasCameraShake() { return sCameraShake; }
	static bool HasControlHints() { return sControlHints; }

private:
	static inline Lion::int32 sResolution = 1;
	static inline Lion::int32 sQuality = 2;
	static inline Lion::int32 sColorMode = 0;
	static inline Lion::int32 sLanguage = 0;
	static inline bool sSound = true;
	static inline bool sVSync = false;
	static inline bool sBloom = true;
	static inline bool sVignette = true;
	static inline bool sMotionBlur = true;
	static inline bool sCameraShake = true;
	static inline bool sControlHints = true;

	static void ApplyWindow();
	static Lion::int32 Wrap(Lion::int32 value, Lion::int32 count);
};
