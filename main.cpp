
#include "utf_converter.h"
#include <fstream>
#include <iterator>

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
