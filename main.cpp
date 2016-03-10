
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstring>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#include <endian.h>

namespace utf {

enum Encoding {
	UTF8,
	UTF16_BE,
	UTF16_LE,
	UTF32_BE,
	UTF32_LE,
};

class invalid_unicode_character : public std::exception {};

std::istream &read_codepoint(std::istream &is, Encoding encoding, uint32_t *codepoint) {

	switch(encoding) {
	case UTF8:
		{
			typedef struct {
				unsigned int expected : 4,
                			 seen     : 4,
                			 reserved : 24;
			} state_t;

			state_t shift_state = {0,0,0};
			uint32_t cp = 0;
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
		}
		break;
	case UTF16_LE:
		{
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

			uint32_t cp;
			if((w1 & 0xfc00) == 0xd800) {
				if((w2 & 0xfc00) == 0xdc00) {
					cp = 0x10000 + (((static_cast<uint32_t>(w1) & 0x3ff) << 10) | (w2 & 0x3ff));
				} else {
					throw invalid_unicode_character();
				}
			} else {
				cp = w1;
			}

			*codepoint = cp;
		}
		break;
	case UTF16_BE:
		{
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

			uint32_t cp;
			if((w1 & 0xfc00) == 0xd800) {
				if((w2 & 0xfc00) == 0xdc00) {
					cp = 0x10000 + (((static_cast<uint32_t>(w1) & 0x3ff) << 10) | (w2 & 0x3ff));
				} else {
					throw invalid_unicode_character();
				}
			} else {
				cp = w1;
			}

			*codepoint = cp;
		}
		break;
	case UTF32_LE:
		{
			uint32_t cp;
			if(!is.read(reinterpret_cast<char *>(&cp), sizeof(cp))) {
				return is;
			}

			*codepoint = le32toh(cp);
		}
		break;
	case UTF32_BE:
		{
			uint32_t cp;
			if(!is.read(reinterpret_cast<char *>(&cp), sizeof(cp))) {
				return is;
			}

			*codepoint = be32toh(cp);
		}
		break;
	default:
		abort();
	}

	return is;
}

template <class Out>
void write_codepoint(uint32_t cp, Encoding encoding, Out &&out) {

	// TODO(eteran): remove assumptions that uint32_t is specifically little-endian

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
			uint32_t x = cp - 0x010000;
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
	default:
		abort();
	}
}

Encoding encoding_from_name(const std::string &name) {
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
	} else {
		abort();
	}
}

}

int main(int argc, char *argv[]) {

	utf::Encoding inEncoding  = utf::UTF8;
	utf::Encoding outEncoding = utf::UTF8;
	std::string inFile;

	// TODO(eteran): make this less brittle!
	for(int i = 1; i < argc; ++i) {
		if(strcmp(argv[i], "--in-encoding") == 0) {
			++i;
			inEncoding = utf::encoding_from_name(argv[i]);
		} else if(strcmp(argv[i], "--out-encoding") == 0) {
			++i;
			outEncoding = utf::encoding_from_name(argv[i]);
		} else {
			break;
		}
	}

	inFile = argv[argc - 1];


	std::ifstream file(inFile, std::ifstream::binary);
	if(file) {
		uint32_t cp;
		while(utf::read_codepoint(file, inEncoding, &cp)) {
			utf::write_codepoint(cp, outEncoding, std::ostream_iterator<uint8_t>(std::cout, ""));
		}
	}
}
