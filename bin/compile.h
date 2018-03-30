
#ifndef _COMPILE_H_
#define _COMPILE_H_

#include <parser/Instance.h>

using namespace parser::pddl;

Domain * d;
Instance * ins;
Domain * cd;
Instance * cins;

IntSet pres, effs, dels;

typedef std::vector< bool > BoolVec;
typedef std::vector< StringVec > StringDVec;
typedef std::set< DoublePair > DoublePairSet;
typedef std::map< double, unsigned > DurationMap;

typedef std::pair< Action *, FunctionModifier * > ActionFunctionModPair;
typedef std::vector< ActionFunctionModPair > ActionFunctionModPairVec;
typedef std::pair< Lifted *, ActionFunctionModPairVec > LiftedActionPair;
typedef std::vector< LiftedActionPair > LiftedActionPairVec;

// Simplified graph just for TPSHE
struct graph {
	PairVec edges;

	SetVec depths;
	DoubleVec durations;

	BoolVec mark;
	DurationMap durationMap;
	DoublePairSet subtractionPairs;

	graph( unsigned n )
		: depths( n ), durations( n ), mark( n, 0 ) {
		for ( unsigned i = 0; i < n; ++i )
			durations[i] = d->actions[i]->duration();
	}

	// add an edge to the graph
	void add( int i, int j ) {
		edges.push_back( std::make_pair( i, j ) );
	}

	// check if node is outgoing (i.e. is envelope)
	bool outgoing( int node ) {
		for ( unsigned i = 0; i < edges.size(); ++i )
			if ( edges[i].first == node ) return true;
		return false;
	}

	// compute the possible durations of contents of each envelope
	void computeDurations() {
		SetMap outgoing;
		for ( unsigned i = 0; i < edges.size(); ++i )
			outgoing[edges[i].first].insert( edges[i].second );

		for ( SetMap::iterator i = outgoing.begin(); i != outgoing.end(); ++i ) {
			double d = durations[i->first];

			DoubleSet u;
			for ( IntSet::const_iterator j = i->second.begin(); j != i->second.end(); ++j ) {
				u.insert( durations[*j] );
				durationMap[durations[*j]] = 0;
			}

			DoubleSet v;
			v.insert( 0 );
			for ( DoubleSet::iterator j = v.begin(); j != v.end(); ++j ) {
				durationMap[d - *j] = 0;
				for ( DoubleSet::iterator k = u.begin(); k != u.end(); ++k )
					if ( *j + *k < d ) {
						v.insert( *j + *k );
						subtractionPairs.insert( std::make_pair( d - *j, *k ) );
					}
			}
		}

		unsigned ct = 0;
		for ( DurationMap::iterator j = durationMap.begin(); j != durationMap.end(); ++j )
			j->second = ct++;
	}

	void computeDepths( unsigned n, const SetMap & m ) {
		if ( mark[n] ) return;

		mark[n] = true;
		SetMap::const_iterator i = m.find( n );
		if ( i == m.end() ) depths[n].insert( 0 );
		else for ( IntSet::const_iterator j = i->second.begin(); j != i->second.end(); ++j ) {
			computeDepths( *j, m );
			for ( IntSet::const_iterator k = depths[*j].begin(); k != depths[*j].end(); ++k )
				depths[n].insert( *k + 1 );
		}
	}

	// compute the depths of each node (i.e. lengths of incoming directed paths)
	void computeDepths() {
		SetMap incoming;
		for ( unsigned i = 0; i < edges.size(); ++i )
			incoming[edges[i].second].insert( edges[i].first );

		for ( unsigned i = 0; i < mark.size(); ++i )
			computeDepths( i, incoming );
	}

};

// just to simplify downcast
TemporalAction * get( unsigned a ) {
	return ( TemporalAction * )d->actions[a];
}

// assert that the types of two action conditions are compatible (each is subtype of the other)
bool assertTypes( unsigned a, unsigned b, const Ground * g1, const Ground * g2 ) {
	bool z = true;
	for ( unsigned i = 0; i < g1->params.size(); ++i )
		z |= d->assertSubtype( d->actions[a]->params[g1->params[i]], d->actions[b]->params[g2->params[i]] );
	return z;
}

// check if aa contains a ground condition compatible with g
bool includes( unsigned a, unsigned b, Ground * g, And * aa ) {
	for ( unsigned i = 0; i < aa->conds.size(); ++i ) {
		Ground * h = dynamic_cast< Ground * >( aa->conds[i] );
		if ( h && g->name == h->name && assertTypes( a, b, g, h ) ) return 1;
	}
	return 0;
}

