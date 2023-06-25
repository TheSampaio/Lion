#pragma once

namespace owl
{
	class OWL_API Cursor
	{
	public:
		// === SET methods ======

		// Sets the window's icon
		static void SetCursor(const uint Cursor);

		static void SetVisibility(bool bShow) { ShowCursor(bShow); }

		// === Friends ======
		friend class Window;

	protected:
		Cursor();

		// Deletes copy constructor and assigment operator
		Cursor(const Cursor&) = delete;
		Cursor operator=(const Cursor&) = delete;

		// Gets the class's static reference
		static Cursor& GetInstance() { static Cursor s_Instance; return s_Instance; }

	private:
		HCURSOR m_hCursor;
	};
}