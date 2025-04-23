#pragma once

// This class was adapted from the "OmegaMath" library to serve as the core math utility in the Lion engine.
namespace Lion
{
    /**
     * @class Sigma
     * @brief A static math utility class providing common mathematical functions.
     */
    class Sigma
    {
    public:
        /**
         * @brief Returns the value of PI.
         * @return The mathematical constant PI as a float32.
         */
        static constexpr LION_API float32 GetPI() { return 3.1415927f; }

        /**
         * @brief Computes the cosine of a given angle.
         * @param value Angle in radians.
         * @return The cosine of the given angle.
         */
        static LION_API float32 Cos(float32 value) { return std::cosf(value); }

        /**
         * @brief Computes the power of a number.
         * @param value Base value.
         * @param exponent Exponent value.
         * @return The result of raising value to the exponent power.
         */
        static LION_API float32 Pow(float32 value, float32 exponent) { return std::powf(value, exponent); }

        /**
         * @brief Converts degrees to radians.
         * @param degrees Angle in degrees.
         * @return The angle converted to radians.
         */
        static LION_API float32 Rad(float32 degrees) { return degrees * (GetPI() / 180.0f); }

        /**
         * @brief Computes the sine of a given angle.
         * @param value Angle in radians.
         * @return The sine of the given angle.
         */
        static LION_API float32 Sin(float32 value) { return std::sinf(value); }

        /**
         * @brief Computes the square root of a number.
         * @param value Input number.
         * @return The square root of the input number.
         */
        static LION_API float32 Sqrt(float32 value) { return std::sqrtf(value); }

        /**
         * @brief Computes the tangent of a given angle.
         * @param value Angle in radians.
         * @return The tangent of the given angle.
         */
        static LION_API float32 Tan(float32 value) { return std::tanf(value); }

    private:
        Sigma() = delete;
        Sigma(const Sigma&) = delete;
        Sigma& operator=(const Sigma&) = delete;
    };
}
