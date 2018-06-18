#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <typeinfo>
#include <vector>

#include "compile.h"

#define SCHEDULE_DEBUG false

std::string atom2string(Domain &d, Ground *g, StringVec gaction) {
    std::stringstream ss;
    ss << "(" << g->name;

    for (unsigned i = 0; i < g->params.size(); ++i)
        if (g->params[i] >= 0) { // is a variable
            ss << " " << gaction[g->params[i] + 1];
        } else { // is a constant
            ss << " "
               << d.types[g->lifted->params[i]]->object(g->params[i]).first;
        }
    ss << ")";
    return ss.str();
}

class CausalLink {
public:
    unsigned _u, _v;
    std::string _atom;

    CausalLink(unsigned u, unsigned v, std::string atom) {
        _u = u;
        _v = v;
        _atom = atom;
    }
};

class TemporalPlan {
public:
    bool TP_SHE;
    std::vector<CausalLink> _causalLinks;

    std::vector<StringVec> _actions;
    std::vector<double> _startTimes;
    std::vector<double> _durations;
    std::vector<int> _tindexes;
    std::vector<int> _cindexes;

    TemporalPlan() { TP_SHE = false; }

    void readClassicalPlan(const std::string &planFileName, Domain *t_d,
                           Domain *c_d) {
        std::cout << ";";
        for (Filereader f(std::string(planFileName + ""));
             !f.f.eof() && f.s.find("; cost =") != 0; f.c = 0) {
            f.next();
            f.assert_token("(");

            // Reading action info
            unsigned c = 0;
            std::vector<std::string> actionInfo;
            while (c < f.s.size()) {

                std::stringstream ss;
                while (f.s[c] != ' ' && f.s[c] != '\t' && f.s[c] != '\r' &&
                       f.s[c] != '\n' && f.s[c] != '\f' && f.s[c] != '(' &&
                       f.s[c] != ')') {
                    ss << (97 <= f.s[c] && f.s[c] <= 122 ? (char)(f.s[c++] - 32)
                                                         : f.s[c++]);
                }

                if (ss.str() != "")
                    actionInfo.push_back(ss.str());
                c++;
            }

            // Adding action to the plan
            _actions.push_back(actionInfo);

            // Reading new line
            getline(f.f, f.s);
        }

        // Initiallize action values
        for (unsigned i = 0; i < _actions.size(); i++) {
            _startTimes.push_back(0);
            _durations.push_back(0);
        }

        // Initiallize action indexes
        for (unsigned i = 0; i < _actions.size(); i++) {
            bool bfound = false;
            for (unsigned j = 0; j < t_d->actions.size(); ++j) {

                if ((_actions[i][0] == "PUSH-" + t_d->actions[j]->name) ||
                    (_actions[i][0] == "START-" + t_d->actions[j]->name) ||
                    (_actions[i][0] == "DO-" + t_d->actions[j]->name) ||
                    (_actions[i][0] == "POP-" + t_d->actions[j]->name) ||
                    (_actions[i][0] == "END-" + t_d->actions[j]->name)) {
                    TP_SHE = true;
                }

                if ((TP_SHE && ((_actions[i][0] == "PUSH-" + t_d->actions[j]->name) ||
                                (_actions[i][0] == "START-" + t_d->actions[j]->name) ||
                                (_actions[i][0] == "DO-" + t_d->actions[j]->name) ||
                                (_actions[i][0] == "POP-" + t_d->actions[j]->name) ||
                                (_actions[i][0] == "END-" + t_d->actions[j]->name))) ||
                    (!TP_SHE && _actions[i][0] == t_d->actions[j]->name)){
                    _tindexes.push_back(j);
                    bfound = true;
                    break;
                }
            }

            if (bfound == false) {
                _tindexes.push_back(-1);
            }

            for (unsigned j = 0; j < c_d->actions.size(); ++j) {
                if (_actions[i][0] == c_d->actions[j]->name) {
                    _cindexes.push_back(j);
                    break;
                }
            }
        }
    }

