#pragma once

namespace Lion
{
    class Application;

	class Graphics
	{
    public:
        friend Application;

    protected:
        static Graphics* sInstance;

        static void New();
        static void Delete();

        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics&) = delete;

    private:
        Graphics() = default;

        static bool Initialize();
        static void ClearBuffers();
        static void SwapBuffers();
	};
}
