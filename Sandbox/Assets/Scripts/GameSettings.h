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
	GameOver,
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
	enum class Category
	{
		Graphics,
		Sound,
		Accessibility,
		Controls,
		Count,
	};

	static constexpr Lion::int32 kSettingCount = 12;

	static void Change(Lion::int32 setting, Lion::int32 direction);
	static void SetValue(Lion::int32 setting, Lion::int32 value);
	static Lion::int32 GetValue(Lion::int32 setting);
	static bool IsToggle(Lion::int32 setting);
	static void Apply(Lion::PostProcessingComponent& postProcessing);
	static void ApplyAudio();
	static std::string Label(Lion::int32 setting);
	static Category GetCategory(Lion::int32 setting);
	static const char* CategoryName(Category category);
	static const char* Text(GameText text);
	static bool HasCameraShake() { return sCameraShake; }
	static bool HasControlHints() { return sControlHints; }

private:
	static inline Lion::int32 sResolution = 1;
	static inline Lion::int32 sQuality = 2;
	static inline Lion::int32 sColorMode = 0;
	static inline Lion::int32 sLanguage = 0;
	static inline Lion::int32 sSfxVolume = 8;
	static inline Lion::int32 sMusicVolume = 6;
	static inline bool sVSync = false;
	static inline bool sBloom = true;
	static inline bool sVignette = true;
	static inline bool sMotionBlur = true;
	static inline bool sCameraShake = true;
	static inline bool sControlHints = true;

	static void ApplyWindow();
	static Lion::int32 Wrap(Lion::int32 value, Lion::int32 count);
};
