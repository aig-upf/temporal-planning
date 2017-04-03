#include <fstream>
#include "tp_search_engine.h"
#include "option_parser.h"

TPSearchEngine::TPSearchEngine(const Options &opts)
    : SearchEngine(opts)
	, tp_search_space(OperatorCost(opts.get_enum("cost_type"))) {}

void TPSearchEngine::save_plan_if_necessary() const {
	std::cout << "Got here?\n";
    if (found_solution()) {
        save_plan(get_plan());
		save_temporal_plan(get_temporal_plan());
	}
}

const TPSearchEngine::TemporalPlan &TPSearchEngine::get_temporal_plan() const {
	assert(solution_found);
    return temporal_plan;
}

void TPSearchEngine::set_temporal_plan(const TemporalPlan &plan) {
	temporal_plan = plan;
}

bool TPSearchEngine::check_goal_and_set_plan(const GlobalState &state) {
    if (test_goal(state)) {
        cout << "Solution found!" << endl;

        Plan plan;
        tp_search_space.trace_path(state, plan);
        set_plan(plan);

		TemporalPlan temp_plan;
		tp_search_space.trace_temporal_path(state, temp_plan);
		set_temporal_plan(temp_plan);

        return true;
    }
    return false;
}

void TPSearchEngine::save_temporal_plan(const TemporalPlan &plan,
                                        bool generates_multiple_plan_files) {
    // TODO: Refactor: this is only used by the SearchEngine classes
    //       and hence should maybe be moved into the SearchEngine.
    ostringstream filename;
    filename << "tmp_" << g_plan_filename;
    int plan_number = g_num_previously_generated_plans + 1;
    if (generates_multiple_plan_files || g_is_part_of_anytime_portfolio) {
        filename << "." << plan_number;
    } else {
        assert(plan_number == 1);
    }
    ofstream outfile(filename.str());
    for (size_t i = 0; i < plan.size(); ++i) {
		decimal_print( cout, plan[i].index );
		cout << ": ( " << plan[i].action << " ) [";
		decimal_print( cout, plan[i].duration );
		cout << plan[i].duration % 10;
		cout << "]\n";

		decimal_print( outfile, plan[i].index );
		outfile << ": ( " << plan[i].action << " ) [";
		decimal_print( outfile, plan[i].duration );
		outfile << plan[i].duration % 10;
		outfile << "]\n";
    }
	/* COULD COMPUTE MAKESPAN HERE !!!
    int plan_cost = calculate_plan_cost(plan);
    outfile << "; cost = " << plan_cost << " ("
            << (is_unit_cost() ? "unit cost" : "general cost") << ")" << endl;
	*/
    outfile.close();
	/* COULD OUTPUT MAKESPAN HERE !!!
    cout << "Plan length: " << plan.size() << " step(s)." << endl;
    cout << "Plan cost: " << plan_cost << endl;
	*/
}
