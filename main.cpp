
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <exception>

enum UtfEncoding {
	UTF8,
	UTF16_BE,
	UTF16_LE,
	UTF32_BE,
	UTF32_LE,
};

class invalid_unicode_character : public std::exception {
};

#if 0
	uint32_t potential_bom;
	file.read(reinterpret_cast<char *>(&potential_bom), sizeof(potential_bom));

	printf("%08x\n", potential_bom);

	switch(potential_bom) {
	case 0xFFFE:
	case 0xFEFF:
	case 0x0000FEFF:
	case 0xFFFE0000:
	break;
	}
	file.seekg (0, std::ios_base::beg);
#endif

std::istream &readUtf16Value(std::istream &is, uint32_t *value) {

	uint16_t w1;
	uint16_t w2;
	if(!is.read(reinterpret_cast<char *>(&w1), sizeof(w1))) {
		return is;
	}
	
	// part of a surrogate pair
	if((w1 & 0xfc00) == 0xd800) {
		if(!is.read(reinterpret_cast<char *>(&w2), sizeof(w2))) {
			return is;
		}	
	
	}
	
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
	
	*value = cp;

	return is;
}

std::vector<uint8_t> codepointToUtf8(uint32_t cp) {

	std::vector<uint8_t> utf8;
	auto out = std::back_inserter(utf8);

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
	
	return utf8;
}

int main(int argc, char *argv[]) {

	std::vector<uint32_t> codepoints;
	
	UtfEncoding inputEncoding = UTF16_LE;
	
	
	
	std::ifstream file("Event.log", std::ifstream::binary);
	if(file) {
		switch(inputEncoding) {
		case UTF16_LE:
			{
				uint32_t value;
				while(readUtf16Value(file, &value)) {
					codepoints.push_back(value);
				}
			}
			break;
		default:
			abort();
		}
	}
	
	for(uint32_t cp : codepoints) {
		std::vector<uint8_t> utf8 = codepointToUtf8(cp);
		for(uint8_t byte : utf8) {
			std::cout << byte;
		}
	}
}
