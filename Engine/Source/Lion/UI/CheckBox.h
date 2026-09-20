#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	// A reusable screen-space checkbox with pointer interaction and independently skinnable frame and mark.
	class CheckBox final : public Component
	{
	public:
		LION_API CheckBox();
		LION_API ~CheckBox();

		LION_API bool IsChecked() const { return mChecked; }
		LION_API bool WasChanged() const { return mChanged; }
		LION_API bool IsHovered() const { return mHovered; }
		LION_API void SetChecked(bool checked);
		LION_API void SetInteractable(bool interactable);

		LION_API void OnEnable() override;
		LION_API void OnDisable() override;
		LION_API void OnUpdateBegin() override;
		bool UpdatesWhenPaused() const override { return true; }
		LION_API void OnRender() override;
		LION_API void Reflect(Reflector& reflector) override;

	private:
		std::string mBackgroundPath;
		std::string mCheckmarkPath;
		Vector mSize{ 30.0f, 30.0f, 0.0f };
		Vector mNormalColor{ 0.12f, 0.82f, 1.0f };
		Vector mHoveredColor{ 1.0f, 0.22f, 0.72f };
		Vector mCheckmarkColor{ 1.0f, 1.0f, 1.0f };
		int32 mOrder = 100;
		bool mChecked = false;
		bool mInteractable = true;
		bool mHovered = false;
		bool mPointerWasPressed = false;
		bool mPressStartedInside = false;
		bool mChanged = false;

		Scope<Sprite> mBackground;
		Scope<Sprite> mCheckmark;
		std::string mBuiltBackgroundPath;
		std::string mBuiltCheckmarkPath;

		bool ContainsPointer() const;
		void ResetInteraction();
	};
}
