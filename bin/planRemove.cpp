#include "compile.h"

/**
 * Code for removing objects from actions extended to support numerical fluents
 */

class TemporalPlanAction {
public:
    std::string startTime;
    std::string name;
    StringVec params;
    std::string duration;

    TemporalPlanAction(const std::string& startTime, const std::string& name, const StringVec& params, const std::string& duration)
        : startTime(startTime), name(name), params(params), duration(duration) {}

    void printAction(unsigned numParams) {
        std::cout << startTime << ": (" << name;

        for (unsigned i = 0; i < numParams; ++i) {
            std::cout << " " << params[i];
        }

        std::cout << ") [" << duration << "]\n";
    }
};

class TemporalPlan {
public:
    std::vector<TemporalPlanAction> actions;

    TemporalPlan(const std::string& s) {
        parse(s);
    }

    void parse(const std::string& s) {
        Filereader f(s);

        while (!f.f.eof() && !f.s.empty()) {
            f.c = 0;

            std::stringstream startTime;
            while (f.getChar() != ':') {
                startTime << f.getChar();
                ++f.c;
            }

            ++f.c;
            f.next();
            f.assert_token("(");
            f.next();

            std::string actionName = "";
            StringVec actionParams;

            while (f.getChar() != ')') {
                std::stringstream ss;

                while (f.getChar() != ' ' && f.getChar() != ')') {
                    ss << f.getChar();
                    ++f.c;
                }

                f.next();

                if (actionName.empty()){
                    actionName = ss.str();
                }
                else {
                    actionParams.push_back(ss.str());
                }
            }

            ++f.c;
            f.next();
            f.assert_token("[");

            std::stringstream duration;
            while (f.getChar() != ']') {
                duration << f.getChar();
                ++f.c;
            }

            getline(f.f, f.s);

            TemporalPlanAction tpa(startTime.str(), actionName, actionParams, duration.str());
            actions.push_back(tpa);
        }
    }

    void printPlanWithoutAuxiliaryObjects(Domain * d) {
        std::cout << ";;;; init: (action) [duration]\n";
        for (unsigned i = 0; i < actions.size(); ++i) {
            Action * a = d->actions.get(actions[i].name);
            actions[i].printAction(a->params.size());
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./planRemove <domain.pddl> <plan>\n";
        exit(1);
    }

    d = new Domain(argv[1]);

    TemporalPlan plan(argv[2]);
    plan.printPlanWithoutAuxiliaryObjects(d);

    delete d;

    return 0;
}
