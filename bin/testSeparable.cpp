
// ./testSeparable <temporal_domain> <temporal_ins> output.sas

#include "compile.h"

std::vector< CondVec > mutexes;
ActionVec actions;
CondVec init, goal;

std::map< unsigned, std::set< unsigned > > prods;       // producibles
std::map< unsigned, std::set< unsigned > > envs, conts; // envelopes and contents

/*
bool seps = true, sepe = true;
*/

bool mutex( unsigned k, Condition * c, Condition * d ) {
    bool a = false, b = false;
    for ( unsigned i = 0; i < mutexes[k].size(); ++i ) {
        a |= mutexes[k][i] == c;
        b |= mutexes[k][i] == d;
    }
    return a && b;
}

void parseTranslation( const std::string &s, std::vector< CondVec > &v ) {
    Filereader f( s );
    while ( f.s != "end_metric" ) {
        std::getline( f.f, f.s );
    }

    std::getline( f.f, f.s );
    std::istringstream is( f.getToken() );
    f.next();

    unsigned numVariables;
    is >> numVariables;

    for ( unsigned i = 0; i < numVariables; ++i, f.c = 0 ) {
        f.assert_token( "begin_variable" );
        f.getToken();
        f.next();
        f.getToken();
        f.next();

        is.clear();
        is.str(f.getToken());
        unsigned numVariableValues;
        is >> numVariableValues;

        f.next();

        CondVec u;
        for ( unsigned j = 0; j < numVariableValues; ++j ) {
            std::string t = f.getToken();
            f.next();

            if ( t[0] != '<' ) {
                Ground * ground = new Ground( f.getToken() );
                f.assert_token( "(" );

                Lifted * pred = d->preds[d->preds.index( ground->name )];

                for ( unsigned k = 0; k < pred->params.size(); ++k ) {
                    if ( k > 0 ) {
                        f.assert_token( "," );
                    }

                    std::string objName = f.getToken();

                    std::pair< bool, unsigned > res = d->types[pred->params[k]]->parseObject( objName );
                    if ( res.first ) {
                        ground->params.push_back( res.second );
                    }
                    else {
                        std::pair< bool, int > res2 = d->types[pred->params[k]]->parseConstant( objName );
                        if ( res2.first ) {
                            ground->params.push_back( res.second );
                        }
                        else {
                            f.tokenExit( objName );
                        }
                    }
                }

                f.assert_token( ")" );

                Condition * c = ground; //vec[0];
                if ( t == "NegatedAtom" ) {
                    c = new Not( ground );
                }

                u.push_back( c );
            }
            else {
                f.getToken();
                f.next();
                f.getToken();
                f.next();
            }
        }

        f.assert_token( "end_variable" );
        v.push_back( u );
    }

    is.clear();
    is.str( f.getToken() );
    f.next();
    unsigned numMutexGroups;
    is >> numMutexGroups;

    for ( unsigned i = 0; i < numMutexGroups; ++i ) {
        f.assert_token( "begin_mutex_group" );

        is.clear();
        is.str(f.getToken());
        unsigned numMutexFacts;
        is >> numMutexFacts;

        f.next();

        CondVec u;
        for ( unsigned j = 0; j < numMutexFacts; ++j ) {
            is.clear();
            is.str( f.getToken() );
            f.next();
            unsigned varIndex;
            is >> varIndex;

            is.clear();
            is.str( f.getToken() );
            f.next();
            unsigned varValue;
            is >> varValue;

            u.push_back( v[varIndex][varValue] );
        }

        f.assert_token( "end_mutex_group" );
        mutexes.push_back( u );
    }

    f.assert_token( "begin_state" );

    for ( unsigned i = 0; i < v.size(); ++i ) {
        is.clear();
        is.str( f.getToken() );
        f.next();
        unsigned varValue;
        is >> varValue;
        init.push_back( v[i][varValue] );
    }

    f.assert_token( "end_state" );

    f.assert_token( "begin_goal" );
    is.clear();
    is.str( f.getToken() );
    f.next();

    unsigned numGoalPairings;
    is >> numGoalPairings;

    for ( unsigned i = 0; i < numGoalPairings; ++i ) {
        is.clear();
        is.str( f.getToken() );
        f.next();
        unsigned varIndex;
        is >> varIndex;

        is.clear();
        is.str( f.getToken() );
        f.next();
        unsigned varValue;
        is >> varValue;

        goal.push_back( v[varIndex][varValue] );
    }

    f.assert_token( "end_goal" );

    is.clear();
    is.str( f.getToken() );
    f.next();
    unsigned numOperators;
    is >> numOperators;

    for ( unsigned i = 0; i < numOperators; ++i ) {
        f.assert_token( "begin_operator" );

        std::string actionName = f.getToken();
        Action * a = new Action( actionName );
        Action * act = d->actions.get( actionName );

        f.next();

        for ( unsigned j = 0; j < act->params.size(); ++j ) {
            std::string objName = f.getToken();

            std::pair< bool, unsigned > res = d->types[act->params[j]]->parseObject( objName );
            if ( res.first ) {
                a->params.push_back( res.second );
            }
            else {
                std::pair< bool, int > res2 = d->types[act->params[j]]->parseConstant( objName );
                if ( res2.first ) {
                    a->params.push_back( res.second );
                }
                else {
                    f.tokenExit( objName );
                }
            }

            f.next();
        }

        is.clear();
        is.str( f.getToken() );
        f.next();
        unsigned numPrevailConditions;
        is >> numPrevailConditions;

        for ( unsigned j = 0; j < numPrevailConditions; ++j ) {
            is.clear();
            is.str( f.getToken() );
            f.next();
            unsigned varIndex;
            is >> varIndex;

            is.clear();
            is.str( f.getToken() );
            f.next();
            unsigned varValue;
            is >> varValue;
        }

        is.clear();
        is.str( f.getToken() );
        f.next();
        unsigned numEffects;
        is >> numEffects;

        std::vector< std::pair< long, long > > pres, effs;
        for ( unsigned j = 0; j < numEffects; ++j ) {
            is.clear();
            is.str( f.getToken() );
            f.next();
            long numEffectConditions;
            is >> numEffectConditions;

            is.clear();
            is.str( f.getToken() );
            f.next();
            long varIndex;
            is >> varIndex;

            is.clear();
            is.str( f.getToken() );
            f.next();
            long varValueOld;
            is >> varValueOld;

            is.clear();
            is.str( f.getToken() );
            f.next();
            long varValueNew;
            is >> varValueNew;

            if ( varValueOld >= 0 ) {
                pres.push_back( std::make_pair( varIndex, varValueOld ) );
            }

            if ( (varValueNew >= 0) && ((unsigned)varValueNew < v[varIndex].size()) ) {
                effs.push_back( std::make_pair( varIndex, varValueNew ) );
            }
        }

        Ground * dummyGround = new Ground( "dummy" );
        for ( unsigned j = 0; j < pres.size(); ++j ) {
            for ( unsigned k = 0; k < effs.size(); ++k ) {
                for ( unsigned l = 0; l < mutexes.size(); ++l ) {
                    if ( mutex( l, v[pres[j].first][pres[j].second], v[effs[k].first][effs[k].second] ) ) {
                        dummyGround->params.push_back( l );
                    }
                }
            }
        }

        And * actionPreStart = dynamic_cast< And * >( a->pre );
        if ( actionPreStart ) {
            actionPreStart->add( dummyGround );
        }
        else {
            Ground * actionPreStartGround = dynamic_cast< Ground * >( a->pre );
            if ( actionPreStartGround ) {
              And * preAnd = new And;
              a->pre = preAnd;
              preAnd->add( dummyGround );
              preAnd->add( actionPreStartGround );
            }
            else {
                a->pre = dummyGround;
            }
        }

        is.clear();
        is.str( f.getToken() );
        f.next();
        unsigned operatorCost;
        is >> operatorCost;

        actions.push_back( a );

        f.assert_token( "end_operator" );
    }
}

