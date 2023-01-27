// To check for memory leaks:
// valgrind --leak-check=yes bin/compileTempoParallelAlt ../domains/andersAllenAlgebra/domain/domain.pddl ../domains/andersAllenAlgebra/problems/pfile70.pddl 2

#include "compile.h"

int main( int argc, char *argv[] ) {
	if ( argc < 4 ) {
		std::cout << "Usage: ./compileTempoParallel <domain.pddl> <task.pddl> <bound>\n";
		std::cout << "Writes domain file to standard output and instance file to standard error\n";
		exit( 1 );
	}

	d = new Domain( argv[1] );
	ins = new Instance( *d, argv[2] );

	// maximum count is larger than 1 !!! (else no parallel events are necessary)

	unsigned maxCount;
	std::istringstream is( argv[3] );
	is >> maxCount;

	// RIGHT NOW ASSUMES POSITIVE PRECONDITIONS OVER ALL !!!

	// include all contexts
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		for ( unsigned j = 0; j < get( i )->pre_o->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( get( i )->pre_o->conds[j] );
			if ( pre ) {
				int k = d->preds.index( pre->name );
				if ( k >= 0 && isDel( k ) ) pres.insert( k );
			}
		}
		for ( unsigned j = 0; j < ( ( And * )get( i )->eff )->conds.size(); ++j ) {
			Not * n = dynamic_cast< Not * >( ( ( And * )get( i )->eff )->conds[j] );
			if ( n ) effs.insert( d->preds.index( n->cond->name ) );
			else effs.insert( d->preds.index( ( ( Ground * )( ( And * )get( i )->eff )->conds[j] )->name ) );
		}
		for ( unsigned j = 0; j < get( i )->eff_e->conds.size(); ++j ) {
			Not * n = dynamic_cast< Not * >( get( i )->eff_e->conds[j] );
			if ( n ) effs.insert( d->preds.index( n->cond->name ) );
			else effs.insert( d->preds.index( ( ( Ground * )get( i )->eff_e->conds[j] )->name ) );
		}
	}

	// For each temporal action, check if duration depends on parameters
	SetVec durationIndices( d->actions.size() );
	for ( unsigned i = 0; i < d->actions.size(); ++i )
		durationIndices[i] = get( i )->durationExpr->params();

	// Create classical domain
	cd = new Domain;
	cd->name = d->name;
	cd->condeffects = cd->typed = cd->costs = true;
	cd->cons = d->cons;
	cd->equality = d->equality;

	// Add types
	cd->setTypes( d->copyTypes() );
	cd->createType( "COUNTER" ); // new

	cd->createPredicate( "END-PHASE" );
	cd->createPredicate( "STACK-PHASE" );
	cd->createPredicate( "START-PHASE" );

	// Add predicates
	for ( unsigned i = 0; i < d->preds.size(); ++i ) {
		cd->createPredicate( d->preds[i]->name, d->typeList( d->preds[i] ) );
		if ( effs.find( i ) != effs.end() ) {
			cd->createPredicate( "PRE-" + d->preds[i]->name, d->typeList( d->preds[i] ) );
			cd->createPredicate( "LOCK-" + d->preds[i]->name, d->typeList( d->preds[i] ) );
		}
	}

	// represent the "stack size", i.e. number of active, starting, ending actions
	StringVec acts, starts, ends;
	for ( unsigned i = 0; i <= maxCount; ++i ) {
		std::stringstream ss;
		ss << "ACTIVE" << i;
		acts.push_back( ss.str() );
		cd->createPredicate( acts[i] );

		ss.str( "" );
		ss << "STARTING" << i;
		starts.push_back( ss.str() );
		cd->createPredicate( starts[i] );

		ss.str( "" );
		ss << "ENDING" << i;
		ends.push_back( ss.str() );
		cd->createPredicate( ends[i] );
	}

	// represent that actions are active
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		cd->createPredicate( "ACTIVE-" + d->actions[i]->name, d->typeList( d->actions[i] ) );
		cd->createPredicate( "STARTING-" + d->actions[i]->name, d->typeList( d->actions[i] ) );
		cd->createPredicate( "ENDING-" + d->actions[i]->name, d->typeList( d->actions[i] ) );
	}

	// associate counts with each context
	StringDVec counts( d->preds.size() );
	for ( IntSet::iterator i = pres.begin(); i != pres.end(); ++i )
		for ( unsigned j = 0; j <= maxCount; ++j ) {
			std::stringstream ss;
			ss << "COUNT" << j << "-" << d->preds[*i]->name;
			counts[*i].push_back( ss.str() );
			cd->createPredicate( counts[*i][j], d->typeList( d->preds[*i] ) );
		}

	cd->createPredicate( "ENDING-COUNTER", StringVec( 1, "COUNTER" ) );
	cd->createPredicate( "CONSEC-COUNT", StringVec( 2, "COUNTER" ) );

	cd->createPredicate( "STARTING-ALLOWED" );

	// Currently does NOT add functions
