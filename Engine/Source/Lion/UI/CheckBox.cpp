#include "Engine.h"
#include "CheckBox.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Input.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	CheckBox::CheckBox() = default;

	CheckBox::~CheckBox() = default;

	void CheckBox::SetChecked(bool checked)
	{
		mChecked = checked;
	}

	void CheckBox::SetInteractable(bool interactable)
	{
		mInteractable = interactable;
		if (!interactable)
			ResetInteraction();
	}

	void CheckBox::OnEnable()
	{
		ResetInteraction();
	}

	void CheckBox::OnDisable()
	{
		ResetInteraction();
	}

	void CheckBox::OnUpdateBegin()
	{
		mChanged = false;
		if (!mInteractable)
		{
			ResetInteraction();
			return;
		}

		mHovered = ContainsPointer();
		const bool pointerPressed = Input::GetMouseButtonPress(0);
		if (pointerPressed && !mPointerWasPressed)
			mPressStartedInside = mHovered;
		else if (!pointerPressed && mPointerWasPressed)
		{
			if (mPressStartedInside && mHovered)
			{
				mChecked = !mChecked;
				mChanged = true;
			}
			mPressStartedInside = false;
		}
		mPointerWasPressed = pointerPressed;
	}

	void CheckBox::OnRender()
	{
		if (mBackgroundPath != mBuiltBackgroundPath)
		{
			mBuiltBackgroundPath = mBackgroundPath;
			const Reference<Texture> texture = mBackgroundPath.empty()
				? nullptr : Asset::LoadTexture(mBackgroundPath, mBackgroundPath);
			mBackground = texture ? MakeScope<Sprite>(texture) : nullptr;
		}
		if (mCheckmarkPath != mBuiltCheckmarkPath)
		{
			mBuiltCheckmarkPath = mCheckmarkPath;
			const Reference<Texture> texture = mCheckmarkPath.empty()
				? nullptr : Asset::LoadTexture(mCheckmarkPath, mCheckmarkPath);
			mCheckmark = texture ? MakeScope<Sprite>(texture) : nullptr;
		}

		Entity& owner = GetOwner();
		const Vector2 scale = owner.GetWorldScale();
		const Vector drawScale(mSize.x * scale.x, mSize.y * scale.y, 1.0f);
		if (mBackground)
		{
			const Size textureSize = mBackground->GetSize();
			mBackground->SetOrder(mOrder);
			mBackground->SetColor(mHovered ? mHoveredColor : mNormalColor);
			mBackground->Draw(owner.GetWorldPosition(), Vector(0.0f, 0.0f, owner.GetWorldRotation()),
				Vector(drawScale.x / textureSize.width, drawScale.y / textureSize.height, 1.0f), owner.GetId());
		}
		if (mChecked && mCheckmark)
		{
			const Size textureSize = mCheckmark->GetSize();
			mCheckmark->SetOrder(mOrder + 1);
			mCheckmark->SetColor(mCheckmarkColor);
			mCheckmark->Draw(owner.GetWorldPosition(), Vector(0.0f, 0.0f, owner.GetWorldRotation()),
				Vector(drawScale.x / textureSize.width, drawScale.y / textureSize.height, 1.0f), owner.GetId());
		}
	}

	void CheckBox::Reflect(Reflector& reflector)
	{
		reflector.FieldAsset("Background", mBackgroundPath);
		reflector.FieldAsset("Checkmark", mCheckmarkPath);
		reflector.Field("Size", mSize);
		reflector.Field("Normal Color", mNormalColor);
		reflector.Field("Hovered Color", mHoveredColor);
		reflector.Field("Checkmark Color", mCheckmarkColor);
		reflector.Field("Order", mOrder);
		reflector.Field("Checked", mChecked);
		reflector.Field("Interactable", mInteractable);
	}

	bool CheckBox::ContainsPointer() const
	{
		const Vector2 pointer = Input::GetPointerPosition();
		const Vector2 position = GetOwner().GetWorldPosition();
		const Vector2 scale = GetOwner().GetWorldScale();
		return std::abs(pointer.x - position.x) <= std::abs(mSize.x * scale.x) * 0.5f
			&& std::abs(pointer.y - position.y) <= std::abs(mSize.y * scale.y) * 0.5f;
	}

	void CheckBox::ResetInteraction()
	{
		mHovered = false;
		mPointerWasPressed = false;
		mPressStartedInside = false;
		mChanged = false;
	}

	LION_REGISTER_COMPONENT(CheckBox)
}