bool isProducer( Action * a, Lifted * l, bool test ) {
    std::set< Condition * > seff, eeff;

    TemporalAction * ta = dynamic_cast< TemporalAction * >( a );

    And * startEffectsAnd = dynamic_cast< And * >( ta->eff );
    if ( startEffectsAnd ) {
        for ( unsigned i = 0; i < startEffectsAnd->conds.size(); ++i ) {
            Ground * groundEffect = dynamic_cast< Ground * >( startEffectsAnd->conds[i] );
            if ( groundEffect && groundEffect->name == l->name ) {
                seff.insert( groundEffect );
            }
            Not * negGroundEffect = dynamic_cast< Not * >( startEffectsAnd->conds[i] );
            if ( negGroundEffect && negGroundEffect->cond->name == l->name ) {
                seff.insert( negGroundEffect );
            }
        }
    }
    else {
        Ground * startEffectGround = dynamic_cast< Ground * >( ta->eff );
        if ( startEffectGround && startEffectGround->name == l->name ) {
            seff.insert( startEffectGround );
        }
        Not * startNegGroundEffect = dynamic_cast< Not * >( ta->eff );
        if ( startNegGroundEffect && startNegGroundEffect->cond->name == l->name ) {
            seff.insert( startNegGroundEffect );
        }
    }

    And * endEffectsAnd = ta->eff_e;
    for ( unsigned i = 0; i < endEffectsAnd->conds.size(); ++i ) {
        Ground * groundEffect = dynamic_cast< Ground * >( endEffectsAnd->conds[i] );
        if ( groundEffect && groundEffect->name == l->name ) {
            eeff.insert( groundEffect );
        }
        Not * negGroundEffect = dynamic_cast< Not * >( endEffectsAnd->conds[i] );
        if ( negGroundEffect && negGroundEffect->cond->name == l->name ) {
            eeff.insert( negGroundEffect );
        }
    }

    if ( seff.size() != eeff.size() ) {
        return false;
    }

    for ( auto i = seff.begin(); i != seff.end(); ++i ) {
        if ( dynamic_cast< Not * >( *i ) ) {
            return false;
        }

        bool b = false;

        Ground * condi = dynamic_cast< Ground * >( *i );
        if ( !condi ) {
            Not * condineg = dynamic_cast< Not * >( *i );
            if ( condineg ) {
                condi = condineg->cond;
            }
        }

        for ( auto j = eeff.begin(); j != eeff.end(); ++j ) {
            Not * condj = dynamic_cast< Not * >( *j );
            b |= condi && condj && condi->params == condj->cond->params;
        }

        if ( !b ) {
            return false;
        }
    }

    return test || seff.size();
}