// check if aa contains a ground condition equal to g (possibly negated)
bool includes( bool neg, Ground * g, And * aa ) {
	for ( unsigned i = 0; i < aa->conds.size(); ++i ) {
		Not * n = dynamic_cast< Not * >( aa->conds[i] );
		Ground * h = dynamic_cast< Ground * >( aa->conds[i] );
		if ( ( neg && n && g->name == n->cond->name && g->params == n->cond->params ) ||
		     ( !neg && h && g->name == h->name && g->params == h->params ) )
			return 1;
	}
	return 0;
}

// detect concurrency conditions on two actions
// ASSUMES POSITIVE PRECONDITIONS OVER ALL
void detectDependency( unsigned a, unsigned b, graph & g ) {
	GroundVec gv = get( a )->addEffects();
	for ( unsigned i = 0; i < gv.size(); ++i )
		if ( includes( 1, gv[i], get( a )->eff_e ) &&
		     includes( a, b, gv[i], get( b )->pre_o ) &&
		     get( b )->duration() < get( a )->duration() )
			g.add( a, b );
}

// check whether a context of an envelope (with predicate index k) is deleted by a content
// CHANGE TO COMPUTE *NUMBER* OF OCCURRENCES !!!
bool isPre( int a, int k, graph & g ) {
	for ( unsigned i = 0; i < g.edges.size(); ++i )
		if ( g.edges[i].first == a ) {
			int b = g.edges[i].second;
			GroundVec gv = get( b )->deleteEffects();
			for ( unsigned j = 0; j < gv.size(); ++j )
				if ( d->preds.index( gv[j]->name ) == k ) return 1;
			for ( unsigned j = 0; j < get( b )->eff_e->conds.size(); ++j ) {
				Not * n = dynamic_cast< Not * >( get( b )->eff_e->conds[j] );
				if ( n && d->preds.index( n->cond->name ) == k ) return 1;
			}
			if ( isPre( b, k, g ) ) return 1;
		}
	return 0;
}

// check whether a context is deleted by any action
bool isDel( int k ) {
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		for ( unsigned j = 0; j < ( ( And * )get( i )->eff )->conds.size(); ++j ) {
			Not * n = dynamic_cast< Not * >( ( ( And * )get( i )->eff )->conds[j] );
			if ( n && d->preds.index( n->cond->name ) == k ) return 1;
		}
		for ( unsigned j = 0; j < get( i )->eff_e->conds.size(); ++j ) {
			Not * n = dynamic_cast< Not * >( get( i )->eff_e->conds[j] );
			if ( n && d->preds.index( n->cond->name ) == k ) return 1;
		}
	}
	return 0;
}

// Needed to recurse over the parameters of a cost function for initial state
void recCost( unsigned k, StringVec & pars, const IntVec & indices, unsigned a );

void typeCost( Type * type, unsigned k, StringVec & pars, const IntVec & indices, unsigned a ) {
	for ( unsigned i = 0; i < type->objects.size(); ++i ) {
		pars[indices[k]] = type->objects[i];
		recCost( k + 1, pars, indices, a );
	}
	for ( unsigned i = 0; i < type->constants.size(); ++i ) {
		pars[indices[k]] = type->constants[i];
		recCost( k + 1, pars, indices, a );
	}
	for ( unsigned i = 0; i < type->subtypes.size(); ++i )
		typeCost( type->subtypes[i], k, pars, indices, a );
}

void recCost( unsigned k, StringVec & pars, const IntVec & indices, unsigned a ) {
	if ( k == indices.size() ) {
		StringVec actual;
		for ( unsigned i = 0; i < indices.size(); ++i )
			actual.push_back( pars[indices[i]] );
		double cost = ( int )( 10000 * get( a )->durationExpr->evaluate( *ins, pars ) );
		cins->addInit( get( a )->name + "-COST", cost, actual );
	}
	else typeCost( d->types[d->actions[a]->params[indices[k]]], k, pars, indices, a );
}

// Needed to recurse over the parameters of an action for initial state

bool valid( int k, StringVec & v, const GroundVec & w ) {
	for ( unsigned i = 0; i < w.size(); ++i ) {
		IntVec::iterator it = std::max_element( w[i]->params.begin(), w[i]->params.end() );
		if ( *it == k ) {
			StringVec u( w[i]->params.size() );
			for ( unsigned j = 0; j < w[i]->params.size(); ++j )
				u[j] = v[w[i]->params[j]];

			TypeGround * tg = new TypeGround( d->preds.get( w[i]->name ) );
			tg->insert( *d, u );

			bool b = false;
			for ( unsigned j = 0; j < ins->init.size(); ++j )
				b |= ins->init[j]->name == tg->name && ins->init[j]->params == tg->params;

			delete tg;

			if ( !b ) return false;
		}
	}
	return true;
}

