#ifndef GLF3D_UTILS_H
#define GLF3D_UTILS_H

namespace Debug {

    namespace Log {

        inline void Info(const char* Text, bool BreakLine = true){ (BreakLine) ? std::cout << "[INFO] " << Text << std::endl : std::cout << "[INFO] " << Text; }
        inline void Warning(const char* Text, bool BreakLine = true){ (BreakLine) ? std::cout << "[WARN] " << Text << std::endl : std::cout << "[WANR] " << Text; }
        inline void Error(const char* Text, bool BreakLine = true){ (BreakLine) ? std::cout << "[ERRO] " << Text << std::endl : std::cout << "[ERRO] " << Text; }
    }
}

namespace Types {
    typedef int Int;
    typedef unsigned int Uint;

    typedef long int Lint;
    typedef long long int LLint;

    typedef std::string String;
}

#endif
