#include "Engine.h"
#include "ComboBox.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Render/TextRenderer.h>
#include <Lion/UI/Button.h>

namespace Lion
{
	void ComboBox::SetSelectedIndex(int32 index)
	{
		if (mValues.empty())
			return;
		mSelectedIndex = std::clamp(index, 0, static_cast<int32>(mValues.size()) - 1);
		RefreshLabel();
		RefreshPopup();
	}

	void ComboBox::SelectRelative(int32 direction)
	{
		if (mValues.empty())
			return;
		const int32 count = static_cast<int32>(mValues.size());
		const int32 next = (mSelectedIndex + (direction < 0 ? -1 : 1) + count) % count;
		if (next == mSelectedIndex)
			return;
		mSelectedIndex = next;
		mChanged = true;
		RefreshLabel();
		RefreshPopup();
	}

	void ComboBox::SetOpen(bool open)
	{
		mOpen = open;
		RefreshLabel();
		RefreshPopup();
	}

	void ComboBox::OnAwake()
	{
		mButton = GetOwner().GetComponent<Button>();
		mText = GetOwner().GetComponent<TextRenderer>();
		BuildOptions();
	}

	void ComboBox::OnDisable()
	{
		SetOpen(false);
	}

	void ComboBox::OnUpdate()
	{
		mChanged = false;
		if (mButton && mButton->WasClicked())
			SetOpen(!mOpen);

		if (!mOpen)
			return;

		for (int32 index = 0; index < static_cast<int32>(mOptionButtons.size()); ++index)
		{
			if (!mOptionButtons[index] || !mOptionButtons[index]->WasClicked())
				continue;
			mSelectedIndex = index;
			mChanged = true;
			SetOpen(false);
			return;
		}
	}

	void ComboBox::OnDestroy()
	{
		for (const Reference<Entity>& entity : mOptionEntities)
			if (entity) entity->RemoveFromScene();
		mOptionEntities.clear();
		mOptionButtons.clear();
	}

	void ComboBox::Reflect(Reflector& reflector)
	{
		reflector.Field("Options", mOptions);
		reflector.Field("Prefix", mPrefix);
		reflector.FieldAsset("Font", mFontPath);
		reflector.Field("Option Height", mOptionHeight);
		reflector.Field("Selected Index", mSelectedIndex);
		reflector.Field("Popup Order", mPopupOrder);
	}

	void ComboBox::BuildOptions()
	{
		mValues.clear();
		size_t start = 0;
		while (start <= mOptions.size())
		{
			const size_t separator = mOptions.find('|', start);
			mValues.push_back(mOptions.substr(start, separator == std::string::npos
				? std::string::npos : separator - start));
			if (separator == std::string::npos)
				break;
			start = separator + 1;
		}
		if (mValues.size() == 1 && mValues.front().empty())
			mValues.clear();

		mSelectedIndex = mValues.empty() ? 0
			: std::clamp(mSelectedIndex, 0, static_cast<int32>(mValues.size()) - 1);
		const Reference<Scene> scene = GetOwner().GetScene();
		if (!scene || !mButton || !mText)
		{
			RefreshLabel();
			return;
		}

		for (int32 index = 0; index < static_cast<int32>(mValues.size()); ++index)
		{
			Reference<Entity> option = MakeReference<Entity>();
			option->SetName(LION_FORMAT_TEXT("{} Option {}", GetOwner().GetName(), index + 1));
			option->GetTransform()->SetPosition(Vector2(0.0f, -(index + 1) * mOptionHeight));
			option->SetParent(&GetOwner(), false);
			Button* optionButton = option->AddComponent<Button>();
			mButton->CopyVisualStyleTo(*optionButton);
			optionButton->SetOrder(mPopupOrder);
			TextRenderer* optionText = option->AddComponent<TextRenderer>();
			mText->CopyStyleTo(*optionText);
			optionText->SetFontPath(mFontPath.empty() ? "Fonts/Arcade.lnfont" : mFontPath);
			optionText->SetText(mValues[index]);
			optionText->SetCentered(true);
			optionText->SetOffset(Vector2());
			optionText->SetOrder(mPopupOrder + 1);
			scene->Add(option);
			mOptionEntities.push_back(option);
			mOptionButtons.push_back(optionButton);
		}
		RefreshLabel();
		RefreshPopup();
	}

	void ComboBox::RefreshLabel()
	{
		if (!mText)
			return;
		const std::string value = mValues.empty() ? std::string() : mValues[mSelectedIndex];
		mText->SetText(LION_FORMAT_TEXT("{}{}{}", mPrefix, mPrefix.empty() ? "" : "     ", value));
	}

	void ComboBox::RefreshPopup()
	{
		for (int32 index = 0; index < static_cast<int32>(mOptionEntities.size()); ++index)
		{
			const bool visible = mOpen;
			mOptionEntities[index]->SetEnabled(visible);
			mOptionEntities[index]->SetVisible(visible);
			if (mOptionButtons[index])
				mOptionButtons[index]->SetSelected(index == mSelectedIndex);
		}
	}

	LION_REGISTER_COMPONENT(ComboBox)
}