void recurse( TemporalAction * a, unsigned k, StringVec & v, const std::string & prefix, const GroundVec & w );

void typeRec( Type * type, TemporalAction * a, unsigned k, StringVec & v, const std::string & prefix, const GroundVec & w ) {
	for ( unsigned i = 0; i < type->objects.size(); ++i ) {
		v[k] = type->objects[i];
		if ( valid( k, v, w ) )
			recurse( a, k + 1, v, prefix, w );
	}
	for ( unsigned i = 0; i < type->constants.size(); ++i ) {
		v[k] = type->constants[i];
		if ( valid( k, v, w ) )
			recurse( a, k + 1, v, prefix, w );
	}
	for ( unsigned i = 0; i < type->subtypes.size(); ++i )
		typeRec( type->subtypes[i], a, k, v, prefix, w );
}

void recurse( TemporalAction * a, unsigned k, StringVec & v, const std::string & prefix, const GroundVec & w ) {
	if ( k == v.size() ) cins->addInit( prefix + a->name, v );
	else typeRec( cd->types[a->params[k]], a, k, v, prefix, w );
}

void recurse( TemporalAction * a, const std::string & prefix ) {
	GroundVec v;
	for ( unsigned i = 0; i < ( ( And * )a->pre )->conds.size(); ++i ) {
		Ground * g = ( Ground * )( ( And * )a->pre )->conds[i];
		if ( effs.find( d->preds.index( g->name ) ) == effs.end() )
			v.push_back( g );
	}
	for ( unsigned i = 0; i < a->pre_o->conds.size(); ++i ) {
		Ground * g = ( Ground * )a->pre_o->conds[i];
		if ( effs.find( d->preds.index( g->name ) ) == effs.end() )
			v.push_back( g );
	}
	for ( unsigned i = 0; i < a->pre_e->conds.size(); ++i ) {
		Ground * g = ( Ground * )a->pre_e->conds[i];
		if ( effs.find( d->preds.index( g->name ) ) == effs.end() )
			v.push_back( g );
	}

	StringVec pars( a->params.size() );
	recurse( a, 0, pars, prefix, v );
}

// Needed to recurse over the parameters of a predicate for initial state
void recurse( ParamCond * c, unsigned k, StringVec & v, const std::string & prefix );

void typeRec( Type * type, ParamCond * c, unsigned k, StringVec & v, const std::string & prefix ) {
	for ( unsigned i = 0; i < type->objects.size(); ++i ) {
		v[k] = type->objects[i];
		recurse( c, k + 1, v, prefix );
	}
	for ( unsigned i = 0; i < type->constants.size(); ++i ) {
		v[k] = type->constants[i];
		recurse( c, k + 1, v, prefix );
	}
	for ( unsigned i = 0; i < type->subtypes.size(); ++i )
		typeRec( type->subtypes[i], c, k, v, prefix );
}

void recurse( ParamCond * c, unsigned k, StringVec & v, const std::string & prefix ) {
	if ( k == v.size() ) cins->addInit( prefix + c->name, v );
	else typeRec( cd->types[c->params[k]], c, k, v, prefix );
}

void replaceDurationExpressions( Expression * durationExpr, Condition * c, Domain * d ) {
    And * a = dynamic_cast< And * >( c );
    if ( a ) {
        for ( unsigned i = 0; i < a->conds.size(); ++i ) {
            replaceDurationExpressions( durationExpr, a->conds[i], d );
        }
    }

    FunctionModifier * fm = dynamic_cast< FunctionModifier * >( c );
    if ( fm ) {
        replaceDurationExpressions( durationExpr, fm->modifierExpr, d );
    }

    CompositeExpression * ce = dynamic_cast< CompositeExpression * >( c );
    if ( ce ) {
        if ( dynamic_cast< DurationExpression * >( ce->left ) ) {
            ce->left = dynamic_cast< Expression * >( durationExpr->copy( *d ) );
        }
        else {
            replaceDurationExpressions( durationExpr, ce->left, d );
        }

        if ( dynamic_cast< DurationExpression * >( ce->right ) ) {
            ce->right = dynamic_cast< Expression * >( durationExpr->copy( *d ) );
        }
        else {
            replaceDurationExpressions( durationExpr, ce->right, d );
        }
    }
}

