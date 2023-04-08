#ifndef OWL_WINDOW_H
#define OWL_WINDOW_H

// Enumerate window's display modes
enum EDisplayMode
{
	Fullscreen = 0,
	Windowed
};

class OWL_API Window
{
public:
	Window();
	~Window();

	// Delete copy constructor and assignment operator
	Window(const Window&) = delete;
	Window operator=(const Window&) = delete;

	// Main methods
	bool Close();

	// Get methods
	inline GLFWwindow*& GetId()                               { return m_Id; }
	inline GLFWmonitor*& GetMonitor()                         { return m_Monitor; }
	inline std::string& GetTitle()                            { return m_Title; }
	inline EDisplayMode GetDisplayMode()                      { return m_DisplayMode; }
	inline std::array<unsigned short, 2> GetSize()            { return m_Size; }
	inline std::array<unsigned short, 2> GetCenter()          { return m_Center; }
	inline std::array<int, 2> GetScreen()                     { return m_Screen; }
	inline std::array<int, 2> GetPosition()                   { return m_Position; }
	inline std::array<unsigned short, 2> GetOpenGLVersion()   { return m_OpenGLVersion; }
	inline std::array<unsigned short, 3> GetBackgroundColor() { return m_BackgroundColor; }

	// Set methods
	inline void SetTitle(const char* Title)                                                       { m_Title = Title; }
	inline void SetDisplayMode(EDisplayMode DisplayMode)                                          { m_DisplayMode = DisplayMode; }
	inline void SetMaximize(bool Maximize)                                                        { m_bMaximize = Maximize; }
	inline void SetSize(unsigned short Width, unsigned short Height)                              { m_Size = { Width, Height }; }
	inline void SetOpenGLVersion(unsigned short Major, unsigned short Minor)                      { m_OpenGLVersion = { Major, Minor }; }
	inline void SetBackgroundColor(unsigned short Red, unsigned short Green, unsigned short Blue) { m_BackgroundColor = { Red, Green, Blue }; }

	// Friend classes
	friend class Application;

private:
	// Attributes
	GLFWwindow* m_Id;
	GLFWmonitor* m_Monitor;

	std::string m_Title;
	EDisplayMode m_DisplayMode;

	bool m_bMaximize;

	std::array<unsigned short, 2> m_Size;
	std::array<unsigned short, 2> m_Center;
	std::array<int, 2> m_Screen;
	std::array<int, 2> m_Position;
	std::array<unsigned short, 3> m_BackgroundColor;
	std::array<unsigned short, 2> m_OpenGLVersion;

	// Main methods
	bool Create();
	void ProcessEvents();

	// Static attribures
	static class Debug& s_Debug;
};

#endif
