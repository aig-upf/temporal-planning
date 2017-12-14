#include "compile.h"

void addRequirements(Domain *d, Domain *cd) {
    cd->name = d->name;
    cd->equality = d->equality;
    cd->condeffects = cd->typed = true;
    cd->cons = d->cons;
}

void addTypes(Domain *d, Domain *cd) {
    cd->setTypes(d->copyTypes());
}

void addPredicates(Domain *d, Domain *cd) {
    for (unsigned i = 0; i < d->preds.size(); ++i) {
        StringVec pars = d->typeList(d->preds[i]);
        cd->createPredicate(d->preds[i]->name, pars);
    }
}

void addActionPreconditions(Domain *d, Domain *cd, TemporalAction *a) {
    // precondition = pre_s(a) U ((pre_o(a) U (pre_e(a)) | add_s(a)))
    cd->setPre(a->name, a->pre);

    for (unsigned i = 0; i < a->pre_o->conds.size(); ++i) {
        Ground *g = dynamic_cast<Ground *>(a->pre_o->conds[i]);
        if (g && !includes(false, g, dynamic_cast<And *>(a->eff))) {
            cd->addPre(0, a->name, g->name, g->params);
        }
    }

    for (unsigned i = 0; i < a->pre_e->conds.size(); ++i) {
        Ground *g = dynamic_cast<Ground *>(a->pre_e->conds[i]);
        if (g && !includes(false, g, dynamic_cast<And *>(a->eff))) {
            cd->addPre(0, a->name, g->name, g->params);
        }
    }
}

void addActionAddEffects(Domain *d, Domain *cd, TemporalAction *a) {
    // add = (add_s \ del_e) U add_e
    GroundVec addEffects = a->addEffects();
    GroundVec addEndEffects = a->addEndEffects();
    GroundVec delEndEffects = a->deleteEndEffects();

    for (unsigned i = 0; i < addEndEffects.size(); ++i) {
        cd->addEff(0, a->name, addEndEffects[i]->name,
                   IntVec(addEndEffects[i]->params));
    }

    for (unsigned i = 0; i < addEffects.size(); ++i) {
        Ground *ga = addEffects[i];
        bool addGround = true;
        for (unsigned j = 0; j < delEndEffects.size(); ++j) {
            Ground *gb = delEndEffects[j];
            if (ga->name == gb->name && ga->params == gb->params) {
                addGround = false;
                break;
            }
        }
        if (addGround) {
            cd->addEff(0, a->name, ga->name, IntVec(ga->params));
        }
    }
}

void addActionDeleteEffects(Domain *d, Domain *cd, TemporalAction *a) {
    // delete = (del_s \ add_e) U del_e
    GroundVec delEffects = a->deleteEffects();
    GroundVec addEndEffects = a->addEndEffects();
    GroundVec delEndEffects = a->deleteEndEffects();

    for (unsigned i = 0; i < delEndEffects.size(); ++i) {
        cd->addEff(1, a->name, delEndEffects[i]->name,
                   IntVec(delEndEffects[i]->params));
    }

    for (unsigned i = 0; i < delEffects.size(); ++i) {
        Ground *ga = delEffects[i];
        bool addGround = true;
        for (unsigned j = 0; j < addEndEffects.size(); ++j) {
            Ground *gb = addEndEffects[j];
            if (ga->name == gb->name && ga->params == gb->params) {
                addGround = false;
                break;
            }
        }
        if (addGround) {
            cd->addEff(1, a->name, ga->name, IntVec(ga->params));
        }
    }
}

void addActions(Domain *d, Domain *cd) {
    for (unsigned i = 0; i < d->actions.size(); ++i) {
        TemporalAction *a = get(i);
        cd->createAction(a->name, d->typeList(a));
        addActionPreconditions(d, cd, a);
        addActionAddEffects(d, cd, a);
        addActionDeleteEffects(d, cd, a);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./compileSequential <domain.pddl> <task.pddl>\n";
        exit(1);
    }

    d = new Domain(argv[1]);
    ins = new Instance(*d, argv[2]);

    cd = new Domain;

    addRequirements(d, cd);
    addTypes(d, cd);
    addPredicates(d, cd);
    addActions(d, cd);

    ins->metric = false; // nullify temporal metric

    std::cout << *cd;
    std::cerr << *ins;

    delete cd;
    delete ins;
    delete d;

    return 0;
}