void replaceDurationExpressions( Expression * durationExpr, Action * a, Domain * d ) {
    CondVec prec = a->precons();
    for ( unsigned i = 0; i < prec.size(); ++i ) {
        replaceDurationExpressions( durationExpr, prec[i], d );
    }

    // TODO: add preconditions at start

    CondVec eff = a->effects();
    for ( unsigned i = 0; i < eff.size(); ++i ) {
        replaceDurationExpressions( durationExpr, eff[i], d );
    }

    TemporalAction * ta = dynamic_cast< TemporalAction * >( a );
    if ( ta ) {
        CondVec endeff = ta->endEffects();
        for ( unsigned i = 0; i < endeff.size(); ++i ) {
            replaceDurationExpressions( durationExpr, endeff[i], d );
        }
    }
}

void replaceDurationExpressions( Domain * d ) {
    for ( unsigned i = 0; i < d->actions.size(); ++i ) {
        TemporalAction * ta = dynamic_cast< TemporalAction * >( d->actions[i] );
        if ( ta ) {
            replaceDurationExpressions( ta->durationExpr, ta, d );
        }
    }
}

// fills a vector with the functions whose value is incremented or decremented
void getVariableFunctions( Action * ac, Condition * c, LiftedActionPairVec & variableFunctions ) {
    And * a = dynamic_cast< And * >( c );
    if ( a ) {
        for ( unsigned i = 0; i < a->conds.size(); ++i ) {
            getVariableFunctions( ac, c, variableFunctions );
        }
    }

    FunctionModifier * fm = dynamic_cast< FunctionModifier * >( c );
    if ( fm ) {
        Ground * g = fm->modifiedGround;
        Lifted * l = g->lifted;

        bool addNewEntry = true;

        for ( unsigned i = 0; i < variableFunctions.size(); ++i ) {
            if ( variableFunctions[i].first == l) {
                variableFunctions[i].second.push_back( std::make_pair( ac, fm ) );
                addNewEntry = false;
                break;
            }
        }

        if ( addNewEntry ) {
            ActionFunctionModPairVec avec;
            avec.push_back( std::make_pair( ac, fm ) );
            variableFunctions.push_back( std::make_pair( l, avec ) );
        }
    }
}

// fills a vector with the functions whose value is incremented or decremented
LiftedActionPairVec getVariableFunctions( Domain * d ) {
    const TokenStruct< Action * >& domainActions = d->actions;
    LiftedActionPairVec variableFunctions;

    for ( unsigned i = 0; i < domainActions.size(); ++i ) {
        TemporalAction * ta = dynamic_cast< TemporalAction * >( domainActions[i] );

        CondVec actionStartEffects = ta->effects();
        for ( unsigned j = 0; j < actionStartEffects.size(); ++j ) {
            getVariableFunctions( ta, actionStartEffects[j], variableFunctions );
        }

        CondVec actionEndEffects = ta->endEffects();
        for ( unsigned j = 0; j < actionEndEffects.size(); ++j ) {
            getVariableFunctions( ta, actionEndEffects[j], variableFunctions );
        }
    }

    return variableFunctions;
}

void fillObjectSets( StringDVec& objectSets, const StringVec& sv, Domain * d, unsigned currentTypeIndex, int startIndex, int endIndex ) {
    if ( currentTypeIndex >= sv.size() ) return;

    Type * currentType = d->getType( sv[currentTypeIndex] );
    unsigned typeObjs = currentType->noObjects();
    unsigned divisionSize = (endIndex - startIndex) / typeObjs;

    for ( unsigned i = 0; i < typeObjs; ++i ) {
        std::pair< std::string, int > obj = currentType->object( i );
        for ( unsigned j = 0; j < divisionSize; ++j ) {
            objectSets[ startIndex + i * divisionSize + j ].push_back( obj.first );
        }

        fillObjectSets( objectSets, sv, d, currentTypeIndex + 1, startIndex + i * divisionSize, startIndex + (i + 1) *  divisionSize);
    }
}

// returns all possible object combinations for a vector of types
StringDVec getObjectSetsForTypes( const StringVec& sv, Domain * d ) {
    int totalNumObjectsComb = 0;
    for ( unsigned i = 0; i < sv.size(); ++i ) {
        Type * t = d->getType( sv[i] );
        if ( totalNumObjectsComb > 0 ) {
            totalNumObjectsComb *= t->noObjects();
        }
        else {
            totalNumObjectsComb = t->noObjects();
        }
    }

    StringDVec objectSets( totalNumObjectsComb );
    fillObjectSets( objectSets, sv, d, 0, 0, totalNumObjectsComb );
    return objectSets;
}

