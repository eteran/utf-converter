
#include "utf_converter.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <getopt.h>

namespace {

void print_usage(const char *argv0) {
	fprintf(stderr, "Usage: %s [OPTIONS] <file>\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "\t-i, --in-encoding=<Encoding> (default: UTF8)\n");
	fprintf(stderr, "\t-o, --out-encoding=<Encoding> (default: UTF8)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Encodings:\n");
	fprintf(stderr, "\tUTF8\n");
	fprintf(stderr, "\tUTF16LE, UTF16-LE, U16-LE\n");
	fprintf(stderr, "\tUTF16BE, UTF16-BE, U16-BE\n");
	fprintf(stderr, "\tUTF32LE, UTF32-LE, U32-LE\n");
	fprintf(stderr, "\tUTF32BE, UTF32-BE, U32-BE\n");
	exit(EXIT_FAILURE);
}

}

int main(int argc, char *argv[]) {

	std::string i_encoding = "UTF8";
	std::string o_encoding = "UTF8";


	static struct option long_options[] = {
		{ "in-encoding" , required_argument, nullptr, 'i' },
		{ "out-encoding", required_argument, nullptr, 'o' },
		{ 0, 0, 0, 0 }
	};

	int option_index;
	int c;

	while((c = getopt_long(argc, argv, "i:o:", long_options, &option_index)) != -1) {
		switch(c) {
		case 'i':
			i_encoding = optarg;
			break;
		case 'o':
			o_encoding = optarg;
			break;
		default:
			print_usage(argv[0]);
		}
	}

	if(optind == argc) {
		print_usage(argv[0]);
	}

	try {
		std::string   in_file      = argv[optind];
		utf::Encoding in_encoding  = utf::encoding_from_name(i_encoding);
		utf::Encoding out_encoding = utf::encoding_from_name(o_encoding);

		std::ifstream file(in_file, std::ifstream::binary);
		if(file) {
			auto curr = std::istreambuf_iterator<char>(file);
			auto last = std::istreambuf_iterator<char>();
			utf::code_point cp;

			while(utf::read_codepoint(curr, last, in_encoding, &cp)) {
				utf::write_codepoint(cp, out_encoding, std::ostream_iterator<uint8_t>(std::cout, ""));
			}
		}
	} catch(const utf::invalid_utf_encoding &) {
		print_usage(argv[0]);
	}
}
