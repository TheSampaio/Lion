#include "Engine.h"
#include "Vector.h"

namespace Lion
{
    Vector::Vector() : mData(0.0f) {}
    Vector::Vector(float32 value) : mData(value) {}
    Vector::Vector(float32 x, float32 y) : mData(x, y, 0.0f) {}
    Vector::Vector(float32 x, float32 y, float32 z) : mData(x, y, z) {}

    float32 Vector::GetX() const { return mData.x; }
    float32 Vector::GetY() const { return mData.y; }
    float32 Vector::GetZ() const { return mData.z; }

    void Vector::SetX(float32 value) { mData.x = value; }
    void Vector::SetY(float32 value) { mData.y = value; }
    void Vector::SetZ(float32 value) { mData.z = value; }

    float32 Vector::Magnitude() const { return glm::length(mData); }

    Vector Vector::Normalized() const
    {
        auto norm = glm::normalize(mData);
        return Vector(norm.x, norm.y, norm.z);
    }

    Vector Vector::operator+(float32 value) const
    {
        auto result = mData + glm::vec3(value);
        return Vector(result.x, result.y, result.z);
    }

    Vector Vector::operator-(float32 value) const
    {
        auto result = mData - glm::vec3(value);
        return Vector(result.x, result.y, result.z);
    }

    Vector Vector::operator*(float32 value) const
    {
        auto result = mData * value;
        return Vector(result.x, result.y, result.z);
    }

    Vector Vector::operator/(float32 value) const
    {
        auto result = mData / value;
        return Vector(result.x, result.y, result.z);
    }

    Vector Vector::operator-() const
    {
        auto result = -mData;
        return Vector(result.x, result.y, result.z);
    }

    Vector Vector::operator+(const Vector& other) const
    {
        auto result = mData + other.mData;
        return Vector(result.x, result.y, result.z);
    }

    Vector Vector::operator-(const Vector& other) const
    {
        auto result = mData - other.mData;
        return Vector(result.x, result.y, result.z);
    }

    float32 Vector::Dot(const Vector& other) const
    {
        return glm::dot(mData, other.mData);
    }

    Vector Vector::Cross(const Vector& other) const
    {
        auto result = glm::cross(mData, other.mData);
        return Vector(result.x, result.y, result.z);
    }
}
