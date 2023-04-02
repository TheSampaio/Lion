#include "Core.h"
#include "Debug.h"

void Debug::Log(ELogMode Mode, const char* Text, bool BreakLine, bool Pause)
{
	const char* TextMode = (Mode == None) ? "\0" : (Mode == Information) ? "[INFO] " : (Mode == Warning) ? "[WARN] " : "[ERRO] ";

	(BreakLine) ? std::cout << TextMode << Text << std::endl : std::cout << TextMode << Text;

	if (Pause)
		char Stop = std::getchar();
}
