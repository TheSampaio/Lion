#include "MainMenu.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void MainMenu::OnAwake()
{
	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> prompt = scene->FindEntity("Menu Prompt");
	const Reference<Entity> options = scene->FindEntity("Menu Options");
	const Reference<Entity> detail = scene->FindEntity("Menu Detail");
	mPrompt = prompt.get();
	mOptions = options.get();
	mDetail = detail.get();
	mOptionsText = mOptions ? mOptions->GetComponent<TextRenderer>() : nullptr;
	mDetailText = mDetail ? mDetail->GetComponent<TextRenderer>() : nullptr;
	mSoundEnabled = Audio::GetBusVolume(AudioBus::Master) > 0.0f;
	Window::SetBackgroundColor(0.015f, 0.02f, 0.045f);
	ShowState(State::Attract);
}

void MainMenu::OnUpdate()
{
	// Play is F5 in the editor. Requiring one completely released frame prevents that same F5 from
	// becoming the title screen's AnyKey and skipping the menu before the player sees it.
	if (!mInputArmed)
	{
		if (Input::GetKeyRelease(KeyCode::AnyKey))
			mInputArmed = true;

		return;
	}

	if (mState == State::Attract)
	{
		if (Input::GetKeyTap(KeyCode::AnyKey))
			ShowState(State::Menu);

		return;
	}

	if (mState == State::Credits || mState == State::Settings)
	{
		if (mState == State::Settings
			&& (Input::GetActionTap("menu_left") || Input::GetActionTap("menu_right")
				|| Input::GetActionTap("menu_confirm")))
		{
			mSoundEnabled = !mSoundEnabled;
			Audio::SetBusVolume(AudioBus::Master, mSoundEnabled ? 1.0f : 0.0f);
			ShowState(State::Settings);
		}
		else if (Input::GetActionTap("menu_back"))
			ShowState(State::Menu);

		return;
	}

	if (Input::GetActionTap("menu_up"))
	{
		mSelection = (mSelection + 3) % 4;
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_down"))
	{
		mSelection = (mSelection + 1) % 4;
		RefreshMenu();
	}
	else if (Input::GetActionTap("menu_confirm"))
		ActivateSelection();
}

void MainMenu::ShowState(State state)
{
	mState = state;

	if (mPrompt)
		mPrompt->SetVisible(state == State::Attract);
	if (mOptions)
		mOptions->SetVisible(state == State::Menu);
	if (mDetail)
		mDetail->SetVisible(state == State::Credits || state == State::Settings);

	if (state == State::Menu)
		RefreshMenu();
	else if (state == State::Credits && mDetailText)
		mDetailText->SetText("CREDITS\n\nBRICKOUT\nBUILT WITH LION ENGINE\n\nPRESS ESC TO RETURN");
	else if (state == State::Settings && mDetailText)
		mDetailText->SetText(LION_FORMAT_TEXT(
			"SETTINGS\n\nSOUND: {}\n\nLEFT OR RIGHT TO CHANGE\nPRESS ESC TO RETURN",
			mSoundEnabled ? "ON" : "OFF"));
}

void MainMenu::RefreshMenu()
{
	if (!mOptionsText)
		return;

	static const char8* labels[] = { "PLAY", "CREDITS", "SETTINGS", "QUIT" };
	std::string text;

	for (int32 index = 0; index < 4; ++index)
	{
		if (index > 0)
			text += '\n';

		text += index == mSelection
			? LION_FORMAT_TEXT("> {} <", labels[index])
			: LION_FORMAT_TEXT("  {}  ", labels[index]);
	}

	mOptionsText->SetText(text);
}

void MainMenu::ActivateSelection()
{
	switch (mSelection)
	{
		case 0: GameRules::StartNewGame(); break;
		case 1: ShowState(State::Credits); break;
		case 2: ShowState(State::Settings); break;
		case 3:
			if (Application::IsEditor())
				Log::Console(LogLevel::Information, "[Game] Quit is available in the standalone player.");
			else
				Window::RequestClose();
			break;
	}
}

LION_REGISTER_COMPONENT(MainMenu)
