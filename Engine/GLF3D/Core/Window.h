#ifndef GLF3D_WINDOW_H
#define GLF3D_WINDOW_H

#include "Core.h"

class GLF3D_API Window
{
public:
    Window();
    ~Window();

    // Main methods
    bool Create();

    inline bool Close()         { return glfwWindowShouldClose(m_Id); }
    inline void ClearBuffers()  { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); }
    inline void ProcessEvents() { glfwPollEvents(); }      // Processes window's events
    inline void SwapBuffers()   { glfwSwapBuffers(m_Id); } // Swaps window's buffers

    // Get methods
    inline GLFWwindow*& GetId()                        { return m_Id; }
    inline std::array<unsigned int, 2> GetSize() const { return m_Size; }
    
    // Set methods
    void SetOpenGLVersion(unsigned int Major = 3, unsigned int Minor = 3);

    inline void SetVerticalSynchronization(bool State)           { glfwSwapInterval(State); }
    inline void SetTitle(std::string Title)                      { m_Title = Title; }
    inline void SetSize(unsigned int Width, unsigned int Heigth) { m_Size = {Width, Heigth}; }
    inline void SetBackgroundColor(float R, float G, float B)    { m_BackgroundColor = { R, G, B }; }

private:
    GLFWwindow* m_Id;

    std::string m_Title;
    std::array<unsigned int, 2> m_Size;
    std::array<unsigned int, 2> m_GLVersion;
    std::array<float, 3> m_BackgroundColor;
};

#endif
