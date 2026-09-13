#include "Engine.h"
#include "Button.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Input.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	Button::~Button() = default;

	void Button::SetSelected(bool selected)
	{
		if (mSelected == selected)
			return;

		mSelected = selected;
		RefreshVisual();
	}

	void Button::SetInteractable(bool interactable)
	{
		if (mInteractable == interactable)
			return;

		mInteractable = interactable;
		ResetInteraction();
		RefreshVisual();
	}

	void Button::OnEnable()
	{
		ResetInteraction();
		RefreshVisual();
	}

	void Button::OnDisable()
	{
		ResetInteraction();
		RefreshVisual();
	}

	void Button::OnUpdateBegin()
	{
		mClicked = false;

		if (!mInteractable)
		{
			ResetInteraction();
			RefreshVisual();
			return;
		}

		mHovered = ContainsPointer();
		const bool pointerPressed = Input::GetMouseButtonPress(0);

		if (pointerPressed && !mPointerWasPressed)
			mPressStartedInside = mHovered;
		else if (!pointerPressed && mPointerWasPressed)
		{
			mClicked = mPressStartedInside && mHovered;
			mPressStartedInside = false;
		}

		mPointerWasPressed = pointerPressed;
		RefreshVisual();
	}

	void Button::OnRender()
	{
		if (mBackgroundPath != mBuiltBackgroundPath)
		{
			mBuiltBackgroundPath = mBackgroundPath;
			const Reference<Texture> texture = mBackgroundPath.empty()
				? nullptr : Asset::LoadTexture(mBackgroundPath, mBackgroundPath);
			mBackground = texture ? MakeScope<Sprite>(texture) : nullptr;
		}

		if (!mBackground)
			return;

		const Size textureSize = mBackground->GetSize();
		if (textureSize.width <= 0.0f || textureSize.height <= 0.0f)
			return;

		Entity& owner = GetOwner();
		const Vector2 ownerScale = owner.GetWorldScale();
		mBackground->SetOrder(mOrder);
		mBackground->SetColor(mVisualColor);
		mBackground->Draw(
			owner.GetWorldPosition(),
			Vector(0.0f, 0.0f, owner.GetWorldRotation()),
			Vector(
				mSize.x / textureSize.width * ownerScale.x,
				mSize.y / textureSize.height * ownerScale.y,
				1.0f),
			owner.GetId());
	}

	void Button::Reflect(Reflector& reflector)
	{
		reflector.FieldAsset("Background", mBackgroundPath);
		reflector.Field("Size", mSize);
		reflector.Field("Normal Color", mNormalColor);
		reflector.Field("Selected Color", mSelectedColor);
		reflector.Field("Hovered Color", mHoveredColor);
		reflector.Field("Pressed Color", mPressedColor);
		reflector.Field("Order", mOrder);
		reflector.Field("Interactable", mInteractable);
	}

	bool Button::ContainsPointer() const
	{
		const Vector2 pointer = Input::GetPointerPosition();
		const Vector2 position = GetOwner().GetWorldPosition();
		const Vector2 scale = GetOwner().GetWorldScale();
		const float32 radians = GetOwner().GetWorldRotation() * 3.14159265359f / 180.0f;
		const float32 cosine = std::cos(radians);
		const float32 sine = std::sin(radians);
		const float32 deltaX = pointer.x - position.x;
		const float32 deltaY = pointer.y - position.y;
		const float32 localX = deltaX * cosine + deltaY * sine;
		const float32 localY = -deltaX * sine + deltaY * cosine;

		return std::abs(localX) <= std::abs(mSize.x * scale.x) * 0.5f
			&& std::abs(localY) <= std::abs(mSize.y * scale.y) * 0.5f;
	}

	void Button::RefreshVisual()
	{
		if (mPointerWasPressed && mPressStartedInside && mHovered)
			mVisualColor = mPressedColor;
		else if (mHovered && mInteractable)
			mVisualColor = mHoveredColor;
		else if (mSelected)
			mVisualColor = mSelectedColor;
		else
			mVisualColor = mNormalColor;
	}

	void Button::ResetInteraction()
	{
		mHovered = false;
		mPointerWasPressed = false;
		mPressStartedInside = false;
		mClicked = false;
	}

	LION_REGISTER_COMPONENT(Button)
}
