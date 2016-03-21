
#ifndef UTF_CONVERTER_H_
#define UTF_CONVERTER_H_

#include <cstdint>
#include <cstring>
#include <exception>
#include <sstream>
#include <iostream>


#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#include <endian.h>

namespace utf {

typedef uint32_t code_point;

enum Encoding {
	UTF8,
	UTF16_BE,
	UTF16_LE,
	UTF32_BE,
	UTF32_LE,
};

class invalid_unicode_character : public std::exception {};
class invalid_codepoint         : public std::exception {};
class invalid_utf_encoding      : public std::exception {};

namespace detail {

inline std::istream &read_codepoint_utf8(std::istream &is, Encoding encoding, code_point *codepoint) {
	typedef struct {
		unsigned int expected : 4,
                	 seen     : 4,
                	 reserved : 24;
	} state_t;

	state_t shift_state = {0,0,0};
	code_point cp = 0;
	uint8_t ch;

	while(is.read(reinterpret_cast<char *>(&ch), sizeof(ch))) {

		if(shift_state.seen == 0) {
			if((ch & 0x80) == 0) {
				cp = ch & 0x7f;
				// done with this character
				*codepoint = cp;
				break;
			} else if((ch & 0xe0) == 0xc0) {
				// 2 byte
				cp = ch & 0x1f;
				shift_state.expected = 2;
				shift_state.seen     = 1;
			} else if((ch & 0xf0) == 0xe0) {
				// 3 byte
				cp = ch & 0x0f;
				shift_state.expected = 3;
				shift_state.seen     = 1;
			} else if((ch & 0xf8) == 0xf0) {
				// 4 byte
				cp = ch & 0x07;
				shift_state.expected = 4;
				shift_state.seen     = 1;
			} else if((ch & 0xfc) == 0xf8) {
				// 5 byte
				throw invalid_unicode_character(); // Restricted by RFC 3629
			} else if((ch & 0xfe) == 0xfc) {
				// 6 byte
				throw invalid_unicode_character(); // Restricted by RFC 3629
			} else {
				throw invalid_unicode_character();
			}
		} else if(shift_state.seen < shift_state.expected) {
			if((ch & 0xc0) == 0x80) {
				cp <<= 6;
				cp |= ch & 0x3f;
				// increment the shift state
				++shift_state.seen;

				if(shift_state.seen == shift_state.expected) {
					// done with this character

					if(cp >= 0x110000) {
						throw invalid_codepoint();
					}							

					*codepoint = cp;
					break;
				}

			} else {
				throw invalid_unicode_character();
			}
		} else {
			throw invalid_unicode_character();
		}
	}
	
	return is;
}

inline std::istream &read_codepoint_utf16le(std::istream &is, Encoding encoding, code_point *codepoint) {
	uint16_t w1;
	uint16_t w2 = 0;
	if(!is.read(reinterpret_cast<char *>(&w1), sizeof(w1))) {
		return is;
	}

	w1 = le16toh(w1);

	// part of a surrogate pair
	if((w1 & 0xfc00) == 0xd800) {
		if(!is.read(reinterpret_cast<char *>(&w2), sizeof(w2))) {
			throw invalid_unicode_character();
		}

	}

	w2 = le16toh(w2);

	code_point cp;
	if((w1 & 0xfc00) == 0xd800) {
		if((w2 & 0xfc00) == 0xdc00) {
			cp = 0x10000 + (((static_cast<code_point>(w1) & 0x3ff) << 10) | (w2 & 0x3ff));
		} else {
			throw invalid_unicode_character();
		}
	} else {
		cp = w1;
	}

	if(cp >= 0x110000) {
		throw invalid_codepoint();
	}			

	*codepoint = cp;
	return is;
}

inline std::istream &read_codepoint_utf16be(std::istream &is, Encoding encoding, code_point *codepoint) {
	uint16_t w1;
	uint16_t w2 = 0;
	if(!is.read(reinterpret_cast<char *>(&w1), sizeof(w1))) {
		return is;
	}

	w1 = be16toh(w1);

	// part of a surrogate pair
	if((w1 & 0xfc00) == 0xd800) {
		if(!is.read(reinterpret_cast<char *>(&w2), sizeof(w2))) {
			throw invalid_unicode_character();
		}

	}

	w2 = be16toh(w2);

	code_point cp;
	if((w1 & 0xfc00) == 0xd800) {
		if((w2 & 0xfc00) == 0xdc00) {
			cp = 0x10000 + (((static_cast<code_point>(w1) & 0x3ff) << 10) | (w2 & 0x3ff));
		} else {
			throw invalid_unicode_character();
		}
	} else {
		cp = w1;
	}

	if(cp >= 0x110000) {
		throw invalid_codepoint();
	}			

	*codepoint = cp;
	return is;
}

inline std::istream &read_codepoint_utf32le(std::istream &is, Encoding encoding, code_point *codepoint) {
	code_point cp;
	if(!is.read(reinterpret_cast<char *>(&cp), sizeof(cp))) {
		return is;
	}

	*codepoint = le32toh(cp);
	return is;
}

inline std::istream &read_codepoint_utf32be(std::istream &is, Encoding encoding, code_point *codepoint) {
	code_point cp;
	if(!is.read(reinterpret_cast<char *>(&cp), sizeof(cp))) {
		return is;
	}

	*codepoint = be32toh(cp);
}

}

inline std::istream &read_codepoint(std::istream &is, Encoding encoding, code_point *codepoint) {

	switch(encoding) {
	case UTF8:
		return detail::read_codepoint_utf8(is, encoding, codepoint);
	case UTF16_LE:
		return detail::read_codepoint_utf16le(is, encoding, codepoint);
	case UTF16_BE:
		return detail::read_codepoint_utf16be(is, encoding, codepoint);
	case UTF32_LE:
		return detail::read_codepoint_utf32le(is, encoding, codepoint);
	case UTF32_BE:
		return detail::read_codepoint_utf32be(is, encoding, codepoint);
	default:
		throw invalid_utf_encoding();
	}

	return is;
}

template <class Out>
void write_codepoint(code_point cp, Encoding encoding, Out &&out) {

	if(cp >= 0x110000) {
		throw invalid_codepoint();
	}

	switch(encoding) {
	case UTF8:
		if(cp < 0x80) {
			*out++ = static_cast<uint8_t>(cp);
		} else if(cp < 0x0800) {
			*out++ = static_cast<uint8_t>(0xc0 | ((cp >> 6) & 0x1f));
			*out++ = static_cast<uint8_t>(0x80 | (cp & 0x3f));
		} else if(cp < 0x10000) {
			*out++ = static_cast<uint8_t>(0xe0 | ((cp >> 12) & 0x0f));
			*out++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3f));
			*out++ = static_cast<uint8_t>(0x80 | (cp & 0x3f));
		} else if(cp < 0x1fffff) {
			*out++ = static_cast<uint8_t>(0xf0 | ((cp >> 18) & 0x07));
			*out++ = static_cast<uint8_t>(0x80 | ((cp >> 12) & 0x3f));
			*out++ = static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3f));
			*out++ = static_cast<uint8_t>(0x80 | (cp & 0x3f));
		}
		break;
	case UTF16_LE:
		if(cp < 0x10000) {
			*out++ = static_cast<uint8_t>(cp & 0x00ff);
			*out++ = static_cast<uint8_t>((cp >> 8) & 0x00ff);
		} else {
			code_point x = cp - 0x010000;
			uint16_t w1 = 0xD800 + ((x >> 10)  & 0x3FF);
			uint16_t w2 = 0xDC00 + (x & 0x3FF);

			// write the pairs
			write_codepoint(w1, encoding, out);
			write_codepoint(w2, encoding, out);

		}
		break;
	case UTF16_BE:
		if(cp < 0x10000) {
			*out++ = static_cast<uint8_t>((cp >> 8) & 0x00ff);
			*out++ = static_cast<uint8_t>(cp & 0x00ff);			
		} else {
			code_point x = cp - 0x010000;
			uint16_t w1 = 0xD800 + ((x >> 10)  & 0x3FF);
			uint16_t w2 = 0xDC00 + (x & 0x3FF);

			// write the pairs
			write_codepoint(w1, encoding, out);
			write_codepoint(w2, encoding, out);

		}
		break;		
	case UTF32_LE:
		*out++ = static_cast<uint8_t>(cp & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 8)  & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 16) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 24) & 0x00ff);
		break;
	case UTF32_BE:
		*out++ = static_cast<uint8_t>((cp >> 24) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 16) & 0x00ff);
		*out++ = static_cast<uint8_t>((cp >> 8)  & 0x00ff);
		*out++ = static_cast<uint8_t>(cp & 0x00ff);
		break;		
	default:
		throw invalid_utf_encoding();
	}
}

inline Encoding encoding_from_name(const std::string &name) {

	if(name == "UTF8") {
		return UTF8;
	} else if(name == "UTF16-LE") {
		return UTF16_LE;
	} else if(name == "UTF16-BE") {
		return UTF16_BE;
	} else if(name == "UTF32-LE") {
		return UTF32_LE;
	} else if(name == "UTF32-BE") {
		return UTF32_BE;
	} else if(name == "UTF16LE") {
		return UTF16_LE;
	} else if(name == "UTF16BE") {
		return UTF16_BE;
	} else if(name == "UTF32LE") {
		return UTF32_LE;
	} else if(name == "UTF32BE") {
		return UTF32_BE;
	} else if(name == "U16LE") {
		return UTF16_LE;
	} else if(name == "U16BE") {
		return UTF16_BE;
	} else if(name == "U32LE") {
		return UTF32_LE;
	} else if(name == "U32BE") {
		return UTF32_BE;			
	} else {
		throw invalid_utf_encoding();
	}
}

}

#endif
