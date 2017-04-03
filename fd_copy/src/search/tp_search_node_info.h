#ifndef TP_SEARCH_NODE_INFO_H
#define TP_SEARCH_NODE_INFO_H

#include <list>
#include "search_node_info.h"
#include "tp_actions.h"

using namespace std;

// IDEA: update "g" based on temporal info in order to approximate makespan

struct TPSearchNodeInfo : SearchNodeInfo {
	pair<unsigned, int> last_event;
	list<TemporalNode> active_actions;
	pair<int, int> first_parallel_event;
	vector<vector<int> > distance_matrix;

    TPSearchNodeInfo() : first_parallel_event(-1, 0) {}

	bool is_applicable(const GlobalOperator *) const;
	bool schedule(unsigned, unsigned, vector< vector< int > > &) const;
	void update_temporal(const TPSearchNodeInfo &);
};

#endif
