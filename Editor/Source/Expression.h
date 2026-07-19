#pragma once

#include <optional>
#include <string>

#include <Lion/Lion.h>

// Arithmetic in a number field, the way every editor worth typing into has it: a box that takes 2+3,
// 100/4 or (8-2)*1.5 and keeps the answer. A field that only takes digits makes the person holding a
// calculator the one doing the engine's arithmetic.
namespace Expression
{
	// Evaluates the text, or nothing when it is not an expression this understands. Supports + - * /,
	// parentheses, unary signs and decimals; anything else — a stray letter, a division by zero, an
	// unclosed bracket — is refused rather than guessed at, so the field keeps what it had.
	std::optional<Lion::float32> Evaluate(const std::string& text);

	// Whether the text is worth evaluating at all: a plain number is not, and letting the widget's own
	// parse handle those keeps every ordinary edit on the path it always took.
	bool IsExpression(const std::string& text);
}