    void compute_causal_links(Domain *c_d) {
        for (unsigned i = 0; i < _actions.size(); ++i) {
            for (unsigned m = 0;
                 m < c_d->actions[_cindexes[i]]->addEffects().size(); ++m) {
                Ground *gi = dynamic_cast<Ground *>(
                    c_d->actions[_cindexes[i]]->addEffects()[m]);
                if ((!TP_SHE && gi->name == "EMPTYSTACK") ||
                    (!TP_SHE && gi->name == "STACK1"))
                    continue;

                for (unsigned j = i + 1; j < _actions.size(); ++j) {
                    for (unsigned n = 0;
                         n < c_d->actions[_cindexes[j]]->precons().size();
                         ++n) {
                        bool bneg = false;
                        Ground *gj = dynamic_cast<Ground *>(
                            c_d->actions[_cindexes[j]]->precons()[n]);
                        if (gj == NULL) {
                            Not *pnot = dynamic_cast<Not *>(
                                c_d->actions[_cindexes[j]]->precons()[n]);
                            gj = pnot->cond;
                            bneg = true;
                        }

                        if (atom2string(*c_d, gi, _actions[i]) ==
                                atom2string(*c_d, gj, _actions[j]) &&
                            (bneg == false)) {
                            bool bthreat = false;
                            for (unsigned k = i + 1; (k < j && !bthreat); ++k) {
                                for (unsigned o = 0;
                                     (o < c_d->actions[_cindexes[k]]
                                              ->addEffects()
                                              .size() &&
                                      !bthreat);
                                     ++o) {
                                    Ground *gk = dynamic_cast<Ground *>(
                                        c_d->actions[_cindexes[k]]
                                            ->addEffects()[o]);

                                    if (atom2string(*c_d, gi, _actions[i]) ==
                                        atom2string(*c_d, gk, _actions[k]))
                                        bthreat = true;
                                }
                            }

                            if (!bthreat) {
                                _causalLinks.push_back(CausalLink(
                                    i, j, atom2string(*c_d, gi, _actions[i])));
                            }
                        }
                    }
                }
            }
        }

        for (unsigned i = 1; i < _actions.size(); ++i) {
            for (unsigned m = 0;
                 m < c_d->actions[_cindexes[i]]->deleteEffects().size(); ++m) {
                Ground *gi = dynamic_cast<Ground *>(
                    c_d->actions[_cindexes[i]]->deleteEffects()[m]);

                for (unsigned j = 0; j < i; ++j) {
                    for (unsigned n = 0;
                         n < c_d->actions[_cindexes[j]]->precons().size();
                         ++n) {
                        Ground *gj = dynamic_cast<Ground *>(
                            c_d->actions[_cindexes[j]]->precons()[n]);
                        if ((gj != NULL) &&
                            atom2string(*c_d, gi, _actions[i]) ==
                                atom2string(*c_d, gj, _actions[j])) {
                            _causalLinks.push_back(CausalLink(
                                j, i, atom2string(*c_d, gi, _actions[i])));
                        }
                    }
                }
            }
        }

        for (unsigned i = 0; i < _actions.size(); ++i) {
            for (unsigned m = 0;
                 m < c_d->actions[_cindexes[i]]->deleteEffects().size(); ++m) {
                Ground *gi = dynamic_cast<Ground *>(
                    c_d->actions[_cindexes[i]]->deleteEffects()[m]);
                if ((!TP_SHE && gi->name == "EMPTYSTACK") ||
                    (!TP_SHE && gi->name == "STACK1"))
                    continue;

                for (unsigned j = i + 1; j < _actions.size(); ++j) {
                    for (unsigned n = 0;
                         n < c_d->actions[_cindexes[j]]->addEffects().size();
                         ++n) {
                        Ground *gj = dynamic_cast<Ground *>(
                            c_d->actions[_cindexes[j]]->addEffects()[n]);

                        if (atom2string(*c_d, gi, _actions[i]) ==
                            atom2string(*c_d, gj, _actions[j])) {
                            bool isprimary = false;
                            for (unsigned k = j + 1;
                                 (k < _actions.size() && !isprimary); ++k) {
                                for (unsigned o = 0;
                                     (o < c_d->actions[_cindexes[k]]
                                              ->precons()
                                              .size() &&
                                      !isprimary);
                                     ++o) {
                                    Ground *gk = dynamic_cast<Ground *>(
                                        c_d->actions[_cindexes[k]]
                                            ->precons()[o]);
                                    if (gk != NULL &&
                                        atom2string(*c_d, gi, _actions[i]) ==
                                            atom2string(*c_d, gk, _actions[k]))
                                        isprimary = true;
                                }
                            }
                            if (isprimary) {
                                _causalLinks.push_back(CausalLink(
                                    i, j, atom2string(*c_d, gi, _actions[i])));
                            }
                        }
                    }
                }
            }
        }

        return;
    }

