#pragma once

#include <Lion/Core.h>

class WindowLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnCreate() override;

	void OnEvent(Lion::Event& event) override;

private:
	bool OnEventWindowClose(const Lion::EventWindowClose& event);
};
