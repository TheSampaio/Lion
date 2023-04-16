#ifndef OWL_INPUT_H
#define OWL_INPUT_H

class OWL_API Input
{
public:
	Input() {};

	// Delete copy constructor and assignment operator
	Input(const Input&) = delete;
	Input operator=(const Input&) = delete;

	// Main methods
	bool GetKeyPressed(int KeyCode);
	bool GetKeyReleased(int KeyCode);
	bool GetKeyTapped(int KeyCode);

	// Friends
	friend class Application;

private:
	// Main methods
	void ProcessEvents();

	// Static Attributes
	static bool m_bPressed;
};

#endif // !OWL_INPUT_H
