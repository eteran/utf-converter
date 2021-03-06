
#ifndef UTF_CONVERTER_H_
#define UTF_CONVERTER_H_

#include <cstdint>
#include <cstring>
#include <exception>
#include <string>

#if __cplusplus >= 201703L
#include <optional>
#include <string_view>
#else
#include <boost/optional.hpp>
#include <boost/utility/string_view.hpp>
#endif

namespace utf {

#if __cplusplus >= 201703L
template <class T>
using Optional = std::optional<T>;

using StringView = std::string_view;
#else
template <class T>
using Optional = boost::optional<T>;

using StringView = boost::string_view;
#endif


enum Encoding {
	UTF8,
	UTF16_BE,
	UTF16_LE,
	UTF32_BE,
	UTF32_LE,
};

class invalid_unicode_character : public std::exception {};
class invalid_codepoint : public std::exception {};
class invalid_utf_encoding : public std::exception {};

namespace detail {

constexpr uint32_t make_uint32(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) noexcept {
	return (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
}

constexpr uint16_t make_uint16(uint8_t b1, uint8_t b2) noexcept {
	return (b1 << 8) | (b2);
}

template <class In>
bool next_byte(In &it, In end, uint8_t *ch) {
	if (it == end) {
		return false;
	}

	*ch = *it++;
	return true;
}

template <class In>
uint8_t require_byte(In &it, In end) {

	uint8_t ch;
	if (!next_byte(it, end, &ch)) {
		throw invalid_unicode_character();
	}

	return ch;
}

template <class In>
Optional<char32_t> read_codepoint_utf8(In &it, In end) {

	struct state_t {
		unsigned int expected : 4,
			seen : 4,
			reserved : 24;
	};

	state_t  shift_state = {0, 0, 0};
	char32_t cp          = 0;

	while (true) {
		if (shift_state.seen == 0) {

			uint8_t ch;
			if (!next_byte(it, end, &ch)) {
				return {};
			}

			if ((ch & 0x80) == 0) {
				cp = ch & 0x7f;
				// done with this character
				return cp;
			} else if ((ch & 0xe0) == 0xc0) {
				// 2 byte
				cp                   = ch & 0x1f;
				shift_state.expected = 2;
				shift_state.seen     = 1;
			} else if ((ch & 0xf0) == 0xe0) {
				// 3 byte
				cp                   = ch & 0x0f;
				shift_state.expected = 3;
				shift_state.seen     = 1;
			} else if ((ch & 0xf8) == 0xf0) {
				// 4 byte
				cp                   = ch & 0x07;
				shift_state.expected = 4;
				shift_state.seen     = 1;
			} else if ((ch & 0xfc) == 0xf8) {
				// 5 byte
				throw invalid_unicode_character(); // Restricted by RFC 3629
			} else if ((ch & 0xfe) == 0xfc) {
				// 6 byte
				throw invalid_unicode_character(); // Restricted by RFC 3629
			} else {
				throw invalid_unicode_character();
			}
		} else if (shift_state.seen < shift_state.expected) {

			uint8_t ch = require_byte(it, end);

			if ((ch & 0xc0) == 0x80) {
				cp <<= 6;
				cp |= ch & 0x3f;
				// increment the shift state
				++shift_state.seen;

				if (shift_state.seen == shift_state.expected) {
					// done with this character

					if (cp >= 0x110000) {
						throw invalid_codepoint();
					}

					return cp;
				}

			} else {
				throw invalid_unicode_character();
			}
		} else {
			throw invalid_unicode_character();
		}
	}
}

template <class In>
Optional<char32_t> read_codepoint_utf16le(In &it, In end) {

	uint8_t bytes[2];

	if (!next_byte(it, end, &bytes[0])) {
		return {};
	}

	bytes[1] = require_byte(it, end);

	char16_t w1 = make_uint16(bytes[1], bytes[0]);
	char16_t w2 = 0;

	// part of a surrogate pair
	if ((w1 & 0xfc00) == 0xd800) {

		bytes[0] = require_byte(it, end);
		bytes[1] = require_byte(it, end);

		w2 = make_uint16(bytes[1], bytes[0]);
	}

	char32_t cp;
	if ((w1 & 0xfc00) == 0xd800) {
		if ((w2 & 0xfc00) == 0xdc00) {
			cp = 0x10000 + (((static_cast<char32_t>(w1) & 0x3ff) << 10) | (w2 & 0x3ff));
		} else {
			throw invalid_unicode_character();
		}
	} else {
		cp = w1;
	}

	if (cp >= 0x110000) {
		throw invalid_codepoint();
	}

	return cp;
}

template <class In>
Optional<char32_t> read_codepoint_utf16be(In &it, In end) {

	uint8_t bytes[2];

	if (!next_byte(it, end, &bytes[0])) {
		return {};
	}

	bytes[1] = require_byte(it, end);

	char16_t w1 = make_uint16(bytes[0], bytes[1]);
	char16_t w2 = 0;

	// part of a surrogate pair
	if ((w1 & 0xfc00) == 0xd800) {

		bytes[0] = require_byte(it, end);
		bytes[1] = require_byte(it, end);

		w2 = make_uint16(bytes[0], bytes[1]);
	}

	char32_t cp;
	if ((w1 & 0xfc00) == 0xd800) {
		if ((w2 & 0xfc00) == 0xdc00) {
			cp = 0x10000 + (((static_cast<char32_t>(w1) & 0x3ff) << 10) | (w2 & 0x3ff));
		} else {
			throw invalid_unicode_character();
		}
	} else {
		cp = w1;
	}

	if (cp >= 0x110000) {
		throw invalid_codepoint();
	}

	return cp;
}

template <class In>
Optional<char32_t> read_codepoint_utf32le(In &it, In end) {

	uint8_t bytes[4];

	if (!next_byte(it, end, &bytes[0])) {
		return {};
	}

	bytes[1] = require_byte(it, end);
	bytes[2] = require_byte(it, end);
	bytes[3] = require_byte(it, end);

	const char32_t cp = make_uint32(bytes[3], bytes[2], bytes[1], bytes[0]);
	if (cp >= 0x110000) {
		throw invalid_codepoint();
	}

	return cp;
}

template <class In>
Optional<char32_t> read_codepoint_utf32be(In &it, In end) {

	uint8_t bytes[4];

	if (!next_byte(it, end, &bytes[0])) {
		return {};
	}

	bytes[1] = require_byte(it, end);
	bytes[2] = require_byte(it, end);
	bytes[3] = require_byte(it, end);

	const char32_t cp = make_uint32(bytes[0], bytes[1], bytes[2], bytes[3]);
	if (cp >= 0x110000) {
		throw invalid_codepoint();
	}

	return cp;
}

}

template <class In>
Optional<char32_t> read_codepoint(In &it, In end, Encoding encoding) {

	switch (encoding) {
	case UTF8:
		return detail::read_codepoint_utf8(it, end);
	case UTF16_LE:
		return detail::read_codepoint_utf16le(it, end);
	case UTF16_BE:
		return detail::read_codepoint_utf16be(it, end);
	case UTF32_LE:
		return detail::read_codepoint_utf32le(it, end);
	case UTF32_BE:
		return detail::read_codepoint_utf32be(it, end);
	default:
		throw invalid_utf_encoding();
	}
}

template <class Out>
void write_codepoint(char32_t cp, Encoding encoding, Out &&out) {

	if (cp >= 0x110000) {
		throw invalid_codepoint();
	}

	switch (encoding) {
	case UTF8:
		if (cp < 0x80) {
			*out++ = static_cast<uint8_t>(cp);
		} else if (cp < 0x0800) {
			*out++ = static_cast<uint8_t>(0xc0 | ((cp >> 6) & 0x1f));
			*out++ = static_cast<uint8_t>(0x80 | (cp & 0x3f));
		} else if (cp < 0x10000) {
			*out++ = static_cast<uint8_t>(0xe0 | ((cp >> 12) & 0x0f));
			*out++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3f));
			*out++ = static_cast<uint8_t>(0x80 | (cp & 0x3f));
		} else if (cp < 0x1fffff) {
			*out++ = static_cast<uint8_t>(0xf0 | ((cp >> 18) & 0x07));
			*out++ = static_cast<uint8_t>(0x80 | ((cp >> 12) & 0x3f));
			*out++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3f));
			*out++ = static_cast<uint8_t>(0x80 | (cp & 0x3f));
		}
		break;
	case UTF16_LE:
		if (cp < 0x10000) {
			*out++ = static_cast<uint8_t>(cp & 0x00ff);
			*out++ = static_cast<uint8_t>((cp >> 8) & 0x00ff);
		} else {
			char32_t x  = cp - 0x010000;
			char16_t w1 = 0xD800 + ((x >> 10) & 0x3FF);
			char16_t w2 = 0xDC00 + (x & 0x3FF);

			// write the pairs
			write_codepoint(w1, encoding, out);
			write_codepoint(w2, encoding, out);
		}
		break;
	case UTF16_BE:
		if (cp < 0x10000) {
			*out++ = static_cast<uint8_t>((cp >> 8) & 0x00ff);
			*out++ = static_cast<uint8_t>(cp & 0x00ff);
		} else {
			char32_t x  = cp - 0x010000;
			char16_t w1 = 0xD800 + ((x >> 10) & 0x3FF);
			char16_t w2 = 0xDC00 + (x & 0x3FF);

			// write the pairs
			write_codepoint(w1, encoding, out);
			write_codepoint(w2, encoding, out);
		}
		break;
	case UTF32_LE:
		*out++ = static_cast<uint8_t>(cp & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 8) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 16) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 24) & 0x00ff);
		break;
	case UTF32_BE:
		*out++ = static_cast<uint8_t>((cp >> 24) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 16) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 8) & 0x00ff);
		*out++ = static_cast<uint8_t>(cp & 0x00ff);
		break;
	default:
		throw invalid_utf_encoding();
	}
}

inline Encoding encoding_from_name(StringView name) {

	if (name == "UTF8") {
		return UTF8;
	} else if (name == "UTF16-LE") {
		return UTF16_LE;
	} else if (name == "UTF16-BE") {
		return UTF16_BE;
	} else if (name == "UTF32-LE") {
		return UTF32_LE;
	} else if (name == "UTF32-BE") {
		return UTF32_BE;
	} else if (name == "UTF16LE") {
		return UTF16_LE;
	} else if (name == "UTF16BE") {
		return UTF16_BE;
	} else if (name == "UTF32LE") {
		return UTF32_LE;
	} else if (name == "UTF32BE") {
		return UTF32_BE;
	} else if (name == "U16LE") {
		return UTF16_LE;
	} else if (name == "U16BE") {
		return UTF16_BE;
	} else if (name == "U32LE") {
		return UTF32_LE;
	} else if (name == "U32BE") {
		return UTF32_BE;
	} else {
		throw invalid_utf_encoding();
	}
}

}

#endif
