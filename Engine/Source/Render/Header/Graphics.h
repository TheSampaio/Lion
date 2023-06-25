#pragma once

namespace owl
{
	class OWL_API Graphics
	{
	public:

	protected:
		Graphics();
		~Graphics();

		// Deletes copy constructor and assigment operator
		Graphics(const Graphics&) = delete;
		Graphics operator=(const Graphics&) = delete;

		// Gets the class's static reference
		static Graphics& GetInstance() { static Graphics s_Instance; return s_Instance; }

	private:

	};
}
