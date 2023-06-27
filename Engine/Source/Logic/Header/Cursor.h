#pragma once

namespace owl
{
	class Cursor
	{
	public:
		// === SET methods ======

		// Sets the cursor's icon
		static void OWL_API SetCursor(const uint Cursor);

		// Sets the cursor's visibility
		static void OWL_API SetVisibility(bool bShow) { ShowCursor(bShow); }

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
		// Attributes
		HCURSOR m_hCursor;
	};
}