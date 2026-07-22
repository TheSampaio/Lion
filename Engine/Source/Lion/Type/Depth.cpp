#include "Engine.h"
#include "Depth.h"

namespace Lion
{
    // Spaced out, so a sprite can be slipped between two named layers without renaming them.
    const int32 Depth::Back = 0;
    const int32 Depth::Lower = 10;
    const int32 Depth::Middle = 20;
    const int32 Depth::Upper = 30;
    const int32 Depth::Front = 40;
    const int32 Depth::Overlay = 50;
}
