#pragma once

namespace Lion
{
    // Named draw orders, low to high, for laying sprites over one another: a background sits at Back, a
    // character at Front, an effect on top of everything at Overlay. Higher draws later, so higher is in
    // front — the order a Sprite Renderer takes, and the order the Hierarchy falls back to when two share
    // one. It used to be a Z on the transform; a 2D engine has no Z, so depth is what it always was — an
    // order, not a place.
    struct Depth
    {
        static const LION_API int32 Back;
        static const LION_API int32 Lower;
        static const LION_API int32 Middle;
        static const LION_API int32 Upper;
        static const LION_API int32 Front;
        static const LION_API int32 Overlay;
    };
}
