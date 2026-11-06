#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	// A screen-space button for Widget Assemblies. It owns its visual background, exposes interaction
	// state to gameplay components and leaves the action itself in game code.
	class Button final : public Component
	{
	public:
		LION_API Button();
		LION_API ~Button();

		LION_API bool WasClicked() const { return mClicked; }
		LION_API bool IsHovered() const { return mHovered; }
		LION_API bool IsSelected() const { return mSelected; }
		LION_API void SetSelected(bool selected);
		LION_API void SetInteractable(bool interactable);
		LION_API void SetBackgroundPath(const std::string& path) { mBackgroundPath = path; }
		LION_API void SetSize(const Vector& size) { mSize = size; }
		LION_API void SetOrder(int32 order) { mOrder = order; }
		LION_API void SetColors(const Vector& normal, const Vector& selected,
			const Vector& hovered, const Vector& pressed);
		LION_API void CopyVisualStyleTo(Button& target) const;

		LION_API void OnEnable() override;
		LION_API void OnDisable() override;
		LION_API void OnUpdateBegin() override;
		bool UpdatesWhenPaused() const override { return true; }
		LION_API void OnRender() override;
		LION_API void Reflect(Reflector& reflector) override;

	private:
		std::string mBackgroundPath;
		Vector mSize{ 360.0f, 56.0f, 0.0f };
		Vector mNormalColor{ 0.48f, 0.48f, 0.48f };
		Vector mSelectedColor{ 1.0f, 1.0f, 1.0f };
		Vector mHoveredColor{ 0.82f, 0.82f, 0.82f };
		Vector mPressedColor{ 0.65f, 0.65f, 0.65f };
		int32 mOrder = 90;
		bool mInteractable = true;
		bool mSelected = false;
		bool mHovered = false;
		bool mPointerWasPressed = false;
		bool mPressStartedInside = false;
		bool mClicked = false;

		Scope<Sprite> mBackground;
		std::string mBuiltBackgroundPath;
		Vector mVisualColor = mNormalColor;

		bool ContainsPointer() const;
		void RefreshVisual();
		void ResetInteraction();
	};
}