void identifyProducibles() {
    std::set< unsigned > pcand;
    for ( unsigned i = 0; i < d->preds.size(); ++i ) {
        bool b = true;
        for ( unsigned j = 0; j < ins->init.size(); ++j ) {
            b &= ins->init[j]->name != d->preds[i]->name;
        }
        if ( b ) {
            pcand.insert(i);
        }
    }

    // std::cout << pcand << "\n";

    for ( auto i = pcand.begin(); i != pcand.end(); ++i ) {
        bool b = true;
        for ( unsigned j = 0; j < d->actions.size(); ++j ) {
            b &= isProducer( d->actions[j], d->preds[*i], true );
        }
        for ( unsigned j = 0; b && j < d->actions.size(); ++j ) {
            if ( isProducer( d->actions[j], d->preds[*i], false ) ) {
                prods[*i].insert( j );
            }
        }
    }
}

void identifyEnvelopes() {
    std::map< unsigned, std::set< unsigned > >::iterator i;
    for ( i = prods.begin(); i != prods.end(); ++i ) {
        std::set< unsigned >::const_iterator j;
        for ( j = i->second.begin(); j != i->second.end(); ++j ) {
            TemporalAction * aj = get( *j );
            for ( unsigned k = 0; k < d->actions.size(); ++k ) {
                TemporalAction * ak = get( k );
                if ( ak->duration() < aj->duration() ) {
                    And * akPreo = ak->pre_o;
                    for ( unsigned l = 0; akPreo && l < akPreo->conds.size(); ++l ) {
                        Ground * g = dynamic_cast< Ground * >( akPreo->conds[l] );
                        if ( ! g ) {
                            Not * n = dynamic_cast< Not * >( akPreo->conds[l] );
                            if ( n ) {
                                g = n->cond;
                            }
                        }

                        // TODO: check if second part of the IF is correct
                        if ( g && d->preds.index( g->name ) == int( i->first ) ) {
                            envs[*j].insert( i->first );
                            conts[k].insert( i->first );
                        }
                    }
                }
            }
        }
    }
}

