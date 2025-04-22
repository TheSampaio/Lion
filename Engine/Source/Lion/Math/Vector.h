#pragma once

#include <Lion/Base/Platform.h>
#include <Lion/Math/Sigma.h>

// This class was adapted from the "OmegaMath" library to serve as the core math utility in the Lion engine.
namespace Lion
{
    /**
     * @struct Vector
     * @brief A structure representing a 2D or 3D vector with basic operations.
     */
    struct Vector
    {
        float32 x, y, z; ///< X, Y, and Z components of the vector.

        /**
         * @brief Constructs a vector with the zero value for all components.
         * @param value The value for all components.
         */
        LION_API Vector() : x(0.0f), y(0.0f), z(0.0f) {}

        /**
         * @brief Constructs a vector with the same value for all components.
         * @param Same value for all components.
         */
        LION_API Vector(float32 value) : x(value), y(value), z(value) {}
        LION_API Vector(int32 value) : x(static_cast<float32>(value)), y(static_cast<float32>(value)), z(static_cast<float32>(value)) {}

        /**
         * @brief Constructs a 2D vector with Z set to 0.
         * @param x X component.
         * @param y Y component.
         */
        LION_API Vector(float32 x, float32 y) : x(x), y(y), z(0.0f) {}
        LION_API Vector(int32 x, int32 y) : x(static_cast<float32>(x)), y(static_cast<float32>(y)), z(0.0f) {}

        /**
         * @brief Constructs a 3D vector.
         * @param x X component.
         * @param y Y component.
         * @param z Z component.
         */
        LION_API Vector(float32 x, float32 y, float32 z) : x(x), y(y), z(z) {}
        LION_API Vector(int32 x, int32 y, int32 z) : x(static_cast<float32>(x)), y(static_cast<float32>(y)), z(static_cast<float32>(z)) {}

        /**
         * @brief Computes the magnitude (length) of the vector.
         * @return The magnitude of the vector.
         */
        LION_API float32 GetMagnitude() const { return Sigma::Sqrt(Sigma::Pow(x, 2.0f) + Sigma::Pow(y, 2.0f) + Sigma::Pow(z, 2.0f)); }

        /**
         * @brief Returns a normalized version of the vector.
         * @return A unit vector pointing in the same direction.
         */
        LION_API Vector GetNormalized() const { return *this / GetMagnitude(); }

        /**
         * @brief Computes the dot product of two vectors.
         * @param left First vector.
         * @param right Second vector.
         * @return The dot product as a float32.
         */
        static LION_API float32 Dot(const Vector& left, const Vector& right) { return left.x * right.x + left.y * right.y + left.z * right.z; }

        /**
         * @brief Computes the cross product of two vectors.
         * @param left First vector.
         * @param right Second vector.
         * @return A new vector representing the cross product.
         */
        static LION_API Vector Cross(const Vector& left, const Vector& right)
        {
            return Vector(
                left.y * right.z - left.z * right.y,
                left.z * right.x - left.x * right.z,
                left.x * right.y - left.y * right.x
            );
        }

        // Scalar Operations

        LION_API Vector operator+(float32 value) const { return Vector(x + value, y + value, z + value); }
        LION_API Vector operator-(float32 value) const { return Vector(x - value, y - value, z - value); }
        LION_API Vector operator*(float32 value) const { return Vector(x * value, y * value, z * value); }
        LION_API Vector operator/(float32 value) const { return Vector(x / value, y / value, z / value); }

        // Negation Operator

        LION_API Vector operator-() const { return *this * -1.0f; }

        // Vector Addition & Subtraction

        LION_API Vector operator+(const Vector& vector) const { return Vector(x + vector.x, y + vector.y, z + vector.z); }
        LION_API Vector operator-(const Vector& vector) const { return Vector(x - vector.x, y - vector.y, z - vector.z); }
    };
}
