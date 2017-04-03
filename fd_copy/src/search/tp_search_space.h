#ifndef TP_SEARCH_SPACE_H
#define TP_SEARCH_SPACE_H

#include "search_space.h"
#include "tp_search_node_info.h"

class TPSearchNode : public SearchNode {
    StateID state_idp; // parent class stateid field is private
    TPSearchNodeInfo &info;
public:
    TPSearchNode(StateID, TPSearchNodeInfo &, OperatorCost);
    void open(const TPSearchNode &, const GlobalOperator *);
    void reopen(const TPSearchNode &, const GlobalOperator *);
    void update_parent(const TPSearchNode &, const GlobalOperator *);
    void dump() const;
};

class TPSearchSpace : public SearchSpace {
    PerStateInformation<TPSearchNodeInfo> tp_search_node_infos;

    OperatorCost cost_type;
public:
    TPSearchSpace(OperatorCost cost_type_);
    TPSearchNode get_node(const GlobalState &state);
    void trace_path(const GlobalState &goal_state,
                    std::vector<const GlobalOperator *> &path) const;
    void trace_temporal_path(const GlobalState &goal_state,
                             std::vector< TemporalNode > &path) const;
    void dump() const;
    bool is_applicable(const GlobalState &, const GlobalOperator *);
};

#endif
