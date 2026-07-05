#pragma once

namespace Lion
{
    class Application;
    class GraphicsContext;

	class Graphics
	{
    public:
        // Enables or disables vertical synchronization (applied on the next present).
        static LION_API void SetVerticalSynchronization(bool enable);

        friend Application;

    protected:
        static Graphics* sInstance;

        static void New();
        static void Delete();

        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics&) = delete;

    private:
        Scope<GraphicsContext> mContext;

        bool mIsVerticalSynchronizedEnabled;
        bool mIsVerticalSynchronizationDirty;  // Deferred apply: the swap interval is set once per change.

        Graphics();

        static bool Initialize();
        static void ClearBuffers();
        static void SwapBuffers();
        static void ShowSpecification();
	};
}
