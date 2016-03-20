
#include "utf_converter.h"
#include <fstream>
#include <iterator>
#include <boost/program_options.hpp>

int main(int argc, char *argv[]) {
	
	std::string i_encoding;
	std::string o_encoding;
	std::string endian;

	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options() 
		("help",                                                                       "Print help messages") 
		("in-encoding,i",  po::value<std::string>(&i_encoding)->default_value("UTF8"), "UTF encoding of the input file")
		("out-encoding,o", po::value<std::string>(&o_encoding)->default_value("UTF8"), "UTF encoding of the output")
		("input-file",     po::value<std::string>(),                                   "Input file")
		;

	po::positional_options_description p;
	p.add("input-file", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
		options(desc).positional(p).run(), vm);

	po::notify(vm);

	if(vm.count("help") || vm.count("input-file") == 0) {
		std::cout << desc << "\n";
		return 1;
	}

	std::string   in_file      = vm["input-file"].as<std::string>();
	utf::Encoding in_encoding  = utf::encoding_from_name(vm["in-encoding"].as<std::string>());
	utf::Encoding out_encoding = utf::encoding_from_name(vm["out-encoding"].as<std::string>());

	std::ifstream file(in_file, std::ifstream::binary);
	if(file) {
		utf::code_point cp;
		while(utf::read_codepoint(file, in_encoding, &cp)) {
			utf::write_codepoint(cp, out_encoding, std::ostream_iterator<uint8_t>(std::cout, ""));
		}
	}
}
