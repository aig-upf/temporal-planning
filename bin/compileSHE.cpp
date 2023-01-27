// To check for memory leaks:
// valgrind --leak-check=yes bin/compileSHE ../domains/tempo-sat/Mapanalyser/domain/domain.pddl ../domains/tempo-sat/Mapanalyser/problems/pfile3-4-2-0-1.pddl

#include "compile.h"

int main( int argc, char *argv[] ) {
	if ( argc < 3 ) {
		std::cout << "Usage: ./compileSHE <domain.pddl> <task.pddl>\n";
		std::cout << "Writes domain file to standard output and instance file to standard error\n";
		exit( 1 );
	}

	d = new Domain( argv[1] );
	ins = new Instance( *d, argv[2] );

	// Generate dependency graph among actions and identify durations and depths

	graph g( d->actions.size() );
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		for ( unsigned j = 0; j < d->actions.size(); ++j )
			if ( i != j )
				detectDependency( i, j, g );
	}
	g.computeDurations();
	g.computeDepths();

	// Identify contexts that are threatened by contents
	// RIGHT NOW PRE & EFF ASSUME POSITIVE PRECONDITIONS !!!

	int maxCount = 0;
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		maxCount = MAX( maxCount, *g.depths[i].rbegin() );
		for ( unsigned j = 0; j < get( i )->pre_o->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( get( i )->pre_o->conds[j] );
			if ( pre ) {
				int k = d->preds.index( pre->name );
				if ( k >= 0 && isPre( i, k, g ) )
					pres.insert( k );
			}
		}
	}

	// Create classical domain
	cd = new Domain;
	cd->name = d->name;
	cd->equality = d->equality;
	cd->condeffects = cd->typed = true;
	cd->cons = d->cons || ( maxCount && pres.size() ) || g.subtractionPairs.size();
	cd->equality = d->equality;

	// Add types
	cd->setTypes( d->copyTypes() );
	if ( maxCount && pres.size() ) cd->createType( "COUNT" );
	if ( g.subtractionPairs.size() ) cd->createType( "TIME" );

	// Add constants
	StringVec counts;
	for ( int i = 0; maxCount && pres.size() && i <= maxCount; ++i ) {
		std::stringstream ss;
		ss << "COUNT" << i;
		counts.push_back( ss.str() );
		cd->createConstant( counts[i], "COUNT" );
	}
	StringVec times;
	for ( unsigned i = 0; g.subtractionPairs.size() && i < g.durationMap.size(); ++i ) {
		std::stringstream ss;
		ss << "TIME" << i;
		times.push_back( ss.str() );
		cd->createConstant( times[i], "TIME" );
	}

	// Add predicates
	for ( unsigned i = 0; i < d->preds.size(); ++i ) {
		StringVec pars = d->typeList( d->preds[i] );
		cd->createPredicate( d->preds[i]->name, pars );
		if ( pres.find( i ) != pres.end() ) {
			pars.push_back( "COUNT" );
			cd->createPredicate( "COUNT-" + d->preds[i]->name, pars );
		}
	}

	StringVec stacks( 1, "EMPTYSTACK" );
	cd->createPredicate( stacks[0] );
	for ( int i = 1; i <= maxCount; ++i ) {
		std::stringstream ss;
		ss << "STACK" << i;
		stacks.push_back( ss.str() );
		cd->createPredicate( stacks[i] );
		cd->createPredicate( stacks[i] + "-REMAINING", StringVec( 1, "TIME" ) );
		cd->createPredicate( stacks[i] + "-INCREASE", StringVec( 1, "TIME" ) );
		cd->createPredicate( stacks[i] + "-DECREASE" );
		cd->createPredicate( stacks[i] + "-CONTINUE", StringVec( 1, "TIME" ) );
	}

	for ( unsigned i = 0; i < d->actions.size(); ++i )
		for ( IntSet::iterator j = g.depths[i].begin(); g.outgoing( i ) && j != g.depths[i].end(); ++j ) {
			std::stringstream ss;
			ss << "STACK" << *j + 1 << "-" << d->actions[i]->name;
			cd->createPredicate( ss.str(), d->typeList( d->actions[i] ) );
		}

	if ( counts.size() )
		cd->createPredicate( "CONSECUTIVE", StringVec( 2, "COUNT" ) );

	if ( times.size() )
		cd->createPredicate( "SUBTRACT", StringVec( 3, "TIME" ) );

	// Currently does NOT add functions
