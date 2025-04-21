#pragma once

#include <Lion/Base/Platform.h>

namespace Lion
{
    class Application;

	class Graphics
	{
    public:
        static LION_API void SetVerticalSynchronization(bool enable) { sInstance->mIsVerticalSynchronizedEnabled = enable; }

        friend Application;

    protected:
        static Graphics* sInstance;

        static void New();
        static void Delete();

        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics&) = delete;

    private:
        bool mIsVerticalSynchronizedEnabled;

        Graphics();

        static bool Initialize();
        static void ClearBuffers();
        static void SwapBuffers();
	};
}
