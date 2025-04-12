#include "Engine.h"
#include "CameraOrthographic.h"

#include "../Core/Window.h"
#include "../Core/Log.h"

namespace Lion
{
	CameraOrthographic::CameraOrthographic()
	{
		mBounds = { -1.0f, 1.0f, -1.0f, 1.0f };
		mZoomLevel = 1.0f;
	}

	void CameraOrthographic::SetViewMatrix(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
	{
		mView = glm::lookAt(position, target, up);
	}

	void CameraOrthographic::SetProjectionMatrix(float left, float right, float bottom, float top)
	{
		mBounds.Left = left;
		mBounds.Right = right;
		mBounds.Bottom = bottom;
		mBounds.Top = top;

		mProjection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
	}

	void CameraOrthographic::OnResize(int width, int height)
	{
		mAspectRatio = static_cast<float>(width) / static_cast<float>(height);

		mBounds.Left = -mAspectRatio * mZoomLevel;
		mBounds.Right = mAspectRatio * mZoomLevel;
		mBounds.Bottom = -mZoomLevel;
		mBounds.Top = mZoomLevel;

		SetProjectionMatrix(mBounds.Left, mBounds.Right, mBounds.Bottom, mBounds.Top);
	}

	void CameraOrthographic::OnUsage()
	{
		SetViewMatrix(mPosition, mPosition + mRotation, mUp);

		mBounds.Left = -mAspectRatio * mZoomLevel;
		mBounds.Right = mAspectRatio * mZoomLevel;
		mBounds.Bottom = -mZoomLevel;
		mBounds.Top = mZoomLevel;

		SetProjectionMatrix(mBounds.Left, mBounds.Right, mBounds.Bottom, mBounds.Top);
	}
}
