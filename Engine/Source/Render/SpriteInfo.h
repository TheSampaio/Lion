#pragma once

#include "../Math/Vector.h"

namespace Lion
{
    struct SpriteInfo
    {
        Vector position;
        Vector rotation;
        Vector scale;
        uint32 width;
        uint32 height;
        uint32 texture;
    };
}