// gets the grounded function given its lifted predicate and a list of objects
GroundFunc<double> * getGroundFunc( Lifted * l, StringVec& sv, Instance * ins ) {
    const GroundVec& initGrounds = ins->init;

    for ( unsigned i = 0; i < initGrounds.size(); ++i ) {
        if ( l == initGrounds[i]->lifted ) {
            GroundFunc<double> * gf = dynamic_cast< GroundFunc<double> * >( initGrounds[i] );
            if ( gf ) {
                StringVec objList = ins->d.objectList( gf );

                bool objsMatch = true;
                for ( unsigned i = 0; i < sv.size(); ++i ) {
                    if ( sv[i] != objList[i] ) {
                        objsMatch = false;
                        break;
                    }
                }

                if ( objsMatch )
                    return gf;
            }
        }
    }

    return nullptr;
}

// returns all the possible values for a lifted predicat 'l' with object parameter 'sv'
std::set< double > getPossibleValues( Lifted * l, StringVec& sv, ActionFunctionModPairVec& actions, Domain * d, Instance * ins ) {
    // grounded function for which we want to get the possible values
    GroundFunc<double> * groundFunc = getGroundFunc( l, sv, ins );

    // initial value for the lifted condition with sv parameters
    double initialValue = groundFunc->value;

    // structure containing all possible values found in the search
    std::set< double > possibleValues;

    // bfs maximum depth and queue
    int maxDepth = 10;
    std::set< std::pair< int, double > > q;
    q.insert( std::make_pair( 0, initialValue ) );

    while ( !q.empty() ) {
        std::pair< int, double > currentItem = *(q.begin());
        q.erase( q.begin() );

        if ( currentItem.first >= maxDepth ) // max depth reached
            break;

        double currentValue = currentItem.second;
        groundFunc->value = currentValue; // change value in instance so that it is taken into account
        possibleValues.insert( currentValue );

        for ( unsigned i = 0; i < actions.size(); ++i ) {
            Action * a = actions[i].first;
            FunctionModifier * fm = actions[i].second;

            // coger parámetros de la acción y coger todas las combinasiones de parámetros posibles
            TypeVec tv;
            StringVec typeString = ins->d.typeList( a );

            // each set of objects is a different action
            StringDVec objSets = getObjectSetsForTypes( typeString, d );

            for ( unsigned j = 0; j < objSets.size(); ++j ) {
                bool allFunctionParamsFound = true;
                for ( unsigned k = 0; k < sv.size() && allFunctionParamsFound; ++k ) {
                    allFunctionParamsFound &= std::find( objSets[j].begin(), objSets[j].end(), sv[k] ) != objSets[j].end();
                }

                if ( allFunctionParamsFound ) {
                    // checking precons
                    bool preconditionsHold = true;

                    CondVec preconds = a->precons();
                    for ( unsigned j = 0; j < preconds.size() && preconditionsHold; ++j ) {
                        Condition * c = preconds[j];
                        CompositeExpression * ce = dynamic_cast< CompositeExpression * >( c );
                        if ( ce ) {
                            FunctionExpression * fe = dynamic_cast< FunctionExpression * >( ce->left );
                            if ( fe && l->name == fe->fun->name ) {
                                double rightSideExpression = ce->right->evaluate( *ins, objSets[j] );

                                if ( (ce->op == ">" && currentValue <= rightSideExpression) ||
                                     (ce->op == ">=" && currentValue < rightSideExpression) ||
                                     (ce->op == "<" && currentValue >= rightSideExpression) ||
                                     (ce->op == "<=" && currentValue > rightSideExpression) )
                                {
                                    preconditionsHold = false;
                                }
                            }
                        }
                    }

                    // if preconditions hold, add new value to the queue
                    if ( preconditionsHold ) {
                        double newValue = currentValue;
                        double modifiedValue = fm->modifierExpr->evaluate( *ins, objSets[j] );
                        if ( dynamic_cast< Increase * >( fm ) ) {
                            newValue += modifiedValue;
                        }
                        else if ( dynamic_cast< Decrease * >( fm ) ) {
                            newValue -= modifiedValue;
                        }

                        if ( possibleValues.find( newValue ) == possibleValues.end() ) {
                            q.insert( std::make_pair( currentItem.first + 1, newValue ) );
                        }
                    }
                }
            }
        }
    }

    groundFunc->value = initialValue; // reset instance value!!
    return possibleValues;
}

