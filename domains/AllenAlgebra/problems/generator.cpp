
// Usage:
// 1) compile problems/generator with "scons"
// 2) problems/generator domain/domain.pddl problems/probXX.pddl > domain.pddl 2> ins.pddl
// 3) v2.0/bin/compileTempoParallelAlt domain.pddl ins.pddl 3 > cdomain.pddl 2> cins.pddl
// 4) v2.0/fd_impl/plan-ipc seq-sat-lama-2011 cdomain.pddl cins.pddl result.txt

#include <parser/Instance.h>
//#include "/home/hectorpal/work/softw-others/anders-jonsson-parser/universal-pddl-parser/Instance.h"

using namespace parser::pddl;

Domain * d;
Domain * cd;
Instance * ins;
TemporalAction * action;
TokenStruct< std::string > conds;
TokenStruct< std::string > ivals;
std::vector< TemporalAction * > a;

// Create the action associated with the interval at index ix
void createIntervalAction( unsigned ix ) {
	int x = cd->constantIndex( ivals[ix], "INTERVAL" );

	a[ix] = new TemporalAction( action->name + "-" + ivals[ix] );
	a[ix]->durationExpr = new ValueExpression( action->durationExpr->evaluate( *ins, StringVec( 1, ivals[ix] ) ) );
	a[ix]->pre = new And;
	( ( And * )a[ix]->pre )->conds.push_back( cd->ground( "NOT-STARTED", IntVec( 1, x ) ) );
	a[ix]->eff = new And;
	( ( And * )a[ix]->eff )->conds.push_back( new Not( cd->ground( "NOT-STARTED", IntVec( 1, x ) ) ) );
	( ( And * )a[ix]->eff )->conds.push_back( cd->ground( "STARTED", IntVec( 1, x ) ) );
	a[ix]->pre_o = new And;
	a[ix]->pre_e = new And;
	a[ix]->eff_e = new And;
	a[ix]->eff_e->conds.push_back( new Not( cd->ground( "NOT-ENDED", IntVec( 1, x ) ) ) );
	a[ix]->eff_e->conds.push_back( cd->ground( "ENDED", IntVec( 1, x ) ) );
}

void before( int k, int x, int y, const IntVec & v ) {
	// Interval x has to end before starting y
	( ( And * )a[y]->pre )->conds.push_back( cd->ground( "ENDED", IntVec( 1, v[0] ) ) );
}

void overlaps( int k, int x, int y, const IntVec & v ) {
	// Interval x has to be executing in order to start y
	( ( And * )a[y]->pre )->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[0] ) ) );
	( ( And * )a[y]->pre )->conds.push_back( cd->ground( "NOT-ENDED", IntVec( 1, v[0] ) ) );

	// Interval x has to end before ending y
	a[y]->pre_e->conds.push_back( cd->ground( "ENDED", IntVec( 1, v[0] ) ) );
}

void starts( int k, int x, int y, const IntVec & v ) {
	// Interval y has started during the execution of x
	a[x]->pre_o->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[1] ) ) );

	// Interval x has started during the execution of y
	a[y]->pre_o->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[0] ) ) );

	// Interval x has to end before ending y
	a[y]->pre_e->conds.push_back( cd->ground( "ENDED", IntVec( 1, v[0] ) ) );
}

void during( int k, int x, int y, const IntVec & v ) {
	// Interval y has to start before starting x
	( ( And * )a[x]->pre )->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[1] ) ) );

	// Interval y cannot end before ending x
	a[x]->pre_e->conds.push_back( cd->ground( "NOT-ENDED", IntVec( 1, v[1] ) ) );
}

void finishes( int k, int x, int y, const IntVec & v ) {
	// Interval y has to start before starting x
	( ( And * )a[x]->pre )->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[1] ) ) );

	// Interval y has not ended during the execution of x
	a[x]->pre_o->conds.push_back( cd->ground( "NOT-ENDED", IntVec( 1, v[1] ) ) );

	// Interval x has not ended during the execution of y
	a[y]->pre_o->conds.push_back( cd->ground( "NOT-ENDED", IntVec( 1, v[0] ) ) );
}

void equal( int k, int x, int y, const IntVec & v ) {
	// Interval y has started and not ended during the execution of x
	a[x]->pre_o->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[1] ) ) );
	a[x]->pre_o->conds.push_back( cd->ground( "NOT-ENDED", IntVec( 1, v[1] ) ) );

	// Interval x has started and not ended during the execution of y
	a[y]->pre_o->conds.push_back( cd->ground( "STARTED", IntVec( 1, v[0] ) ) );
	a[y]->pre_o->conds.push_back( cd->ground( "NOT-ENDED", IntVec( 1, v[0] ) ) );
}

