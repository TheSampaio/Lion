#pragma once

namespace Lion
{
	// Char type
	using char8 = char;            // 8-bit signed char

	using uchar8 = unsigned char;  // 8-bit unsigned char
	using uchar16 = char16_t;      // 16-bit unsigned Unicode char
	using uchar32 = char32_t;      // 32-bit unsigned Unicode char

	// Signed integer types
	using int8 = int8_t;           // 8-bit signed integer
	using int16 = int16_t;         // 16-bit signed integer
	using int32 = int32_t;         // 32-bit signed integer
	using int64 = int64_t;         // 64-bit signed integer

	// Unsigned integer types
	using uint8 = uint8_t;         // 8-bit unsigned integer
	using uint16 = uint16_t;       // 16-bit unsigned integer
	using uint32 = uint32_t;       // 32-bit unsigned integer
	using uint64 = uint64_t;       // 64-bit unsigned integer

	// Floating-point types
	using float32 = float;         // 32-bit signed float
	using float64 = double;        // 64-bit signed float

	// Byte type
	using byte = uint8;            // Byte alias (for raw memory or buffer manipulation)
}