//	for ( unsigned i = 0; i < d->funcs.size(); ++i )
//		cd->createFunction( d->funcs[i]->name, d->funcs[i]->returnType, d->typeList( d->funcs[i] ) );

	// Add total-cost
	cd->createFunction( "TOTAL-COST", -1 );

	// Add parameter-dependent costs
	for ( unsigned i = 0; i < d->actions.size(); ++i )
		if ( durationIndices[i].size() ) {
			StringVec v;
			for ( IntSet::iterator j = durationIndices[i].begin(); j != durationIndices[i].end(); ++j )
				v.push_back( d->types[d->actions[i]->params[*j]]->name );
			cd->createFunction( d->actions[i]->name + "-COST", -1, v );
		}

	// Add actions
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		// Push action
		std::string name = "PUSH-" + d->actions[i]->name;
		unsigned size = d->actions[i]->params.size();
		Action * push = cd->createAction( name, d->typeList( d->actions[i] ) );

		// copy the preconditions at start
		cd->setPre( name, d->actions[i]->pre );

		// stack phase
		cd->addPre( 0, name, "STACK-PHASE" );

		// action is not already active
		cd->addPre( 1, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

		// the limit on active actions has not been reached
		cd->addPre( 1, name, acts[maxCount] );

		// all counts are 0 for fluents that become deleted
		GroundVec del = d->actions[i]->deleteEffects();
		for ( unsigned j = 0; j < del.size(); ++j ) {
			int x = d->preds.index( del[j]->name );
			if ( pres.find( x ) != pres.end() )
				cd->addPre( 0, name, counts[x][0], del[j]->params );
		}

		// concurrency checks
		for ( unsigned j = 0; j < ( ( And * )get( i )->pre )->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( ( ( And * )get( i )->pre )->conds[j] );
			if ( !pre ) pre = ( ( Not * )( ( And * )get( i )->pre )->conds[j] )->cond;
			if ( effs.find( d->preds.index( pre->name ) ) != effs.end() )
				cd->addPre( 0, name, "LOCK-" + pre->name, pre->params );
		}
		for ( unsigned j = 0; j < ( ( And * )get( i )->eff )->conds.size(); ++j ) {
			Ground * g = dynamic_cast< Ground * >( ( ( And * )get( i )->eff )->conds[j] );
			if ( !g ) g = ( ( Not * )( ( And * )get( i )->eff )->conds[j] )->cond;
			if ( effs.find( d->preds.index( g->name ) ) != effs.end() ) {
				cd->addPre( 0, name, "PRE-" + g->name, g->params );
				cd->addPre( 0, name, "LOCK-" + g->name, g->params );
			}
		}

		// copy the effects at start
		cd->setEff( name, d->actions[i]->eff );

		// action is starting
		cd->addEff( 0, name, "STARTING-" + d->actions[i]->name, incvec( 0, size ) );

		// concurrency updates
		for ( unsigned j = 0; j < ( ( And * )get( i )->pre )->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( ( ( And * )get( i )->pre )->conds[j] );
			if ( !pre ) pre = ( ( Not * )( ( And * )get( i )->pre )->conds[j] )->cond;
			if ( effs.find( d->preds.index( pre->name ) ) != effs.end() )
				cd->addEff( 1, name, "PRE-" + pre->name, pre->params );
		}
		for ( unsigned j = 0; j < ( ( And * )get( i )->eff )->conds.size(); ++j ) {
			Ground * g = dynamic_cast< Ground * >( ( ( And * )get( i )->eff )->conds[j] );
			if ( !g ) g = ( ( Not * )( ( And * )get( i )->eff )->conds[j] )->cond;
			if ( effs.find( d->preds.index( g->name ) ) != effs.end() )
				cd->addEff( 1, name, "LOCK-" + g->name, g->params );
		}

		// add conditional effects
		for ( unsigned j = 0; j < maxCount; ++j ) {
			When * w = new When;
			w->pars = new Ground( cd->preds.get( starts[j] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( starts[j] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( starts[j + 1] ) ) );
			( ( And * )push->eff )->add( w );

			w = new When;
			w->pars = new Ground( cd->preds.get( acts[j] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( acts[j] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( acts[j + 1] ) ) );
			( ( And * )push->eff )->add( w );
		}

		if ( durationIndices[i].size() ) {
			IntVec pars( durationIndices[i].begin(), durationIndices[i].end() );
			cd->addCost( name, d->actions[i]->name + "-COST", pars );
		}
		else cd->addCost( name, 10000 * get( i )->durationExpr->evaluate( *ins, StringVec() ) );

		cd->addEff( 0, name, "STARTING-ALLOWED" );

		// Launch action
		name = "LAUNCH-" + d->actions[i]->name;
		Action * launch = cd->createAction( name, d->typeList( d->actions[i] ) );

		// start phase
		cd->addPre( 0, name, "START-PHASE" );

		// action is starting
		cd->addPre( 0, name, "STARTING-" + d->actions[i]->name, incvec( 0, size ) );

		// all preconditions over all are satisfied
		for ( unsigned j = 0; j < get( i )->pre_o->conds.size(); ++j ) {
//			Ground * g = ( Ground * )get( i )->pre_o->conds[j];
//			cd->addPre( 0, name, g->name, g->params );
			( ( And * )launch->pre )->add( get( i )->pre_o->conds[j]->copy( *cd ) );
		}

		// action becomes active
		cd->addEff( 1, name, "STARTING-" + d->actions[i]->name, incvec( 0, size ) );
		cd->addEff( 0, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

		// add conditional effects
		for ( unsigned j = 0; j < maxCount; ++j ) {
			When * w = new When;
			w->pars = new Ground( cd->preds.get( starts[j + 1] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( starts[j + 1] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( starts[j] ) ) );
			( ( And * )launch->eff )->add( w );
		}

		// increment the counts for all context fluents
		for ( unsigned j = 0; j < get( i )->pre_o->conds.size(); ++j ) {
			Ground * g = dynamic_cast< Ground * >( get( i )->pre_o->conds[j] );
			if ( g && pres.find( d->preds.index( g->name ) ) != pres.end() ) {
				int x = d->preds.index( g->name );
				for ( unsigned k = 0; k < maxCount; ++k ) {
					When * w = new When;
					w->pars = new Ground( cd->preds.get( counts[x][k] ), g->params );
					w->cond = new And;
					( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( counts[x][k] ), g->params ) ) );
					( ( And * )w->cond )->add( new Ground( cd->preds.get( counts[x][k + 1] ), g->params ) );
					( ( And * )launch->eff )->add( w );
				}
			}
		}

		cd->addCost( name, 1 );

		// Pop action
		name = "POP-" + d->actions[i]->name;
		Action * pop = cd->createAction( name, d->typeList( d->actions[i] ) );

		// copy preconditions at end
		cd->setPre( name, get( i )->pre_e );

		// end phase
		cd->addPre( 0, name, "END-PHASE" );

		// action is active
		cd->addPre( 0, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

		// action becomes ending
		cd->addEff( 1, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );
		cd->addEff( 0, name, "ENDING-" + d->actions[i]->name, incvec( 0, size ) );

		// decrement the counts for all context fluents
		for ( unsigned j = 0; j < get( i )->pre_o->conds.size(); ++j ) {
			Ground * g = dynamic_cast< Ground * >( get( i )->pre_o->conds[j] );
			if ( g && pres.find( d->preds.index( g->name ) ) != pres.end() ) {
				int x = d->preds.index( g->name );
				for ( unsigned k = 0; k < maxCount; ++k ) {
					When * w = new When;
					w->pars = new Ground( cd->preds.get( counts[x][k + 1] ), g->params );
					w->cond = new And;
					( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( counts[x][k + 1] ), g->params ) ) );
					( ( And * )w->cond )->add( new Ground( cd->preds.get( counts[x][k] ), g->params ) );
					( ( And * )pop->eff )->add( w );
				}
			}
		}

		// add conditional effects
		for ( unsigned j = 0; j < maxCount; ++j ) {
			When * w = new When;
			w->pars = new Ground( cd->preds.get( acts[j + 1] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( acts[j + 1] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( acts[j] ) ) );
			( ( And * )pop->eff )->add( w );

			w = new When;
			w->pars = new Ground( cd->preds.get( ends[j] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( ends[j] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( ends[j + 1] ) ) );
			( ( And * )pop->eff )->add( w );
		}

		cd->addCost( name, 1 );

		// End action
		name = "FINISH-" + d->actions[i]->name;
		Action * finish = cd->createAction( name, d->typeList( d->actions[i] ) );

		// stack phase
		cd->addPre( 0, name, "STACK-PHASE" );

		// action is ending
		cd->addPre( 0, name, "ENDING-" + d->actions[i]->name, incvec( 0, size ) );

		// all counts are 0 for fluents that become deleted
		for ( unsigned j = 0; j < get( i )->eff_e->conds.size(); ++j ) {
			Not * n = dynamic_cast< Not * >( get( i )->eff_e->conds[j] );
			if ( n && pres.find( d->preds.index( n->cond->name ) ) != pres.end() ) {
				bool b = includes( 0, n->cond, get( i )->pre_o );
				cd->addPre( 0, name, counts[d->preds.index( n->cond->name )][b], n->cond->params );
			}
		}

		// concurrency checks
		for ( unsigned j = 0; j < get( i )->pre_e->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( get( i )->pre_e->conds[j] );
			if ( !pre ) pre = ( ( Not * )get( i )->pre_e->conds[j] )->cond;
			if ( effs.find( d->preds.index( pre->name ) ) != effs.end() ) {
				cd->addPre( 0, name, "LOCK-" + pre->name, pre->params );
			}
		}

		for ( unsigned j = 0; j < get( i )->eff_e->conds.size(); ++j ) {
			Ground * g = dynamic_cast< Ground * >( get( i )->eff_e->conds[j] );
			if ( !g ) g = ( ( Not * )get( i )->eff_e->conds[j] )->cond;
			if ( effs.find( d->preds.index( g->name ) ) != effs.end() ) {
				cd->addPre( 0, name, "PRE-" + g->name, g->params );
				cd->addPre( 0, name, "LOCK-" + g->name, g->params );
			}
		}

		// copy effects at end
		cd->setEff( name, get( i )->eff_e );

		// action becomes free
		cd->addEff( 1, name, "ENDING-" + d->actions[i]->name, incvec( 0, size ) );

		// add auxiliary fluents
		for ( unsigned j = 0; j < get( i )->pre_e->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( get( i )->pre_e->conds[j] );
			if ( !pre ) pre = ( ( Not * )get( i )->pre_e->conds[j] )->cond;
			if ( effs.find( d->preds.index( pre->name ) ) != effs.end() ) {
				cd->addEff( 1, name, "PRE-" + pre->name, pre->params );
			}
		}
		for ( unsigned j = 0; j < get( i )->eff_e->conds.size(); ++j ) {
			Ground * g = dynamic_cast< Ground * >( get( i )->eff_e->conds[j] );
			if ( !g ) g = ( ( Not * )get( i )->eff_e->conds[j] )->cond;
			cd->addEff( 1, name, "LOCK-" + g->name, g->params );
		}

		// add conditional effects
		for ( unsigned j = 0; j < maxCount; ++j ) {
			When * w = new When;
			w->pars = new Ground( cd->preds.get( ends[j + 1] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( ends[j + 1] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( ends[j] ) ) );
			( ( And * )finish->eff )->add( w );
		}

		cd->addEff( 0, name, "STARTING-ALLOWED" );

		cd->addCost( name, 1 );
	}

	// Action that starts the end phase
	Action * endphase = cd->createAction( "ENDING-PHASE" , StringVec( 2, "COUNTER" ) );

	// We are currently in the start phase
	cd->addPre( 0, "ENDING-PHASE", "START-PHASE" );

	// No actions are starting
	cd->addPre( 0, "ENDING-PHASE", starts[0] );

	cd->addPre( 0, "ENDING-PHASE", "ENDING-COUNTER", IntVec( 1, 0 ) );

	cd->addPre( 0, "ENDING-PHASE", "CONSEC-COUNT", incvec( 0, 2 ) );

	// All auxiliary fluents are false
	for ( IntSet::iterator i = effs.begin(); i != effs.end(); ++i ) {
		And * a = new And;
		IntVec v = incvec( 2, 2 + d->preds[*i]->params.size() );
		a->add( new Ground( cd->preds.get( "PRE-" + d->preds[*i]->name ), v ) );
		a->add( new Ground( cd->preds.get( "LOCK-" + d->preds[*i]->name ), v ) );

		Forall * f = new Forall;
		f->params = d->preds[*i]->params;
		f->cond = a;

		( ( And * )endphase->pre )->add( f );
	}

	// We move to the end phase
	cd->addEff( 1, "ENDING-PHASE", "START-PHASE" );
	cd->addEff( 0, "ENDING-PHASE", "END-PHASE" );

	cd->addEff( 1, "ENDING-PHASE", "ENDING-COUNTER", IntVec( 1, 0 ) );
	cd->addEff( 0, "ENDING-PHASE", "ENDING-COUNTER", IntVec( 1, 1 ) );

	cd->addCost( "ENDING-PHASE", 1 );

	// Action that starts the stack phase
	cd->createAction( "STACKING-PHASE" );

	// We are currently in the end phase
	cd->addPre( 0, "STACKING-PHASE", "END-PHASE" );

	// We move to the stack phase
	cd->addEff( 1, "STACKING-PHASE", "END-PHASE" );
	cd->addEff( 0, "STACKING-PHASE", "STACK-PHASE" );

	cd->addCost( "STACKING-PHASE", 1 );

	// Action that starts the start phase
	cd->createAction( "STARTING-PHASE" );

	// We are currently in the stack phase
	cd->addPre( 0, "STARTING-PHASE", "STACK-PHASE" );

	// No actions are ending
	cd->addPre( 0, "STARTING-PHASE", ends[0] );

	// At least a push or finish action must have been done
	cd->addPre( 0, "STARTING-PHASE", "STARTING-ALLOWED" );

	// We move to the start phase
	cd->addEff( 1, "STARTING-PHASE", "STACK-PHASE" );
	cd->addEff( 0, "STARTING-PHASE", "START-PHASE" );

	// Another push or finish will have to be done to do start-phase again
	cd->addEff( 1, "STARTING-PHASE", "STARTING-ALLOWED" );

	cd->addCost( "STARTING-PHASE", 1 );

	// Reset actions
	for ( IntSet::iterator i = effs.begin(); i != effs.end(); ++i ) {
		std::string name = "RESET-" + d->preds[*i]->name;
		unsigned size = d->preds[*i]->params.size();
		cd->createAction( name, d->typeList( d->preds[*i] ) );
		cd->addPre( 0, name, "START-PHASE" );
		cd->addEff( 0, name, "PRE-" + d->preds[*i]->name, incvec( 0, size ) );
		cd->addEff( 0, name, "LOCK-" + d->preds[*i]->name, incvec( 0, size ) );
	}

	std::cout << *cd;

	cins = new Instance( *cd );
	cins->name = ins->name;

	cins->metric = true;

	unsigned numCounters = 10;

	for ( unsigned i = 0; i < numCounters; ++i ) {
		std::string name = "C" + std::to_string(i);
		cins->addObject( name, "COUNTER" );
	}

	// create initial state
	for ( unsigned i = 0; i < ins->init.size(); ++i )
		if ( d->preds.index( ins->init[i]->name ) >= 0 )
			cins->addInit( ins->init[i]->name, d->objectList( ins->init[i] ) );

	// actives, starts, ends are empty
	cins->addInit( acts[0] );
	cins->addInit( starts[0] );
	cins->addInit( ends[0] );

	// we are in the initial phase
	cins->addInit( "START-PHASE" );

	Type * counterType = cd->types.get( "COUNTER" );
	cins->addInit( "ENDING-COUNTER", StringVec( 1, counterType->objects[0] ) );

	for ( unsigned i = 0; i < counterType->noObjects(); ++i ) {
		StringVec v;
		unsigned c1 = i % counterType->noObjects();
		unsigned c2 = ( i + 1 ) % counterType->noObjects();
		v.push_back( counterType->objects[c1] );
		v.push_back( counterType->objects[c2] );
		cins->addInit( "CONSEC-COUNT", v );
	}

	// counts are 0
	for ( IntSet::iterator i = pres.begin(); i != pres.end(); ++i ) {
		StringVec pars( d->preds[*i]->params.size() );
		recurse( d->preds[*i], 0, pars, "COUNT0-" );
	}

	for ( IntSet::iterator i = effs.begin(); i != effs.end(); ++i ) {
		StringVec pars( d->preds[*i]->params.size() );
		recurse( d->preds[*i], 0, pars, "PRE-" );
		recurse( d->preds[*i], 0, pars, "LOCK-" );
	}

	// add function values
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		if ( durationIndices[i].size() ) {
			StringVec pars( d->actions[i]->params.size() );
			IntVec indices( durationIndices[i].begin(), durationIndices[i].end() );
			recCost( 0, pars, indices, i );
		}
	}

	cins->addInit( "TOTAL-COST", 0.0 );

	// create goal state
	for ( unsigned i = 0; i < ins->goal.size(); ++i )
		cins->addGoal( ins->goal[i]->name, d->objectList( ins->goal[i] ) );

	// actives are empty
	cins->addGoal( acts[0] );

	// we are in the start phase
	cins->addGoal( "START-PHASE" );

	std::cerr << *cins;

	delete cins;
	delete cd;
	delete ins;
	delete d;
}
