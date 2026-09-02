#pragma once

#include <Lion/Lion.h>

class CoreLayer : public Lion::Layer
{
public:
	void OnAttach() override;
	void OnEvent(Lion::Event& event) override;

private:
	bool OnEventWindowClose(const Lion::EventWindowClose& event);
};
