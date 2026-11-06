#include "Engine.h"
#include "ProgressBar.h"

#include <Lion/Core/Asset.h>
#include <Lion/Core/Input.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Render/Sprite.h>

namespace Lion
{
	ProgressBar::ProgressBar() = default;

	ProgressBar::~ProgressBar() = default;

	void ProgressBar::SetValue(float32 value)
	{
		mValue = std::clamp(value, mMinimum, mMaximum);
	}

	void ProgressBar::SetRange(float32 minimum, float32 maximum)
	{
		mMinimum = std::min(minimum, maximum);
		mMaximum = std::max(minimum, maximum);
		SetValue(mValue);
	}

	void ProgressBar::OnUpdateBegin()
	{
		mChanged = false;
		if (mInteractable && Input::GetMouseButtonPress(0) && ContainsPointer())
			SetValueFromPointer();
	}

	void ProgressBar::OnRender()
	{
		if (mBackgroundPath != mBuiltBackgroundPath)
		{
			mBuiltBackgroundPath = mBackgroundPath;
			const Reference<Texture> texture = mBackgroundPath.empty()
				? nullptr : Asset::LoadTexture(mBackgroundPath, mBackgroundPath);
			mBackground = texture ? MakeScope<Sprite>(texture) : nullptr;
		}
		if (mFillPath != mBuiltFillPath)
		{
			mBuiltFillPath = mFillPath;
			const Reference<Texture> texture = mFillPath.empty()
				? nullptr : Asset::LoadTexture(mFillPath, mFillPath);
			mFill = texture ? MakeScope<Sprite>(texture) : nullptr;
		}

		Entity& owner = GetOwner();
		const Vector2 ownerScale = owner.GetWorldScale();
		if (mBackground)
		{
			const Size textureSize = mBackground->GetSize();
			mBackground->SetOrder(mOrder);
			mBackground->SetColor(mBackgroundColor);
			mBackground->Draw(owner.GetWorldPosition(), Vector(0.0f, 0.0f, owner.GetWorldRotation()),
				Vector(mSize.x / textureSize.width * ownerScale.x,
					mSize.y / textureSize.height * ownerScale.y, 1.0f), owner.GetId());
		}

		const float32 range = std::max(mMaximum - mMinimum, 0.0001f);
		const float32 ratio = std::clamp((mValue - mMinimum) / range, 0.0f, 1.0f);
		if (!mFill || ratio <= 0.0f)
			return;

		const Size textureSize = mFill->GetSize();
		const Vector2 position = owner.GetWorldPosition();
		const float32 width = mSize.x * ratio * ownerScale.x;
		const float32 left = position.x - mSize.x * ownerScale.x * 0.5f;
		mFill->SetOrder(mOrder + 1);
		mFill->SetColor(mFillColor);
		mFill->Draw(Vector(left + width * 0.5f, position.y), Vector(0.0f, 0.0f, owner.GetWorldRotation()),
			Vector(width / textureSize.width, mSize.y / textureSize.height * ownerScale.y, 1.0f), owner.GetId());
	}

	void ProgressBar::Reflect(Reflector& reflector)
	{
		reflector.FieldAsset("Background", mBackgroundPath);
		reflector.FieldAsset("Fill", mFillPath);
		reflector.Field("Size", mSize);
		reflector.Field("Background Color", mBackgroundColor);
		reflector.Field("Fill Color", mFillColor);
		reflector.Field("Minimum", mMinimum);
		reflector.Field("Maximum", mMaximum);
		reflector.Field("Value", mValue);
		reflector.Field("Order", mOrder);
		reflector.Field("Interactable", mInteractable);
	}

	bool ProgressBar::ContainsPointer() const
	{
		const Vector2 pointer = Input::GetPointerPosition();
		const Vector2 position = GetOwner().GetWorldPosition();
		const Vector2 scale = GetOwner().GetWorldScale();
		return std::abs(pointer.x - position.x) <= std::abs(mSize.x * scale.x) * 0.5f
			&& std::abs(pointer.y - position.y) <= std::abs(mSize.y * scale.y) * 0.5f;
	}

	void ProgressBar::SetValueFromPointer()
	{
		const Vector2 position = GetOwner().GetWorldPosition();
		const float32 width = std::max(std::abs(mSize.x * GetOwner().GetWorldScale().x), 1.0f);
		const float32 ratio = std::clamp((Input::GetPointerPosition().x - (position.x - width * 0.5f)) / width,
			0.0f, 1.0f);
		const float32 value = mMinimum + (mMaximum - mMinimum) * ratio;
		if (std::abs(value - mValue) <= 0.0001f)
			return;
		mValue = value;
		mChanged = true;
	}

	LION_REGISTER_COMPONENT(ProgressBar)
}
