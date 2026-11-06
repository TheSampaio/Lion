#pragma once

#include <Lion/Logic/Component.h>

namespace Lion
{
	class Button;
	class Entity;
	class TextRenderer;

	// A drop-down selector. Options are authored as a pipe-separated string so reflection and scene
	// serialization remain language-agnostic while runtime callers work with stable integer indices.
	class ComboBox final : public Component
	{
	public:
		LION_API ComboBox() = default;
		LION_API ~ComboBox() = default;

		LION_API int32 GetSelectedIndex() const { return mSelectedIndex; }
		LION_API bool WasChanged() const { return mChanged; }
		LION_API bool IsOpen() const { return mOpen; }
		LION_API void SetSelectedIndex(int32 index);
		LION_API void SetPrefix(const std::string& prefix) { mPrefix = prefix; RefreshLabel(); }
		LION_API void SelectRelative(int32 direction);
		LION_API void SetOpen(bool open);

		LION_API void OnAwake() override;
		LION_API void OnDisable() override;
		LION_API void OnUpdate() override;
		LION_API void OnDestroy() override;
		bool UpdatesWhenPaused() const override { return true; }
		LION_API void Reflect(Reflector& reflector) override;
		LION_REQUIRES("Button", "TextRenderer")

	private:
		std::string mOptions;
		std::string mPrefix;
		std::string mFontPath;
		float32 mOptionHeight = 40.0f;
		int32 mSelectedIndex = 0;
		int32 mPopupOrder = 180;
		bool mOpen = false;
		bool mChanged = false;

		Button* mButton = nullptr;
		TextRenderer* mText = nullptr;
		std::vector<std::string> mValues;
		std::vector<Reference<Entity>> mOptionEntities;
		std::vector<Button*> mOptionButtons;

		void BuildOptions();
		void RefreshLabel();
		void RefreshPopup();
	};
}