/*
void typify(Action *a, Action &b, std::vector<int> &u,
            std::vector<std::string> &w) {
    for (unsigned i = 0; i < u.size(); ++i)
        w[i] = ins->objects[a->params[u[i]]][b.params[u[i]]];
}

// detect concurrency conditions on two actions
bool detectDependency(Action &a, Action &b) {
    //	std::cout << "detect " << a << "," << b << "\n";

    Action *A = d->actions[d->amap[a.name]];
    Action *B = d->actions[d->amap[b.name]];

    bool mutex = false;
    for (unsigned i = 0; i < a.pre_s[0]->params.size(); ++i)
        for (unsigned j = 0; j < b.pre_s[0]->params.size(); ++j)
            mutex |= a.pre_s[0]->params[i] == b.pre_s[0]->params[j];

    //	if ( mutex ) std::cout << A->name << a.params << " and " << B->name <<
    //b.params << " are mutex!\n";

    if (!mutex) {
        std::map<unsigned, std::set<unsigned>>::iterator ia =
            conts.find(d->amap[a.name]);
        std::map<unsigned, std::set<unsigned>>::iterator ib =
            conts.find(d->amap[b.name]);

        for (unsigned i = 0; i < A->pre_s.size(); ++i) {
            Condition *c1 = A->pre_s[i];
            std::vector<std::string> p1(c1->params.size());
            typify(A, a, c1->params, p1);

            for (unsigned j = 0; j < B->eff_s.size(); ++j) {
                Condition *c2 = B->eff_s[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only if d(b) < d(a)
                if (c1->name == c2->name && p1 == p2 && c2->neg &&
                    B->duration() < A->duration()) {
                    sepe = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.d.i of Sep(e)\n";
                    if (!seps && !sepe)
                        return true;
                }
                // only for contents
                if (c1->name == c2->name && p1 == p2 && !c2->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.i of SHE(s)\n";
                }
            }
            for (unsigned j = 0; j < B->eff_e.size(); ++j) {
                Condition *c2 = B->eff_e[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // always
                if (c1->name == c2->name && p1 == p2 && c2->neg) {
                    sepe = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.a of Sep(e)\n";
                    if (!seps && !sepe)
                        return true;
                }
                // only for contents
                if (c1->name == c2->name && p1 == p2 && !c2->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.i of SHE(s)\n";
                }
            }
        }

        for (unsigned i = 0; i < A->eff_s.size(); ++i) {
            Condition *c1 = A->eff_s[i];
            std::vector<std::string> p1(c1->params.size());
            typify(A, a, c1->params, p1);

            for (unsigned j = 0; j < B->pre_s.size(); ++j) {
                Condition *c2 = B->pre_s[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only if d(b) < d(a)
                if (c1->name == c2->name && p1 == p2 && !c1->neg &&
                    B->duration() < A->duration()) {
                    sepe = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.d.ii of Sep(e)\n";
                    if (!seps && !sepe)
                        return true;
                }
                // only for contents
                if (c1->name == c2->name && p1 == p2 && c1->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.ii of SHE(s)\n";
                }
            }
            for (unsigned j = 0; j < B->pre_o.size(); ++j) {
                Condition *c2 = B->pre_o[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only if c2 is not an invariant provided by a content
                if (ib == conts.end() ||
                    ib->second.find(d->pmap[c2->name]) == ib->second.end()) {
                    // only if d(b) < d(a)
                    if (c1->name == c2->name && p1 == p2 && !c1->neg &&
                        B->duration() < A->duration()) {
                        sepe = false;
                        std::cout << a.name << a.params << " " << b.name
                                  << b.params << ": ";
                        std::cout << c1->name << p1
                                  << " violates Condition 2.d.ii of Sep(e)\n";
                        if (!seps && !sepe)
                            return true;
                    }
                    // only for contents
                    if (c1->name == c2->name && p1 == p2 && c1->neg &&
                        ia != conts.end()) {
                        std::cout << a.name << a.params << " " << b.name
                                  << b.params << ": ";
                        std::cout << c1->name << p1
                                  << " violates Condition 2.e.ii of SHE(s)\n";
                    }
                }
            }
            for (unsigned j = 0; j < B->pre_e.size(); ++j) {
                Condition *c2 = B->pre_e[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // always
                if (c1->name == c2->name && p1 == p2 && !c1->neg) {
                    sepe = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.b of Sep(e)\n";
                    if (!seps && !sepe)
                        return true;
                }
                // only for contents
                if (c1->name == c2->name && p1 == p2 && c1->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.ii of SHE(s)\n";
                }
            }
            for (unsigned j = 0; j < B->eff_s.size(); ++j) {
                Condition *c2 = B->eff_s[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                if (a != b || c1->params != c2->params ||
                    c1->params.size() < A->params.size()) {
                    if (c1->name == c2->name && p1 == p2 &&
                        c1->neg != c2->neg) {
                        // only if d(b) < d(a)
                        if (B->duration() < A->duration()) {
                            sepe = false;
                            std::cout << a.name << a.params << " " << b.name
                                      << b.params << ": ";
                            std::cout
                                << c1->name << p1
                                << " violates Condition 2.d.iii of Sep(e)\n";
                            if (!seps && !sepe)
                                return true;
                        }
                        // only for contents
                        else if (ia != conts.end()) {
                            std::cout << a.name << a.params << " " << b.name
                                      << b.params << ": ";
                            std::cout << c1->name << p1
                                      << " violates Condition 2.e.iii or "
                                         "2.e.iv of SHE(s)\n";
                        }
                    }
                }
            }
            for (unsigned j = 0; j < B->eff_e.size(); ++j) {
                Condition *c2 = B->eff_e[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                if (a != b || c1->params != c2->params ||
                    c1->params.size() < A->params.size()) {
                    if (c1->name == c2->name && p1 == p2 &&
                        c1->neg != c2->neg) {
                        // always
                        sepe = false;
                        std::cout << a.name << a.params << " " << b.name
                                  << b.params << ": ";
                        std::cout << c1->name << p1
                                  << " violates Condition 2.c of Sep(e)\n";
                        if (!seps && !sepe)
                            return true;
                    }
                }
            }
        }

        for (unsigned i = 0; i < A->pre_e.size(); ++i) {
            Condition *c1 = A->pre_e[i];
            std::vector<std::string> p1(c1->params.size());
            typify(A, a, c1->params, p1);

            for (unsigned j = 0; j < B->eff_s.size(); ++j) {
                Condition *c2 = B->eff_s[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only for contents
                if (c1->name == c2->name && p1 == p2 && c2->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.i of SHE(e)\n";
                }
                // always
                if (c1->name == c2->name && p1 == p2 && !c2->neg) {
                    seps = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.a of Sep(s)\n";
                    if (!seps && !sepe)
                        return true;
                }
            }
            for (unsigned j = 0; j < B->eff_e.size(); ++j) {
                Condition *c2 = B->eff_e[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only for contents
                if (c1->name == c2->name && p1 == p2 && c2->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.i of SHE(e)\n";
                }
                // only if d(b) < d(a)
                if (c1->name == c2->name && p1 == p2 && !c2->neg &&
                    B->duration() < A->duration()) {
                    seps = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.d.i of Sep(s)\n";
                    if (!seps && !sepe)
                        return true;
                }
            }
        }

        for (unsigned i = 0; i < A->eff_e.size(); ++i) {
            Condition *c1 = A->eff_e[i];
            std::vector<std::string> p1(c1->params.size());
            typify(A, a, c1->params, p1);

            for (unsigned j = 0; j < B->pre_s.size(); ++j) {
                Condition *c2 = B->pre_s[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only for contents
                if (c1->name == c2->name && p1 == p2 && !c1->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.ii of SHE(e)\n";
                }
                // always
                if (c1->name == c2->name && p1 == p2 && c1->neg) {
                    seps = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.b of Sep(s)\n";
                    if (!seps && !sepe)
                        return true;
                }
            }
            for (unsigned j = 0; j < B->pre_o.size(); ++j) {
                Condition *c2 = B->pre_o[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only if c2 is not an invariant provided by a content
                if (ib == conts.end() ||
                    ib->second.find(d->pmap[c2->name]) == ib->second.end()) {
                    // only for contents
                    if (c1->name == c2->name && p1 == p2 && !c1->neg &&
                        ia != conts.end()) {
                        std::cout << a.name << a.params << " " << b.name
                                  << b.params << ": ";
                        std::cout << c1->name << p1
                                  << " violates Condition 2.e.ii of SHE(e)\n";
                    }
                    // only if d(b) < d(a)
                    if (c1->name == c2->name && p1 == p2 && c1->neg &&
                        B->duration() < A->duration()) {
                        seps = false;
                        std::cout << a.name << a.params << " " << b.name
                                  << b.params << ": ";
                        std::cout << c1->name << p1
                                  << " violates Condition 2.d.ii of Sep(s)\n";
                        if (!seps && !sepe)
                            return true;
                    }
                }
            }
            for (unsigned j = 0; j < B->pre_e.size(); ++j) {
                Condition *c2 = B->pre_e[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                // only for contents
                if (c1->name == c2->name && p1 == p2 && !c1->neg &&
                    ia != conts.end()) {
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.e.ii of SHE(e)\n";
                }
                // only if d(b) < d(a)
                if (c1->name == c2->name && p1 == p2 && c1->neg &&
                    B->duration() < A->duration()) {
                    seps = false;
                    std::cout << a.name << a.params << " " << b.name << b.params
                              << ": ";
                    std::cout << c1->name << p1
                              << " violates Condition 2.d.ii of Sep(s)\n";
                    if (!seps && !sepe)
                        return true;
                }
            }
            for (unsigned j = 0; j < B->eff_s.size(); ++j) {
                Condition *c2 = B->eff_s[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                if (a != b || c1->params != c2->params ||
                    c1->params.size() < A->params.size()) {
                    if (c1->name == c2->name && p1 == p2 &&
                        c1->neg != c2->neg) {
                        // always
                        seps = false;
                        std::cout << a.name << a.params << " " << b.name
                                  << b.params << ": ";
                        std::cout << c1->name << p1
                                  << " violates Condition 2.c of Sep(s)\n";
                        if (!seps && !sepe)
                            return true;
                    }
                }
            }
            for (unsigned j = 0; j < B->eff_e.size(); ++j) {
                Condition *c2 = B->eff_e[j];
                std::vector<std::string> p2(c2->params.size());
                typify(B, b, c2->params, p2);

                if (a != b || c1->params != c2->params ||
                    c1->params.size() < A->params.size()) {
                    if (c1->name == c2->name && p1 == p2 &&
                        c1->neg != c2->neg) {
                        // only if d(b) < d(a)
                        if (B->duration() < A->duration()) {
                            seps = false;
                            std::cout << a.name << a.params << " " << b.name
                                      << b.params << ": ";
                            std::cout
                                << c1->name << p1
                                << " violates Condition 2.d.iii of Sep(s)\n";
                            if (!seps && !sepe)
                                return true;
                        }
                        // only for contents
                        else if (ia != conts.end()) {
                            std::cout << a.name << a.params << " " << b.name
                                      << b.params << ": ";
                            std::cout << c1->name << p1
                                      << " violates Condition 2.e.iii or "
                                         "2.e.iv of SHE(e)\n";
                        }
                    }
                }
            }
        }
    }
    return false;
}

void detectDependencies() {
    for (unsigned i = 0; i < actions.size(); ++i)
        for (unsigned j = 0; j < actions.size(); ++j)
            if (i != j && detectDependency(*actions[i], *actions[j]))
                return;
}

int findIndex(const std::string &s, const std::vector<int> &w) {
    for (unsigned i = 0; i < actions.size(); ++i)
        if (actions[i]->name == s && actions[i]->params == w)
            return i;
    return -1;
}
*/
int main( int argc, char *argv[] ) {
    if ( argc < 4 ) {
        std::cout << "Usage: ./testSeparable <domain.pddl> <task.pddl> <output.sas>\n";
        exit( 1 );
    }

    d = new Domain( argv[1] );
    ins = new Instance( *d, argv[2] );

    std::vector<CondVec> v;
    parseTranslation( argv[3], v );

    identifyProducibles();

    // std::cout << prods << "\n";

    identifyEnvelopes();

/*
    //	std::cout << envs << " " << conts << "\n";

    // Detect dependencies among actions
    detectDependencies();
*/
    //	Action & a = *d->actions[d->amap["SWITCH_ON"]];
    //	Action & b = *d->actions[d->amap["SWITCH_OFF"]];
    //	for ( unsigned i = 0; i < a.params.size(); ++i )
    //		std::cout << i << " " << a.params[i] << " " <<
    //ins->objects[a.params[i]] << "\n";
    //	for ( unsigned i = 0; i < b.params.size(); ++i )
    //		std::cout << i << " " << b.params[i] << " " <<
    //ins->objects[b.params[i]] << "\n";

    //	std::vector< int > w( 2, 0 );
    //	int x = findIndex( "SWITCH_ON", w );
    //	int y = findIndex( "SWITCH_OFF", w );
    //	std::cout << x << " " << y << "\n";

    //	detectDependency( *actions[x], *actions[y] );
    //	detectDependency( *actions[y], *actions[x] );
/*
    for (unsigned i = 0; i < actions.size(); ++i)
        delete actions[i];
    for (unsigned i = 0; i < v.size(); ++i)
        for (unsigned j = 0; j < v[i].size(); ++j)
            delete v[i][j];
            */
    delete ins;
    delete d;

    return 0;
}