    void compute_durations(Domain *t_d, Instance *t_ins) {
        // Read actions durations and initialize start times
        std::vector<double> remaining;
        double time = 0;
        for (unsigned i = 0; i < _actions.size(); i++) {
            if (_tindexes[i] == -1) {
                continue;
            }

            if ((_actions[i][0] ==
                 "PUSH-" + t_d->actions[_tindexes[i]]->name) ||
                (_actions[i][0] ==
                 "START-" + t_d->actions[_tindexes[i]]->name)) {
                TemporalAction *t_a =
                    (TemporalAction *)t_d->actions[_tindexes[i]];
                StringVec params(_actions[i].begin() + 1, _actions[i].end());
                double aux_dur = t_a->durationExpr->evaluate(*t_ins, params);
                remaining.push_back(aux_dur);
                _durations[i] = 0;
            }

            if ((_actions[i][0] == "DO-" + t_d->actions[_tindexes[i]]->name) ||
                (!TP_SHE && _actions[i][0] == t_d->actions[_tindexes[i]]->name)) {
                TemporalAction *t_a =
                    (TemporalAction *)t_d->actions[_tindexes[i]];
                StringVec params(_actions[i].begin() + 1, _actions[i].end());
                double aux_dur = t_a->durationExpr->evaluate(*t_ins, params);

                _durations[i] = aux_dur;
                if (remaining.size() > 0) {
                    remaining[remaining.size() - 1] =
                        remaining.back() - _durations[i];
                }
            }

            if ((_actions[i][0] == "POP-" + t_d->actions[_tindexes[i]]->name) ||
                (_actions[i][0] == "END-" + t_d->actions[_tindexes[i]]->name)) {
                _durations[i] = remaining.back();
                remaining.pop_back();
            }

            _startTimes[i] = time;
            time = time + _durations[i];
        }
    }

    void schedule_DAG_with_critical_path(Domain *t_d, Instance *t_ins) {
        // Compute the indegrees for each action
        std::vector<unsigned> indegree(_actions.size());
        for (unsigned i = 0; i < _actions.size(); ++i) {
            indegree[i] = 0;
        }

        for (unsigned i = 0; i < _causalLinks.size(); ++i) {
            indegree[_causalLinks[i]._v]++;
        }

        // Initialize distances and insert queue the roots
        std::queue<unsigned> Q;
        std::vector<double> d(_actions.size());
        for (unsigned i = 0; i < _actions.size(); ++i) {
            d[i] = 0;
            if (indegree[i] == 0)
                Q.push(i);
        }

        unsigned cur_vertex;
        while (!Q.empty()) {
            cur_vertex = Q.front();
            Q.pop();
            for (unsigned i = 0; i < _causalLinks.size(); ++i) {
                if (cur_vertex == _causalLinks[i]._u) {
                    d[_causalLinks[i]._v] =
                        std::max(d[_causalLinks[i]._v],
                                 d[_causalLinks[i]._u] +
                                     _durations[cur_vertex] + 0.0002);
                    indegree[_causalLinks[i]._v]--;
                    if (indegree[_causalLinks[i]._v] == 0)
                        Q.push(_causalLinks[i]._v);
                }
            }
        }

        // Updating the start times and durations
        for (unsigned i = 0; i < _actions.size(); i++) {
            _startTimes[i] = d[i];

            if (_tindexes[i] != -1) {
                TemporalAction *t_a =
                    (TemporalAction *)t_d->actions[_tindexes[i]];
                StringVec params(_actions[i].begin() + 1, _actions[i].end());
                double aux_dur = t_a->durationExpr->evaluate(*t_ins, params);
                _durations[i] = aux_dur;
            } else {
                _durations[i] = 0;
            }
        }
    }

