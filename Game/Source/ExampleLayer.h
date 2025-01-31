#pragma once

#include <Lion/Core.h>

class ExampleLayer : public Lion::Layer
{
public:
	void OnEvent(Lion::Event& event) override;

private:
	bool OnEventInputKeyboardPress(const Lion::EventInputKeyboardPress& event);
	bool OnEventInputKeyboardRelease(const Lion::EventInputKeyboardRelease& event);
	bool OnEventInputKeyboardRepeat(const Lion::EventInputKeyboardRepeat& event);

	bool OnEventInputMouseMove(const Lion::EventInputMouseMove& event);
	bool OnEventInputMousePress(const Lion::EventInputMousePress& event);
	bool OnEventInputMouseRelease(const Lion::EventInputMouseRelease& event);
	bool OnEventInputMouseScroll(const Lion::EventInputMouseScroll& event);

	bool OnEventWindowFocusEnter(const Lion::EventWindowFocusEnter& event);
	bool OnEventWindowFocusExit(const Lion::EventWindowFocusExit& event);
	bool OnEventWindowResize(const Lion::EventWindowResize& event);
};

