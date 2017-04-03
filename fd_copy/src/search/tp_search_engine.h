#ifndef TP_SEARCH_ENGINE_H
#define TP_SEARCH_ENGINE_H

#include "search_engine.h"
#include "tp_actions.h"
#include "tp_search_space.h"

using namespace std;

namespace options {
class OptionParser;
class Options;
}

class TPSearchEngine : public SearchEngine {
private:
	typedef vector<TemporalNode> TemporalPlan;
    TemporalPlan temporal_plan;
protected:
    TPSearchSpace tp_search_space;
public:
    TPSearchEngine(const options::Options &opts);
    void save_plan_if_necessary() const;
    const TemporalPlan &get_temporal_plan() const;
    void set_temporal_plan(const TemporalPlan &);
    bool check_goal_and_set_plan(const GlobalState &);
	static void save_temporal_plan(const TemporalPlan &plan, bool generates_multiple_plan_files = false);
};

#endif
