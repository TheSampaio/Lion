#pragma once

#include <Lion/Lion.h>

class GameLayer : public Lion::Layer
{
public:
	void OnCreate() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDetach() override;

protected:
	void OnEvent(Lion::Event& event) override;
	bool OnEventWindowResize(const Lion::EventWindowResize& event);

private:
	Lion::Reference<Lion::CameraOrthographic> mCamera;
};
