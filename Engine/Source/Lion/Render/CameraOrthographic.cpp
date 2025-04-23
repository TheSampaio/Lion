#include "Engine.h"
#include "CameraOrthographic.h"

#include <Lion/Core/Log.h>
#include <Lion/Core/Window.h>

namespace Lion
{
    CameraOrthographic::CameraOrthographic()
    {
        const auto& size = Window::GetSize();

        mViewport.Width = static_cast<float32>(size.width);
        mViewport.Height = static_cast<float32>(size.height);
        mViewport.Zoom = 1.0f;
        mAspectRatio = mViewport.Width / mViewport.Height;

        RecalculateBounds();
    }

    void CameraOrthographic::SetViewMatrix(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        mView = glm::lookAt(position, target, up);
    }

    void CameraOrthographic::SetProjectionMatrix(float32 left, float32 right, float32 bottom, float32 top)
    {
        mBounds.Left = left;
        mBounds.Right = right;
        mBounds.Bottom = bottom;
        mBounds.Top = top;

        mProjection = glm::ortho(left, right, bottom, top, -1.0f, 0.0f);
    }

    void CameraOrthographic::SetZoomLevel(float32 zoomLevel)
    {
        mViewport.Zoom = zoomLevel;
        RecalculateBounds();
    }

    void CameraOrthographic::OnResize(float32 width, float32 height)
    {
        mViewport.Width = width;
        mViewport.Height = height;
        mAspectRatio = width / height;

        RecalculateBounds();
    }

    void CameraOrthographic::OnUsage()
    {
        SetViewMatrix(mPosition, mPosition + mFront, mUp);
        SetProjectionMatrix(mBounds.Left, mBounds.Right, mBounds.Bottom, mBounds.Top);
    }

    void CameraOrthographic::RecalculateBounds()
    {
        float32 halfWidth = (mViewport.Width / 2.0f) * mViewport.Zoom;
        float32 halfHeight = (mViewport.Height / 2.0f) * mViewport.Zoom;

        mBounds.Left = -halfWidth;
        mBounds.Right = halfWidth;
        mBounds.Bottom = -halfHeight;
        mBounds.Top = halfHeight;

        SetProjectionMatrix(mBounds.Left, mBounds.Right, mBounds.Bottom, mBounds.Top);
    }
}
