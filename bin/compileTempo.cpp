// To check for memory leaks:
// valgrind --leak-check=yes bin/compileTempo ../domains/tempo-sat/Mapanalyser/domain/domain.pddl ../domains/tempo-sat/Mapanalyser/problems/pfile3-4-2-0-1.pddl 2

#include "compile.h"

int main( int argc, char *argv[] ) {
	if ( argc < 4 ) {
		std::cout << "Usage: ./compileTempo <domain.pddl> <task.pddl> <bound>\n";
		std::cout << "Writes domain file to standard output and instance file to standard error\n";
		exit( 1 );
	}

	d = new Domain( argv[1] );
	ins = new Instance( *d, argv[2] );

	unsigned maxCount;
	std::istringstream is( argv[3] );
	is >> maxCount;

	// RIGHT NOW ASSUMES POSITIVE PRECONDITIONS OVER ALL !!!

	// include all contexts (only if maxCount > 1)
	for ( unsigned i = 0; 1 < maxCount && i < d->actions.size(); ++i )
		for ( unsigned j = 0; j < get( i )->pre_o->conds.size(); ++j ) {
			Ground * pre = dynamic_cast< Ground * >( get( i )->pre_o->conds[j] );
			if ( pre ) {
				int k = d->preds.index( pre->name );
				if ( k >= 0 && isDel( k ) ) pres.insert( k );
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

	// Add predicates
	for ( unsigned i = 0; i < d->preds.size(); ++i )
		cd->createPredicate( d->preds[i]->name, d->typeList( d->preds[i] ) );

	// represent the "stack size", i.e. number of active actions
	StringVec stacks( 1, "EMPTYSTACK" );
	cd->createPredicate( stacks[0] );
	for ( unsigned i = 1; i <= maxCount; ++i ) {
		std::stringstream ss;
		ss << "STACK" << i;
		stacks.push_back( ss.str() );
		cd->createPredicate( stacks[i] );
	}

	// represent that actions are active
	for ( unsigned i = 0; i < d->actions.size(); ++i )
		cd->createPredicate( "ACTIVE-" + d->actions[i]->name, d->typeList( d->actions[i] ) );

	// associate counts with each context
	StringDVec counts( d->preds.size() );
	for ( IntSet::iterator i = pres.begin(); i != pres.end(); ++i )
		for ( unsigned j = 0; j <= maxCount; ++j ) {
			std::stringstream ss;
			ss << "COUNT" << j << "-" << d->preds[*i]->name;
			counts[*i].push_back( ss.str() );
			cd->createPredicate( counts[*i][j], d->typeList( d->preds[*i] ) );
		}

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
		// Start action
		std::string name = "START-" + d->actions[i]->name;
		unsigned size = d->actions[i]->params.size();
		Action * start = cd->createAction( name, d->typeList( d->actions[i] ) );

		// copy the preconditions at start
		cd->setPre( name, d->actions[i]->pre );

		// add all preconditions over all not added at start
		// ONLY DEALS WITH POSITIVE PRECONDITIONS OVER ALL !!!
		for ( unsigned k = 0; k < get( i )->pre_o->conds.size(); ++k ) {
			Ground * h = dynamic_cast< Ground * >( get( i )->pre_o->conds[k] );
			if ( h &&
			     !includes( 0, h, ( And * )get( i )->pre ) &&
			     !includes( 0, h, ( And * )get( i )->eff ) )
				cd->addPre( 0, name, h->name, h->params );
		}
		// action is not already active
		cd->addPre( 1, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

		// the limit on active actions has not been reached
		if ( maxCount == 1 ) cd->addPre( 0, name, stacks[0] );
		else cd->addPre( 1, name, stacks[maxCount] );

		// all counts are 0 for fluents that become deleted
		GroundVec del = d->actions[i]->deleteEffects();
		for ( unsigned j = 0; j < del.size(); ++j ) {
			int x = d->preds.index( del[j]->name );
			if ( pres.find( x ) != pres.end() )
				cd->addPre( 0, name, counts[x][0], del[j]->params );
		}

		// copy the effects at start
		cd->setEff( name, d->actions[i]->eff );

		// action becomes active
		cd->addEff( 0, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

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
					( ( And * )start->eff )->add( w );
				}
			}
		}

		// only add conditional effects if maxCount > 1
		if ( maxCount == 1 ) {
			cd->addEff( 1, name, stacks[0] );
			cd->addEff( 0, name, stacks[1] );
		}
		else for ( unsigned j = 0; j < maxCount; ++j ) {
			When * w = new When;
			w->pars = new Ground( cd->preds.get( stacks[j] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( stacks[j] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( stacks[j + 1] ) ) );
			( ( And * )start->eff )->add( w );
		}

		if ( durationIndices[i].size() ) {
			IntVec pars( durationIndices[i].begin(), durationIndices[i].end() );
			cd->addCost( name, d->actions[i]->name + "-COST", pars );
		}
		else cd->addCost( name, 10000 * get( i )->durationExpr->evaluate( *ins, StringVec() ) );

		// End action
		name = "END-" + d->actions[i]->name;
		Action * end = cd->createAction( name, d->typeList( d->actions[i] ) );

		// copy preconditions at end
		cd->setPre( name, get( i )->pre_e );

		// add preconditions over all
		// ONLY DEALS WITH POSITIVE PRECONDITIONS OVER ALL !!!
		for ( unsigned k = 0; k < get( i )->pre_o->conds.size(); ++k ) {
			Ground * h = dynamic_cast< Ground * >( get( i )->pre_o->conds[k] );
			if ( h && !includes( 0, h, get( i )->pre_e ) )
				cd->addPre( 0, name, h->name, h->params );
		}

		// action is active
		cd->addPre( 0, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

		// all counts are 0 for fluents that become deleted
		for ( unsigned j = 0; j < get( i )->eff_e->conds.size(); ++j ) {
			Not * n = dynamic_cast< Not * >( get( i )->eff_e->conds[j] );
			if ( n && pres.find( d->preds.index( n->cond->name ) ) != pres.end() ) {
				bool b = includes( 0, n->cond, get( i )->pre_o );
				cd->addPre( 0, name, counts[d->preds.index( n->cond->name )][b], n->cond->params );
			}
		}

		// copy effects at end
		cd->setEff( name, get( i )->eff_e );

		// action becomes free
		cd->addEff( 1, name, "ACTIVE-" + d->actions[i]->name, incvec( 0, size ) );

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
					( ( And * )end->eff )->add( w );
				}
			}
		}

		// only add conditional effects if maxCount > 1
		if ( maxCount == 1 ) {
			cd->addEff( 0, name, stacks[0] );
			cd->addEff( 1, name, stacks[1] );
		}
		else for ( unsigned j = 0; j < maxCount; ++j ) {
			When * w = new When;
			w->pars = new Ground( cd->preds.get( stacks[j + 1] ) );
			w->cond = new And;
			( ( And * )w->cond )->add( new Not( new Ground( cd->preds.get( stacks[j + 1] ) ) ) );
			( ( And * )w->cond )->add( new Ground( cd->preds.get( stacks[j] ) ) );
			( ( And * )end->eff )->add( w );
		}

		cd->addCost( name, 1 );
	}

	std::cout << *cd;

	cins = new Instance( *cd );
	cins->name = ins->name;

	cins->metric = true;

	// create initial state
	for ( unsigned i = 0; i < ins->init.size(); ++i )
		if ( d->preds.index( ins->init[i]->name ) >= 0 )
			cins->addInit( ins->init[i]->name, d->objectList( ins->init[i] ) );

	// stack is empty
	cins->addInit( stacks[0] );

	// counts are 0
	for ( IntSet::iterator i = pres.begin(); i != pres.end(); ++i ) {
		StringVec pars( d->preds[*i]->params.size() );
		recurse( d->preds[*i], 0, pars, "COUNT0-" );
	}

	// add function values
	for ( unsigned i = 0; i < d->actions.size(); ++i )
		if ( durationIndices[i].size() ) {
			StringVec pars( d->actions[i]->params.size() );
			IntVec indices( durationIndices[i].begin(), durationIndices[i].end() );
			recCost( 0, pars, indices, i );
		}

	cins->addInit( "TOTAL-COST", 0.0 );

	// create goal state
	for ( unsigned i = 0; i < ins->goal.size(); ++i )
		cins->addGoal( ins->goal[i]->name, d->objectList( ins->goal[i] ) );

	// stack is empty
	cins->addGoal( stacks[0] );

	std::cerr << *cins;

	delete cins;
	delete cd;
	delete ins;
	delete d;
}
