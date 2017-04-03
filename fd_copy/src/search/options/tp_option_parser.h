#ifndef OPTIONS_TP_OPTION_PARSER_H
#define OPTIONS_TP_OPTION_PARSER_H

#include "option_parser.h"

namespace options {
/*
  The OptionParser stores a parse tree and an Options object. By
  calling addArgument, the parse tree is partially parsed, and the
  result is added to the Options.
*/
class TPOptionParser : public OptionParser {
public:
    TPOptionParser(const TPOptionParser &other) = delete;
    TPOptionParser &operator=(const TPOptionParser &other) = delete;
    TPOptionParser(std::string config, bool dr);
    TPOptionParser(ParseTree pt, bool dr);
    ~TPOptionParser() = default;

    //this is where input from the commandline goes:
    static SearchEngine *parse_cmd_line(
        int argc, const char **argv, bool dr, bool is_unit_cost);
};
}

#endif
