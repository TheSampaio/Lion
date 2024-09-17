#pragma once

#ifdef LN_PLATFORM_WIN

extern Lion::Application* Lion::Main();

int main(int argc, const char* argv[])
{
	auto application = Lion::Main();
	application->Run();
	delete application;
}

#endif // !LN_PLATFORM_WIN
