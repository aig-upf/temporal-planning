#include "errors.h"
#include "tp_option_parser.h"

using namespace std;

namespace options {

SearchEngine *TPOptionParser::parse_cmd_line(
    int argc, const char **argv, bool dry_run, bool is_unit_cost) {
	SearchEngine *engine(0);
	try {
		engine = OptionParser::parse_cmd_line(argc, argv, dry_run, is_unit_cost);
	}
	catch (ArgError error) {
	}
	return engine;
}

TPOptionParser::TPOptionParser(const string config, bool dr)
    : OptionParser(config, dr) {}

TPOptionParser::TPOptionParser(ParseTree pt, bool dr)
    : OptionParser(pt, dr) {}
}