//	for ( unsigned i = 0; i < d->funcs.size(); ++i )
//		cd->createFunction( d->funcs[i]->name, d->funcs[i]->returnType, d->typeList( d->funcs[i] ) );

	// Add actions
	for ( unsigned i = 0; i < d->actions.size(); ++i )
		// DOES NOT CURRENTLY IMPLEMENT COUNT MECHANISM !!!
		for ( IntSet::iterator j = g.depths[i].begin(); j != g.depths[i].end(); ++j )
			if ( g.outgoing( i ) ) {
				// Action is an envelope: compile into push and pop action

				// Push action
				std::string name = "PUSH-" + d->actions[i]->name;
				unsigned size = d->actions[i]->params.size();
				Action * push = cd->createAction( name, d->typeList( d->actions[i] ) );

				//std::cout << "Creating action " << name << "\n";

				// ONLY DEALS WITH POSITIVE PRECONDITIONS HERE !!!
				cd->setPre( name, d->actions[i]->pre );
				for ( unsigned k = 0; k < get( i )->pre_o->conds.size(); ++k ) {
					Ground * pre = dynamic_cast< Ground * >( get( i )->pre_o->conds[k] );
					if ( pre && !includes( 0, pre, ( And * )get( i )->pre ) &&
					            !includes( 0, pre, ( And * )get( i )->eff ) )
						cd->addPre( 0, name, pre->name, pre->params );
					// if precon is not positive, just copy it
					if ( !pre ) ( ( And * )push->pre )->add( get( i )->pre_o->conds[k]->copy( *cd ) );
				}
				cd->addPre( 0, name, stacks[*j] );

				cd->setEff( name, d->actions[i]->eff );
				cd->addEff( 1, name, stacks[*j] );
				if ( *j ) cd->addEff( 0, name, stacks[*j] + "-INCREASE", IntVec( 1, cd->constantIndex( times[g.durationMap[d->actions[i]->duration()]], "TIME" ) ) );
				else cd->addEff( 0, name, stacks[1] );
				cd->addEff( 0, name, stacks[*j + 1] + "-" + d->actions[i]->name, incvec( 0, size ) );
				cd->addEff( 0, name, stacks[*j + 1] + "-REMAINING", IntVec( 1, cd->constantIndex( times[g.durationMap[d->actions[i]->duration()]], "TIME" ) ) );

				// Pop action
				name = "POP-" + d->actions[i]->name;
				cd->createAction( name, d->typeList( d->actions[i] ) );

				//std::cout << "Creating action " << name << "\n";

				// ONLY DEALS WITH POSITIVE PRECONDITIONS HERE !!!
				cd->setPre( name, get( i )->pre_e );
				for ( unsigned k = 0; k < get( i )->pre_o->conds.size(); ++k ) {
					Ground * h = dynamic_cast< Ground * >( get( i )->pre_o->conds[k] );
					if ( h && !includes( 0, h, get( i )->pre_e ) )
						cd->addPre( 0, name, h->name, h->params );
				}
				cd->addPre( 0, name, stacks[*j + 1] );
				cd->addPre( 0, name, stacks[*j + 1] + "-" + d->actions[i]->name, incvec( 0, size ) );

				cd->setEff( name, get( i )->eff_e );
				cd->addEff( 0, name, stacks[*j + 1] + "-DECREASE" );
				cd->addEff( 1, name, stacks[*j + 1] );
				cd->addEff( 1, name, stacks[*j + 1] + "-" + d->actions[i]->name, incvec( 0, size ) );
			}
			else {
				// Action is not an envelope: compile into compressed action
				std::string name = "DO-" + d->actions[i]->name;
				Action * doit = cd->createAction( name, d->typeList( d->actions[i] ) );

				//std::cout << "Creating action " << name << "\n";

				// ONLY DEALS WITH POSITIVE PRECONDITIONS HERE !!!
				cd->setPre( name, d->actions[i]->pre );
				for ( unsigned k = 0; k < get( i )->pre_o->conds.size(); ++k ) {
					Ground * pre = dynamic_cast< Ground * >( get( i )->pre_o->conds[k] );
					if ( pre && !includes( 0, pre, ( And * )get( i )->pre ) &&
					            !includes( 0, pre, ( And * )get( i )->eff ) )
						cd->addPre( 0, name, pre->name, pre->params );
					// if precon is not positive, just copy it
					if ( !pre ) ( ( And * )doit->pre )->add( get( i )->pre_o->conds[k]->copy( *cd ) );
				}
				for ( unsigned k = 0; k < get( i )->pre_e->conds.size(); ++k ) {
					Ground * h = dynamic_cast< Ground * >( get( i )->pre_e->conds[k] );
					if ( h && !includes( 0, h, ( And * )get( i )->pre ) &&
					     !includes( 0, h, ( And * )get( i )->eff ) )
						cd->addPre( 0, name, h->name, h->params );
					// if precon is not positive, just copy it
					if ( !h ) ( ( And * )doit->pre )->add( get( i )->pre_e->conds[k]->copy( *cd ) );
				}
				cd->addPre( 0, name, stacks[*j] );

				cd->setEff( name, get( i )->eff_e );
				GroundVec add = get( i )->addEffects();
				for ( unsigned k = 0; k < add.size(); ++k )
					if ( !includes( 1, add[k], get( i )->eff_e ) )
						cd->addEff( 0, name, add[k]->name, add[k]->params );
				GroundVec del = get( i )->deleteEffects();
				for ( unsigned k = 0; k < del.size(); ++k )
					if ( !includes( 0, del[k], get( i )->eff_e ) )
						cd->addEff( 1, name, del[k]->name, del[k]->params );
				if ( *j ) {
					cd->addEff( 1, name, stacks[*j] );
					cd->addEff( 0, name, stacks[*j] + "-CONTINUE", IntVec( 1, cd->constantIndex( times[g.durationMap[d->actions[i]->duration()]], "TIME" ) ) );
				}
			}

	for ( int i = 1; i <= maxCount; ++i ) {
		if ( i < maxCount ) {
			// Increase action
			std::stringstream ss;
			ss << "INCREASE" << i;
			std::string name = ss.str();
			cd->createAction( name, StringVec( 3, "TIME" ) );

			cd->addPre( 0, name, stacks[i] + "-REMAINING", incvec( 0, 1 ) );
			cd->addPre( 0, name, stacks[i] + "-INCREASE", incvec( 2, 3 ) );
			cd->addPre( 0, name, "SUBTRACT", incvec( 0, 3 ) );

			cd->addEff( 0, name, stacks[i + 1] );
			cd->addEff( 1, name, stacks[i] + "-REMAINING", incvec( 0, 1 ) );
			cd->addEff( 0, name, stacks[i] + "-REMAINING", incvec( 1, 2 ) );
			cd->addEff( 1, name, stacks[i] + "-INCREASE", incvec( 2, 3 ) );
		}
		
		// Decrease action
		std::stringstream ss;
		ss << "DECREASE" << i;
		std::string name = ss.str();
		cd->createAction( name, StringVec( 1, "TIME" ) );

		cd->addPre( 0, name, stacks[i] + "-REMAINING", incvec( 0, 1 ) );
		cd->addPre( 0, name, stacks[i] + "-DECREASE" );

		cd->addEff( 0, name, stacks[i - 1] );
		cd->addEff( 1, name, stacks[i] + "-REMAINING", incvec( 0, 1 ) );
		cd->addEff( 1, name, stacks[i] + "-DECREASE" );

		// Continue action
		std::stringstream ss2;
		ss2 << "CONTINUE" << i;
		name = ss2.str();
		cd->createAction( name, StringVec( 3, "TIME" ) );

		cd->addPre( 0, name, stacks[i] + "-REMAINING", incvec( 0, 1 ) );
		cd->addPre( 0, name, stacks[i] + "-CONTINUE", incvec( 2, 3 ) );
		cd->addPre( 0, name, "SUBTRACT", incvec( 0, 3 ) );
		
		cd->addEff( 0, name, stacks[i] );
		cd->addEff( 1, name, stacks[i] + "-REMAINING", incvec( 0, 1 ) );
		cd->addEff( 0, name, stacks[i] + "-REMAINING", incvec( 1, 2 ) );
		cd->addEff( 1, name, stacks[i] + "-CONTINUE", incvec( 2, 3 ) );
	}

	std::cout << *cd;

	cins = new Instance( *cd );
	cins->name = ins->name;

	// create initial state
	for ( unsigned i = 0; i < ins->init.size(); ++i )
		if ( d->preds.index( ins->init[i]->name ) >= 0 )
			cins->addInit( ins->init[i]->name, d->objectList( ins->init[i] ) );
	cins->addInit( stacks[0] );

	for ( unsigned i = 1; i < counts.size(); ++i ) {
		StringVec pars( 1, counts[i - 1] );
		pars.push_back( counts[i] );
		cins->addInit( "CONSECUTIVE", pars );
	}
	for ( DoublePairSet::iterator i = g.subtractionPairs.begin(); i != g.subtractionPairs.end(); ++i ) {
		StringVec pars( 1, times[g.durationMap[i->first]] );
		pars.push_back( times[g.durationMap[i->first - i->second]] );
		pars.push_back( times[g.durationMap[i->second]] );
		cins->addInit( "SUBTRACT", pars );
	}

	// create goal state
	for ( unsigned i = 0; i < ins->goal.size(); ++i )
		cins->addGoal( ins->goal[i]->name, d->objectList( ins->goal[i] ) );
	cins->addGoal( stacks[0] );

	std::cerr << *cins;

	delete cins;
	delete cd;
	delete ins;
	delete d;
}
