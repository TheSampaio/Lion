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
	void GetKeyPressed();
	void GetKeyReleased();
	void GetKeyTapped();

	// Friends
	friend class Application;

private:
	// Main methods
	void ProcessEvents();
};

#endif // !OWL_INPUT_H
