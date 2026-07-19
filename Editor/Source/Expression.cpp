#include "EditorPch.h"
#include "Expression.h"

#include <cmath>

using namespace Lion;

namespace Expression
{
	namespace
	{
		// A recursive-descent parser over the text: terms bind tighter than sums, parentheses bind
		// tightest, and a failure anywhere unwinds to nothing rather than to a half-read number. It is
		// the smallest shape that reads arithmetic the way a person writes it.
		class Parser
		{
		public:
			explicit Parser(const std::string& text) : mText(text) {}

			std::optional<float32> Run()
			{
				const std::optional<float32> value = Sum();

				SkipBlanks();

				// Trailing anything means the text was not the expression it looked like.
				return (value && mCursor == mText.size()) ? value : std::nullopt;
			}

		private:
			void SkipBlanks()
			{
				while (mCursor < mText.size() && std::isspace(static_cast<unsigned char>(mText[mCursor])))
					++mCursor;
			}

			bool Take(char8 character)
			{
				SkipBlanks();

				if (mCursor >= mText.size() || mText[mCursor] != character)
					return false;

				++mCursor;
				return true;
			}

			// sum := product (('+' | '-') product)*
			std::optional<float32> Sum()
			{
				std::optional<float32> left = Product();

				while (left)
				{
					if (Take('+'))
					{
						const std::optional<float32> right = Product();
						if (!right) return std::nullopt;
						left = *left + *right;
					}
					else if (Take('-'))
					{
						const std::optional<float32> right = Product();
						if (!right) return std::nullopt;
						left = *left - *right;
					}
					else
					{
						break;
					}
				}

				return left;
			}

			// product := unary (('*' | '/') unary)*
			std::optional<float32> Product()
			{
				std::optional<float32> left = Unary();

				while (left)
				{
					if (Take('*'))
					{
						const std::optional<float32> right = Unary();
						if (!right) return std::nullopt;
						left = *left * *right;
					}
					else if (Take('/'))
					{
						const std::optional<float32> right = Unary();

						// Dividing by zero is a question with no answer, not an answer of infinity: the
						// field keeps what it had rather than taking one.
						if (!right || *right == 0.0f) return std::nullopt;
						left = *left / *right;
					}
					else
					{
						break;
					}
				}

				return left;
			}

			// unary := ('+' | '-')* primary
			std::optional<float32> Unary()
			{
				if (Take('-'))
				{
					const std::optional<float32> value = Unary();
					return value ? std::optional<float32>(-*value) : std::nullopt;
				}

				if (Take('+'))
					return Unary();

				return Primary();
			}

			// primary := number | '(' sum ')'
			std::optional<float32> Primary()
			{
				if (Take('('))
				{
					const std::optional<float32> value = Sum();
					return (value && Take(')')) ? value : std::nullopt;
				}

				SkipBlanks();

				// strtof reads as much of a number as there is and says where it stopped, which is exactly
				// the two answers this needs — the value, and whether it read anything at all.
				const char8* begin = mText.c_str() + mCursor;
				char8* end = nullptr;
				const float32 value = std::strtof(begin, &end);

				if (end == begin || !std::isfinite(value))
					return std::nullopt;

				mCursor += static_cast<size_t>(end - begin);
				return value;
			}

			const std::string& mText;
			size_t mCursor = 0;
		};
	}

	std::optional<float32> Evaluate(const std::string& text)
	{
		return Parser(text).Run();
	}

	bool IsExpression(const std::string& text)
	{
		// An operator somewhere past the first character is what makes this worth reading: a leading sign
		// belongs to the number itself, and a plain number is the widget's own business.
		for (size_t index = 1; index < text.size(); ++index)
		{
			const char8 character = text[index];

			if (character == '+' || character == '*' || character == '/' || character == '(')
				return true;

			// A minus after a digit is a subtraction; a minus after 'e' is an exponent, which strtof
			// already reads on its own.
			if (character == '-' && std::isdigit(static_cast<unsigned char>(text[index - 1])))
				return true;
		}

		return false;
	}
}
