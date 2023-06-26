#pragma once

namespace owl
{
	class OWL_API Cursor
	{
	public:
		// === SET methods ======

		// Sets the cursor's icon
		static void SetCursor(const uint Cursor);

		// Sets the cursor's visibility
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
		// Attributes
		HCURSOR m_hCursor;
	};
}