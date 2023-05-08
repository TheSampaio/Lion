#include "Core.h"
#include "Include/Debug.h"

void Debug::ILog(ELogMode Mode, const char* Text, bool Pause, bool BreakLine)
{
	// Set log status
	const char* TextMode = (Mode == None) ? "\0" : (Mode == Information) ? "[INFO] " : (Mode == Warning) ? "[WARN] " : "[ERRO] ";

	// Write on console with or without break line after
	(BreakLine) ? std::cout << TextMode << Text << std::endl : std::cout << TextMode << Text;

	// Pause application
	if (Pause)
		char Stop = std::getchar();
}
