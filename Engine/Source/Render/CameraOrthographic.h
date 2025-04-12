#pragma once

#include "Camera.h"

namespace Lion
{
	class CameraOrthographic : public Camera
	{
	public:
		LION_API CameraOrthographic();

		virtual LION_API void SetViewMatrix(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
		virtual LION_API void SetProjectionMatrix(float left, float right, float bottom, float top);

		virtual LION_API void OnResize(int width, int height) override;
		virtual LION_API void OnUsage() override;

	private:
		struct
		{
			float Left;
			float Right;
			float Bottom;
			float Top;

		} mBounds;

		float mZoomLevel;
	};
}