void addActionDurationFunction( Action * a, Domain * d, Instance * ins ) {
    std::string& actionName = a->name;
    std::string functioName = "DURATION-" + actionName;

    Lifted * l = d->createFunction( functioName, -1 );
    l->params = IntVec( a->params );

    StringVec sv = d->typeList( l );

    StringDVec possibleAssignments = getObjectSetsForTypes( sv, d );
    for ( unsigned i = 0; i < possibleAssignments.size(); ++i ) {
        ins->addInit( functioName, double( 8 ), possibleAssignments[i] );
    }
}

void getVariableFunctionsFromExpression( Expression * e, const LiftedActionPairVec& variableFunctions, LiftedVec& lv ) {
    CompositeExpression * ce = dynamic_cast< CompositeExpression * >( e );
    if ( ce ) {
        getVariableFunctionsFromExpression( ce->left, variableFunctions, lv );
        getVariableFunctionsFromExpression( ce->right, variableFunctions, lv );
    }

    FunctionExpression * fe = dynamic_cast< FunctionExpression * >( e );
    if ( fe ) {
        for ( unsigned i = 0; i < variableFunctions.size(); ++i ) {
            Lifted * l = variableFunctions[i].first;
            if ( l->name == fe->fun->name ) {
                lv.push_back( l );
            }
        }
    }
}

void addIncrementRelatedPredicates( const std::set< double >& finalPossibleValues, Action * a, StringDVec& objectSets, Lifted * l, FunctionModifier * fm, Instance * i ) {
    Expression * modifierExpr = fm->modifierExpr;

    for ( unsigned i = 0; i < objectSets.size(); ++i ) {
        GroundFunc<double> * gf = getGroundFunc( l, objectSets[i], ins );
        double initialValue = gf->value;

        for ( auto it = finalPossibleValues.begin(); it != finalPossibleValues.end(); ++it ) {
            gf->value = *it;

            double evaluation = modifierExpr->evaluate( *ins, objectSets[i] );
            double result = gf->value;

            if ( dynamic_cast< Increase * >( fm ) ) {
                result += evaluation;
            }
            else if ( dynamic_cast< Decrease * >( fm ) ) {
                result -= evaluation;
            }

            if ( finalPossibleValues.find( result ) != finalPossibleValues.end() ) {
                std::stringstream currentValue; currentValue << l->name << gf->value;
                std::stringstream incrementValue; incrementValue << l->name << evaluation;
                std::stringstream resultValue; resultValue << l->name << result;

                StringVec sv;
                sv.push_back( currentValue.str() );
                sv.push_back( resultValue.str() );
                sv.push_back( incrementValue.str() );
                ins->addInit( "SUB", sv );

                StringVec sv2 = objectSets[i];
                sv2.push_back( currentValue.str() );
                sv2.push_back( incrementValue.str() );
                ins->addInit( "INCREASE-" + l->name + "-" + a->name, sv2 );
            }
        }

        gf->value = initialValue;
    }
}

void translateInequalities( TemporalAction * ta, Domain * d, Instance * ins, std::set< double >& finalPossibleValues ) {
    And * preStart = dynamic_cast< And * >( ta->pre );
    if ( preStart ) {
        CondVec& andConds = preStart->conds;
        for ( unsigned i = 0; i < andConds.size(); ++i ) {
            CompositeExpression * ce = dynamic_cast< CompositeExpression * >( andConds[i] );
            if ( ce ) {
                std::string& op = ce->op;

                double rightValue = ce->right->evaluate();

                std::stringstream predicateName;
                if ( op == ">" ) predicateName << "GT";
                else if ( op == ">=" ) predicateName << "GET";
                else if ( op == "<" ) predicateName << "LT";
                else if ( op == "<=" ) predicateName << "LET";
                predicateName << rightValue;

                FunctionExpression * fe = dynamic_cast< FunctionExpression * >( ce->left );
                StringVec sv( 1, fe->fun->name + "-VALUE" );
                d->createPredicate( predicateName.str(), sv );

                andConds[i] = new Ground( predicateName.str(), IntVec( 1, ta->params.size() - 3 ) );

                for ( auto it = finalPossibleValues.begin(); it != finalPossibleValues.end(); ++it ) {
                    std::stringstream valueStr; valueStr << fe->fun->name << *it;
                    StringVec sv( 1, valueStr.str() );

                    if ( op == ">" && *it > rightValue ) {
                        ins->addInit( predicateName.str(), sv );
                    }
                    else if ( op == ">=" && *it >= rightValue ) {
                        ins->addInit( predicateName.str(), sv );
                    }
                    else if ( op == "<" && *it < rightValue ) {
                        ins->addInit( predicateName.str(), sv );
                    }
                    else if ( op == "<=" && *it <= rightValue ) {
                        ins->addInit( predicateName.str(), sv );
                    }
                }
            }
        }
    }
}

