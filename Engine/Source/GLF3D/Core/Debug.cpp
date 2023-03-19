#include "Core.h"
#include "Debug.h"

void Debug::Log(ELogMode Mode, const char* Text, bool BreakLine)
{
	const char* TextMode = (Mode == Information) ? "[INFO] " : (Mode == Warning) ? "[WARN] " : "[ERRO] ";
	(BreakLine) ? std::cout << TextMode << Text << std::endl : std::cout << TextMode << Text;

	if (Mode == Error)
		char Stop = std::getchar();
}