void meets( int k, int x, int y, const IntVec & v ) {
	// Interval x has ended during the execution of y
	a[y]->pre_o->conds.push_back( cd->ground( "ENDED", IntVec( 1, v[0] ) ) );

	// Create auxiliary interval whose duration is d(x)+d(y)
	std::stringstream ss;
	ss << "AUX" << ivals.size() + 1;
	ivals.insert( ss.str() );
	double duration = a[x]->durationExpr->evaluate() + a[y]->durationExpr->evaluate();
	a.push_back( 0 );
	ins->addObject( ss.str(), "INTERVAL" );
	cd->createConstant( ss.str(), "INTERVAL" );
	ins->addInit( "NOT-STARTED", StringVec( 1, ss.str() ) );
	ins->addInit( "NOT-ENDED", StringVec( 1, ss.str() ) );
	ins->addInit( "LENGTH", duration, StringVec( 1, ss.str() ) );
	createIntervalAction( ivals.size() - 1 );

	// Interval x starts auxiliary interval
	IntVec startPars( 1, v[0] );
	startPars.push_back( cd->constantIndex( ss.str(), "INTERVAL" ) );
	starts( 3, x, ivals.size() - 1, startPars );

	// Interval y finishes auxiliary interval
	IntVec finishPars( 1, v[1] );
	finishPars.push_back( cd->constantIndex( ss.str(), "INTERVAL" ) );
	finishes( 5, y, ivals.size() - 1, finishPars );
}

void createDomain() {
	ivals = d->types.get( "INTERVAL" )->objects;
	std::string conditions[7] = { "BEFORE", "MEETS", "OVERLAPS", "STARTS", "DURING", "FINISHES", "EQUAL" };
	for ( unsigned i = 0; i < 7; ++i )
		conds.insert( conditions[i] );

	cd = new Domain;
	cd->name = d->name + "-" + ins->name;
	cd->typed = cd->cons = cd->temp = true;

	// Add types
	cd->createType( "INTERVAL" );

	// Add constants
	for ( unsigned i = 0; i < ivals.size(); ++i )
		cd->createConstant( ivals[i], "INTERVAL" );

	// Add predicates, ignoring goal predicates
	for ( unsigned i = 0; i < 4; ++i )
		cd->createPredicate( d->preds[i]->name, d->typeList( d->preds[i] ) );

	// Add temporal actions
	a.resize( ivals.size(), 0 );
	action = ( TemporalAction * )d->actions[0];
	for ( unsigned i = 0; i < a.size(); ++i )
		createIntervalAction( i );

	// Go over goals
	for ( unsigned i = 0; i < ins->goal.size(); ++i ) {
		int k = conds.index( ins->goal[i]->name );
		int x = ins->goal[i]->params[0];
		int y = ins->goal[i]->params[1];
		IntVec v( 1, cd->constantIndex( ivals[x], "INTERVAL" ) );
		v.push_back( cd->constantIndex( ivals[y], "INTERVAL" ) );
		switch( k ) {
			case 0: before  ( k, x, y, v ); break;
			case 1: meets   ( k, x, y, v ); break;
			case 2: overlaps( k, x, y, v ); break;
			case 3: starts  ( k, x, y, v ); break;
			case 4: during  ( k, x, y, v ); break;
			case 5: finishes( k, x, y, v ); break;
			case 6: equal   ( k, x, y, v ); break;
		}
	}

	for ( unsigned i = 0; i < a.size(); ++i )
		cd->actions.insert( a[i] );
	
	std::cout << *cd;
}

void createInstance() {
	Instance * cins = new Instance( *cd );
	cins->name = ins->name;
	cins->metric = true;

	// create initial state
	for ( unsigned i = 0; i < ins->init.size(); ++i )
		if ( d->preds.index( ins->init[i]->name ) >= 0 )
			cins->addInit( ins->init[i]->name, d->objectList( ins->init[i] ) );

	// create goal state
	for ( unsigned i = 0; i < ivals.size(); ++i )
		cins->addGoal( "ENDED", StringVec( 1, ivals[i] ) );
	//for ( unsigned i = 0; i < ins->goal.size(); ++i )
	//	cins->addGoal( ins->goal[i]->name, d->objectList( ins->goal[i] ) );

	std::cerr << *cins;

	delete cins;
}

int main( int argc, char *argv[] ) {
	if ( argc < 3 ) {
		std::cout << "Usage: ./generator <domain.pddl> <task.pddl>\n";
		std::cout << "Writes domain file to standard output and instance file to standard error\n";
		exit( 1 );
	}

	d = new Domain( argv[1] );
	ins = new Instance( *d, argv[2] );

	createDomain();
	createInstance();

	delete cd;
	delete ins;
	delete d;

}