void removeFunctionModifiers( TemporalAction * ta ) {
    And * effStart = dynamic_cast< And * >( ta->eff );
    CondVec newAnd;
    for ( unsigned i = 0; i < effStart->conds.size(); ++i ) {
        if ( !dynamic_cast< FunctionModifier * >( effStart->conds[i] ) ) {
            newAnd.push_back( effStart->conds[i] );
        }
    }
    effStart->conds = newAnd;

    And * effEnd = dynamic_cast< And * >( ta->eff_e );
    CondVec newAndEnd;
    for ( unsigned i = 0; i < effEnd->conds.size(); ++i ) {
        if ( !dynamic_cast< FunctionModifier * >( effEnd->conds[i] ) ) {
            newAndEnd.push_back( effEnd->conds[i] );
        }
    }
    effEnd->conds = newAndEnd;
}

void getVariableFunctionsValues( Domain * d, Instance * ins ) {
    // add type FUNCTION-VALUE, which will be used by the previously increased
    // decreased values (e.g. energy)
    d->createType( "FUNCTION-VALUE" );

    // create SUB predicate with 3 FUNCTION-VALUE as parameter
    d->createPredicate( "SUB", StringVec( 3, "FUNCTION-VALUE" ) );

    // get the functions which are being increased or decreased
    LiftedActionPairVec variableFunctions = getVariableFunctions( d );

    std::map< std::string, std::map< std::string, double > > functionObjToValues;

    for ( unsigned i = 0; i < variableFunctions.size(); ++i ) {
        LiftedActionPair& lap = variableFunctions[i];
        Lifted * l = lap.first; // lifted function
        ActionFunctionModPairVec& actions = lap.second;

        // add custom type
        std::string variableFunctionTypeName = l->name + "-VALUE";
        d->createType( variableFunctionTypeName, "FUNCTION-VALUE" );

        // get list of types of the lifted function and get possible values for arguments of the function
        StringVec functionTypes = d->typeList( l );
        StringDVec objectSets = getObjectSetsForTypes( functionTypes, d ); // all possible instantiations of the function

        std::set< double > finalPossibleValues; // all possible values for the function 'l'
        for ( unsigned j = 0; j < objectSets.size(); ++j ) {
            std::set< double > possibleValues = getPossibleValues( l, objectSets[j], actions, d, ins );
            for ( auto it = possibleValues.begin(); it != possibleValues.end(); ++it ) {
                finalPossibleValues.insert( *it );
            }
        }

        // for each final possible value, add an object representing it!
        std::map< std::string, double > correspondences;
        for ( auto it = finalPossibleValues.begin(); it != finalPossibleValues.end(); ++it ) {
            std::stringstream ss;
            ss << l->name << *it;
            ins->addObject( ss.str(), variableFunctionTypeName );
            correspondences[ss.str()] = *it;
        }
        functionObjToValues[l->name] = correspondences;

        // add predicate for representing the value of the function
        StringVec functionPredParams = functionTypes;
        functionPredParams.push_back( variableFunctionTypeName );
        std::string currentFunctionValue = "CURRENT-" + l->name + "-VALUE";
        d->createPredicate( currentFunctionValue, functionPredParams );

        // create a CURRENT-VALUE for each function
        for ( unsigned j = 0; j < objectSets.size(); ++j ) {
            GroundFunc<double> * gf = getGroundFunc( l, objectSets[j], ins );
            std::stringstream ss; ss << l->name << gf->value;
            StringVec sv = d->objectList( gf );
            sv.push_back( ss.str() );
            ins->addInit( currentFunctionValue, sv );
        }

        // modification of current actions
        for ( unsigned j = 0; j < actions.size(); ++j ) {
            Action * a = actions[j].first;
            FunctionModifier * fm = actions[j].second;

            unsigned numActionParams = a->params.size();
            unsigned numNewParams = 3; // current, new and incremental function values

            StringVec sv;
            for ( unsigned i = 0; i < numNewParams; ++i ) {
                sv.push_back( variableFunctionTypeName );
            }
            a->addParams( d->convertTypes( sv ) );

            // add predicate for representing ability to increase current function value by a certain quantity
            StringVec increasePredParams = functionTypes;
            increasePredParams.push_back( l->name + "-VALUE" ); // current value
            increasePredParams.push_back( l->name + "-VALUE" ); // increment value
            d->createPredicate( "INCREASE-" + l->name + "-" + a->name, increasePredParams );

            // add preconditions
            Ground * modifiedGround = fm->modifiedGround;
            IntVec currentValueParams = IntVec( modifiedGround->params ); // copy params of modified ground
            currentValueParams.push_back( numActionParams ); // numActionParams = location of current value!
            d->addPre( false, a->name, currentFunctionValue, currentValueParams );
            d->addPre( false, a->name, "SUB", incvec( numActionParams, numActionParams + numNewParams ) );

            IntVec increaseGroundedParams = currentValueParams;
            increaseGroundedParams.push_back( numActionParams + 2 );
            d->addPre( false, a->name, "INCREASE-" + l->name + "-" + a->name, increaseGroundedParams );

            // add effects
            TemporalAction * ta = dynamic_cast< TemporalAction * >( a );
            if ( ta ) {
                translateInequalities( ta, d, ins, finalPossibleValues );

                if ( ta->eff_e == 0 ) ta->eff_e = new And;
                And * andc = dynamic_cast< And * >( ta->eff_e );
                andc->add( new Not( d->ground( currentFunctionValue, currentValueParams ) ) );

                IntVec nextValueParams = IntVec( modifiedGround->params );
                nextValueParams.push_back( numActionParams + 1);
                andc->add( d->ground( currentFunctionValue, nextValueParams ) );
            }

            // addActionDurationFunction( a, d, ins );

            addIncrementRelatedPredicates( finalPossibleValues, a, objectSets, l, fm, ins );
        }
    }

    // computation of durations for the actions
    for ( unsigned i = 0; i < d->actions.size(); ++i ) {
        TemporalAction * ta = get( i );
        IntSet durationIndices = ta->durationExpr->params();
        if ( !durationIndices.empty() ) {
            // get functions in the duration expression that can vary (i.e. are incremented or decremented)
            LiftedVec durationVariableFunctions;
            getVariableFunctionsFromExpression( ta->durationExpr, variableFunctions, durationVariableFunctions );

            // create function for representing cost of the action
            StringVec v;
            for ( auto j = durationIndices.begin(); j != durationIndices.end(); ++j ) {
                v.push_back( d->types[ta->params[*j]]->name );
            }
            for ( unsigned j = 0; j < durationVariableFunctions.size(); ++j ) {
                v.push_back( durationVariableFunctions[j]->name + "-VALUE" );
            }
            Lifted * actionCostFunction = d->createFunction( ta->name + "-COST", -1, v );

            // add comment
            StringDVec objectSets = getObjectSetsForTypes( v, d );
            for ( unsigned j = 0; j < objectSets.size(); ++j ) {

                StringVec fixedValues, variableValues, allValues;
                for ( unsigned k = 0; k < v.size(); ++k ) {
                    if ( k < durationIndices.size() ) {
                        fixedValues.push_back( objectSets[j][k] );
                    }
                    else {
                        variableValues.push_back( objectSets[j][k] );
                    }
                    allValues.push_back( objectSets[j][k] );
                }

                // somehow assuming that we just have ONE variable function!
                DoubleVec initialValues;
                std::vector< GroundFunc<double>* > groundFuncs;
                for ( unsigned k = 0; k < durationVariableFunctions.size(); ++k ) {
                    GroundFunc<double> * gf = getGroundFunc( durationVariableFunctions[k], fixedValues, ins );
                    groundFuncs.push_back( gf );
                    initialValues.push_back( gf->value );
                }

                for ( unsigned k = 0; k < durationVariableFunctions.size(); ++k ) {
                    std::string& functionName = durationVariableFunctions[k]->name;
                    std::string& functionValue = variableValues[k];
                    double transformedValue = functionObjToValues[functionName][functionValue];
                    groundFuncs[k]->value = transformedValue;

                    double cost = 10000 * ta->durationExpr->evaluate( *ins, allValues );
                    ins->addInit( ta->name + "-COST", cost, allValues );
                }

                for ( unsigned k = 0; k < durationVariableFunctions.size(); ++k ) {
                    groundFuncs[k]->value = initialValues[k];
                }
            }

            ParamCond * newDurationCond = new Lifted(actionCostFunction);
            newDurationCond->params[0] = 0; // WRONG, make more flexible...
            unsigned originalNumberParams = ta->params.size() - 3 * durationVariableFunctions.size();
            for ( unsigned j = 1; j < newDurationCond->params.size(); ++j ) {
                newDurationCond->params[j] = originalNumberParams + (j - 1) * 3;
            }

            ta->durationExpr = new FunctionExpression( newDurationCond );
        }

        removeFunctionModifiers( ta );
    }

    std::cout << *d;
    std::cerr << *ins;
}

#endif
