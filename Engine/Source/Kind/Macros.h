#pragma once

#define LN_LOG_FORMAT(message, ...) std::format(message, __VA_ARGS__)

#define LN_CREATE_OPENGL_CONTEXT() \
    do { \
        static bool __glad_initialized = false; \
        if (!__glad_initialized) { \
            if (!gladLoadGL()) { \
                Log::Console(ELogMode::Error, "[Graphics] GLAD initialization failed on client side. Check OpenGL context setup."); \
                break; \
            } \
            __glad_initialized = true; \
            Log::Console(ELogMode::Success, "[Graphics] GLAD initialized successfully."); \
            glViewport(0, 0, \
                static_cast<GLsizei>(Window::GetSize()[0]), \
                static_cast<GLsizei>(Window::GetSize()[1]) \
            ); \
            Log::Console(ELogMode::Information, LN_LOG_FORMAT("[Graphics] Renderer : {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)))); \
            Log::Console(ELogMode::Information, LN_LOG_FORMAT("[Graphics] Vendor   : {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)))); \
            Log::Console(ELogMode::Information, LN_LOG_FORMAT("[Graphics] OpenGL   : {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)))); \
        } \
    } while (0)
