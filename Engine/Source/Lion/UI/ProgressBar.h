#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	// A value bar that can be display-only or pointer-adjustable and renders its fill from the left edge.
	class ProgressBar final : public Component
	{
	public:
		LION_API ProgressBar();
		LION_API ~ProgressBar();

		LION_API float32 GetValue() const { return mValue; }
		LION_API bool WasChanged() const { return mChanged; }
		LION_API void SetValue(float32 value);
		LION_API void SetRange(float32 minimum, float32 maximum);

		LION_API void OnUpdateBegin() override;
		bool UpdatesWhenPaused() const override { return true; }
		LION_API void OnRender() override;
		LION_API void Reflect(Reflector& reflector) override;

	private:
		std::string mBackgroundPath;
		std::string mFillPath;
		Vector mSize{ 280.0f, 22.0f, 0.0f };
		Vector mBackgroundColor{ 0.12f, 0.34f, 0.48f };
		Vector mFillColor{ 0.12f, 0.82f, 1.0f };
		float32 mMinimum = 0.0f;
		float32 mMaximum = 1.0f;
		float32 mValue = 0.5f;
		int32 mOrder = 105;
		bool mInteractable = false;
		bool mChanged = false;

		Scope<Sprite> mBackground;
		Scope<Sprite> mFill;
		std::string mBuiltBackgroundPath;
		std::string mBuiltFillPath;

		bool ContainsPointer() const;
		void SetValueFromPointer();
	};
}
