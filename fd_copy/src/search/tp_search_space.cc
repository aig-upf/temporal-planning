#include "tp_search_space.h"

#include "global_operator.h"
#include "globals.h"

#include <cassert>

using namespace std;

TPSearchNode::TPSearchNode(StateID state_id_, TPSearchNodeInfo &info_,
                           OperatorCost cost_type_)
    : SearchNode(state_id_, info_, cost_type_), state_idp(state_id_), info(info_) {
    assert(state_idp != StateID::no_state);
}

void TPSearchNode::open(const TPSearchNode &parent_node,
                        const GlobalOperator *parent_op) {
	SearchNode::open(parent_node, parent_op);
	info.update_temporal(parent_node.info);
}

void TPSearchNode::reopen(const TPSearchNode &parent_node,
                          const GlobalOperator *parent_op) {
	SearchNode::reopen(parent_node, parent_op);
	info.update_temporal(parent_node.info);
}

void TPSearchNode::update_parent(const TPSearchNode &parent_node,
                                 const GlobalOperator *parent_op) {
  SearchNode::update_parent(parent_node, parent_op);
	info.update_temporal(parent_node.info);
}

void TPSearchNode::dump() const {
  cout << state_idp << ": ";
  // g_state_registry->lookup_state(state_idp).dump_pddl();
  if (info.creating_operator) {
      cout << " created by " << info.creating_operator->get_name()
           << " from " << info.parent_state_id << endl;
  } else {
      cout << " no parent" << endl;
  }
}

TPSearchSpace::TPSearchSpace(OperatorCost cost_type_)
    : SearchSpace(cost_type_), cost_type(cost_type_) {
}

TPSearchNode TPSearchSpace::get_node(const GlobalState &state) {
    return TPSearchNode(state.get_id(), tp_search_node_infos[state], cost_type);
}

void TPSearchSpace::trace_path(const GlobalState &goal_state,
                               vector<const GlobalOperator *> &path) const {
    GlobalState current_state = goal_state;
    assert(path.empty());
    for (;;) {
        const TPSearchNodeInfo &info = tp_search_node_infos[current_state];
        const GlobalOperator *op = info.creating_operator;
        if (op == 0) {
            assert(info.parent_state_id == StateID::no_state);
            break;
        }
        path.push_back(op);
        current_state = g_state_registry->lookup_state(info.parent_state_id);
    }
    reverse(path.begin(), path.end());
}

void TPSearchSpace::trace_temporal_path(const GlobalState &goal_state,
                                        vector<TemporalNode> &path) const {
    GlobalState current_state = goal_state;
    assert(path.empty());

	vector< vector< int > > matrix;
	int count = -1;
	int max_end = 0;

    for (;;) {
        const TPSearchNodeInfo &info = tp_search_node_infos[current_state];
        const GlobalOperator *op = info.creating_operator;
        if (op == 0) {
            assert(info.parent_state_id == StateID::no_state);
            break;
        }
		if ( count < 0 && info.distance_matrix.size() ) {
			matrix = info.distance_matrix;
			count = matrix.size() - 1;
			max_end = 0;
		}

		unsigned ix = op->get_name().find( '-' );
		std::string command = op->get_name().substr( 0, ix );
		std::string real_name = op->get_name().substr( ix + 1 );

		if ( command == "push" || command == "start" ) {
			TemporalNode node;
			node.action = real_name;
			node.duration = op->get_cost();
			node.index = count ? -matrix[count][0] : 0;
			path.push_back( node );

			count--;
			// keep track of the maximum end time
			if ( max_end < node.index + node.duration )
				max_end = node.index + node.duration;
			// once a batch finishes, update start time of later batches
			for ( unsigned i = 0; count < 0 && i + matrix.size() < path.size(); ++i )
				path[i].index += max_end + 20;
		}
        current_state = g_state_registry->lookup_state(info.parent_state_id);
    }
    reverse(path.begin(), path.end());
}

void TPSearchSpace::dump() const {
    for (PerStateInformation<TPSearchNodeInfo>::const_iterator it =
             tp_search_node_infos.begin(g_state_registry);
         it != tp_search_node_infos.end(g_state_registry); ++it) {
        StateID id = *it;
        GlobalState s = g_state_registry->lookup_state(id);
        const TPSearchNodeInfo &node_info = tp_search_node_infos[s];
        cout << id << ": ";
        s.dump_fdr();
        if (node_info.creating_operator && node_info.parent_state_id != StateID::no_state) {
            cout << " created by " << node_info.creating_operator->get_name()
                 << " from " << node_info.parent_state_id << endl;
        } else {
            cout << "has no parent" << endl;
        }
    }
}

bool TPSearchSpace::is_applicable(const GlobalState & state, const GlobalOperator * op) {
	return tp_search_node_infos[state].is_applicable(op);
}