    friend std::ostream &operator<<(std::ostream &stream,
                                    const TemporalPlan &tp) {
        stream << std::fixed << std::setprecision(4);
        if (SCHEDULE_DEBUG) {
            stream << ";;; Length: " << tp._actions.size() << std::endl;
            double makespan = 0;
            for (unsigned i = 0; i < tp._actions.size(); i++) {
                if (tp._startTimes[i] + tp._durations[i] > makespan)
                    makespan = tp._startTimes[i] + tp._durations[i];
            }
            stream << ";;; Makespan: " << makespan << std::endl;
        }

        stream << ";;; init: (action) [duration]" << std::endl;
        for (unsigned i = 0; i < tp._actions.size(); i++) {
            std::stringstream ss;

            for (unsigned j = 1; j < tp._actions[i].size(); j++)
                ss << " " << tp._actions[i][j];

            if (SCHEDULE_DEBUG) {
                stream << tp._startTimes[i] << ": (" << tp._actions[i][0]
                       << ss.str() << ") [" << tp._durations[i] << "]"
                       << std::endl;
            } else {
                if (tp.TP_SHE) {
                    if (tp._actions[i][0].find("DO-") == 0) { // TPSHE actions
                        stream << tp._startTimes[i] << ": ("
                               << tp._actions[i][0].substr(3) << ss.str() << ") ["
                               << tp._durations[i] << "]" << std::endl;
                    } else if (tp._actions[i][0].find("PUSH-") == 0) { // TPSHE actions
                        stream << tp._startTimes[i] << ": ("
                               << tp._actions[i][0].substr(5) << ss.str() << ") ["
                               << tp._durations[i] << "]" << std::endl;
                    } else if (tp._actions[i][0].find("END-") == 0) { // TEMPO(1) actions
                        stream << tp._startTimes[i] << ": ("
                               << tp._actions[i][0].substr(4) << ss.str() << ") ["
                               << tp._durations[i] << "]" << std::endl;
                    }
                }
                else {
                    stream << tp._startTimes[i] << ": (" << tp._actions[i][0]
                           << ss.str() << ") [" << tp._durations[i] << "]"
                           << std::endl;
                }
            }
        }

        if (SCHEDULE_DEBUG) {
            stream << std::endl;
            stream << ";;; Causal Links" << std::endl;
            for (unsigned i = 0; i < tp._causalLinks.size(); i++) {
                stream << tp._actions[tp._causalLinks[i]._u] << " -> "
                       << tp._actions[tp._causalLinks[i]._v] << ", "
                       << tp._causalLinks[i]._atom << std::endl;
            }
        }

        return stream;
    }
};

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Usage: ./planSchedule <tdomain.pddl> <cdomain.pddl> "
                     "<tproblem.pddl> <cplan.txt>\n";
        exit(1);
    }

    Domain *t_d = new Domain(argv[1]);
    Domain *c_d = new Domain(argv[2]);
    Instance *t_ins = new Instance(*t_d, argv[3]);
    TemporalPlan *t_plan = new TemporalPlan();

    t_plan->readClassicalPlan(argv[4], t_d, c_d);
    t_plan->compute_durations(t_d, t_ins);
    t_plan->compute_causal_links(c_d);
    t_plan->schedule_DAG_with_critical_path(t_d, t_ins);
    std::cout << *t_plan << std::endl;

    delete t_plan;
    delete t_ins;
    delete c_d;
    delete t_d;

    return 0;
}
