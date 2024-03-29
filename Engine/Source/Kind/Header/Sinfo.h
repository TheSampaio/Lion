#pragma once

namespace Lion
{
    struct Sinfo
    {
        float X, Y;
        float Scale;
        float Depth;
        float Rotation;
        uint  Width;
        uint  Height;
        ID3D11ShaderResourceView* Texture;
    };
}