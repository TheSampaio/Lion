#pragma once

#include <Lion/Render/Camera.h>

namespace Lion
{
	class CameraOrthographic : public Camera
	{
	public:
		LION_API CameraOrthographic();

		virtual LION_API void SetViewMatrix(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
		virtual LION_API void SetProjectionMatrix(float32 left, float32 right, float32 bottom, float32 top);
		virtual LION_API void SetZoomLevel(float32 zoomLevel);

		virtual LION_API void OnResize(float32 width, float32 height) override;
		virtual LION_API void OnUsage() override;

	private:
		struct
		{
			float32 Left;
			float32 Right;
			float32 Bottom;
			float32 Top;

		} mBounds;

		struct
		{
			float32 Width;
			float32 Height;
			float32 Zoom;

		} mViewport;

		const glm::vec3 mFront = glm::vec3(0.0f, 0.0f, -1.0f);

		void RecalculateBounds();
	};
}
