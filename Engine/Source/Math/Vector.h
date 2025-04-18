#pragma once

namespace Lion
{
    class Renderer;
    class Sprite;

    class Vector
    {
    public:
        LION_API Vector();
        LION_API Vector(float32 value);
        LION_API Vector(float32 x, float32 y);
        LION_API Vector(float32 x, float32 y, float32 z);

        LION_API float32 GetX() const;
        LION_API float32 GetY() const;
        LION_API float32 GetZ() const;

        LION_API void SetX(float32 value);
        LION_API void SetY(float32 value);
        LION_API void SetZ(float32 value);

        LION_API float32 Magnitude() const;
        LION_API Vector Normalized() const;

        LION_API Vector operator+(float32 value) const;
        LION_API Vector operator-(float32 value) const;
        LION_API Vector operator*(float32 value) const;
        LION_API Vector operator/(float32 value) const;

        LION_API Vector operator-() const;

        LION_API Vector operator+(const Vector& other) const;
        LION_API Vector operator-(const Vector& other) const;

        LION_API float32 Dot(const Vector& other) const;
        LION_API Vector Cross(const Vector& other) const;

        friend Sprite;
        friend Renderer;

    private:
        glm::vec3 mData;
    };
}
