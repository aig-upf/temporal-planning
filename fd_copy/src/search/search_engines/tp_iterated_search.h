#ifndef SEARCH_ENGINES_TP_ITERATED_SEARCH_H
#define SEARCH_ENGINES_TP_ITERATED_SEARCH_H

#include "../option_parser_util.h"
#include "../tp_search_engine.h"

namespace options {
class Options;
}

namespace tp_iterated_search {
class TPIteratedSearch : public TPSearchEngine {
    const std::vector<ParseTree> engine_configs;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    SearchEngine *current_search;
    std::string current_search_name;

    SearchEngine *get_search_engine(int engine_config_start_index);
    SearchEngine *create_phase(int p);
    SearchStatus step_return_value();

    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit TPIteratedSearch(const Options &opts);
    virtual ~TPIteratedSearch() override = default;

    virtual void save_plan_if_necessary() const override;
    virtual void print_statistics() const override;
};
}

#endif
