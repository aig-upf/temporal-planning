
#include "compile.h"

/*
std::vector< std::string > printParams( const Condition & c, int t, bool z ) {
	std::vector< std::string > pars;
	for ( unsigned i = 0; i < c.params.size(); ++i ) {
		std::string type = d->types[c.params[i]].name;
		std::stringstream ss;
		ss << "?" << type << i;
		pars.push_back( ss.str() );

		if ( i > 0 || z ) std::cout << " ";
		std::cout << ss.str() << " - " << type;
	}
	for ( unsigned i = 0; i < t; ++i ) {
		std::stringstream ss;
		ss << "?TIME" << c.params.size() + i;
		pars.push_back( ss.str() );

		std::cout << " " << ss.str();
	}
	if ( t ) std::cout << " - TIME";
	std::cout << ")\n";
	return pars;
}

void printPredicate( const Condition & c, const std::string & prefix, int t ) {
	std::cout << "\t\t(" << prefix << c.name;
	printParams( c, t, true );
}

// perhaps we have to take into account the negation of c !!!
void printCond( const Condition & c, const std::string & prefix,
                const std::vector< std::string > & pars, bool neg ) {
	std::cout << "\t\t\t(";
	if ( neg ) std::cout << "NOT (";
	std::cout << prefix << c.name;
	for ( unsigned i = 0; i < c.params.size(); ++i )
		std::cout << " " << pars[c.params[i]];
	if ( neg ) std::cout << ")";
	std::cout << ")\n";
}

// perhaps we have to take into account the negation of c !!!
void printCond2( const Condition & c, const std::string & prefix,
                 const std::vector< std::string > & pars, bool neg ) {
	std::cout << "\t\t\t\t\t(";
	if ( neg ) std::cout << "NOT (";
	std::cout << prefix << c.name;
	for ( unsigned i = 0; i < c.params.size(); ++i )
		std::cout << " " << pars[i];
	if ( neg ) std::cout << ")";
	std::cout << ")\n";
}

void printConditional( unsigned k, bool b ) {
	std::cout << "\t\t\t(FORALL (";
	std::vector< std::string > pars = printParams( d->preds[k], 0, false );
	std::cout << "\t\t\t\t(WHEN\n";
	std::cout << "\t\t\t\t\t(AND\n";
	std::cout << "\t";
	printCond2( d->preds[k], b ? "NEG-" : "POS-", pars, false );
	std::cout << "\t";
	printCond2( d->preds[k], b ? "POS-" : "NEG-", pars, true );
	std::cout << "\t\t\t\t\t)\n";
	std::cout << "\t\t\t\t\t(AND\n";
	std::cout << "\t";
	printCond2( d->preds[k], b ? "NEG-" : "POS-", pars, true );
	std::cout << "\t";
	printCond2( d->preds[k], "CUR-", pars, true );
	std::cout << "\t";
	printCond2( d->preds[k], "", pars, b );
	std::cout << "\t\t\t\t\t)\n";
	std::cout << "\t\t\t\t)\n";
	std::cout << "\t\t\t)\n";
}
*/

// detect concurrency conditions on two actions
// ASSUMES POSITIVE PRECONDITIONS OVER ALL


// detect concurrency conditions on two actions
void detectGeneralDependency( unsigned a, unsigned b, graph & g ) {
	TemporalAction * acta = get( a );
	TemporalAction * actb = get( b );

	bool edge = false, type = true;
	for ( unsigned i = 0; i < acta->eff_s.size(); ++i ) {
		Condition & c1 = acta->eff_s[i];
		// precondition at start of b added at start of a
		for ( unsigned j = 0; j < actb->pre_s.size(); ++j ) {
			Condition & c2 = actb->pre_s[j];
			if ( c1.name == c2.name && c1.neg == c2.neg && assertTypes( a, b, c1, c2 ) ) {
				edge = true;
				type = false;
			}
		}
		// precondition at end of b added at start of a
		for ( unsigned j = 0; j < actb->pre_e.size(); ++j ) {
			Condition & c2 = actb->pre_e[j];
			if ( c1.name == c2.name && c1.neg == c2.neg && assertTypes( a, b, c1, c2 ) ) {
				edge = true;
				type = false;
			}
		}
		for ( unsigned j = 0; j < actb->pre_o.size(); ++j ) {
			Condition & c2 = actb->pre_o[j];
			if ( c1.name == c2.name && c1.neg == c2.neg && assertTypes( a, b, c1, c2 ) ) {
				bool z = false;
				for ( unsigned k = 0; k < acta->eff_e.size(); ++k ) {
					Condition & c3 = acta->eff_e[k];
					z |= c1.name == c3.name && c1.params == c3.params && ( c1.neg ^ c3.neg );
				}
				// precondition over all of b added at start and deleted at end of a
				if ( z && d->actions[b].duration < d->actions[a].duration )
					edge = true;
				// precondition over all of b added at start of a (and not deleted at end)
				else if ( !z ) {
					edge = true;
					type = false;
				}
			}
		}
		// effect at end of b deleted at start of a
		// REMOVE THIS TEST FOR NOW!!!
		// TO DO: do not consider fluents not appearing as goals/precons!
//		for ( unsigned j = 0; j < d->actions[b].eff_e.size(); ++j ) {
//			Condition & c2 = d->actions[b].eff_e[j];
//			if ( c1.name == c2.name && ( c1.neg ^ c2.neg ) && assertTypes( a, b, c1, c2 ) ) {
//				edge = true;
//				type = false;
//			}
//		}
	}

	for ( unsigned i = 0; i < d->actions[a].eff_e.size(); ++i ) {
		Condition & c1 = acta->eff_e[i];
		// precondition at start of b deleted at end of a
		for ( unsigned j = 0; j < actb->pre_s.size(); ++j ) {
			Condition & c2 = actb->pre_s[j];
			if ( c1.name == c2.name && ( c1.neg ^ c2.neg ) && assertTypes( a, b, c1, c2 ) ) {
				edge = true;
				type = false;
			}
		}
		// precondition at end of b added or deleted at end of a
		for ( unsigned j = 0; j < actb->pre_e.size(); ++j ) {
			Condition & c2 = actb->pre_e[j];
			if ( c1.name == c2.name && assertTypes( a, b, c1, c2 ) ) {
				edge = true;
				type = false;
			}
		}
		// precondition over all of b deleted at end of a (and not added at start)
		for ( unsigned j = 0; j < actb->pre_o.size(); ++j ) {
			Condition & c2 = actb->pre_o[j];
			if ( c1.name == c2.name && ( c1.neg ^ c2.neg ) && assertTypes( a, b, c1, c2 ) ) {
				bool z = false;
				for ( unsigned k = 0; k < acta->eff_s.size(); ++k ) {
					Condition c3 = acta->eff_s[k];
					z |= c1.name == c3.name && c1.params == c3.params && ( c1.neg ^ c3.neg );
				}
				if ( !z ) {
					edge = true;
					type = false;
				}
			}
		}
	}
	if ( edge ) g.add( a, b, type );
}

int main( int argc, char *argv[] ) {
	if ( argc < 3 ) {
		std::cout << "Usage: ./compileFull <domain.pddl> <task.pddl>\n";
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
				detectGeneralDependency( i, j, g );
	g.computeDurations();
	g.computeDepths();

	// determine maximum duration and predicates in preconditions and effects
	unsigned duration = 0;
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		TemporalAction * act = get( i );
		duration = MAX( duration, act->duration );
		for ( unsigned j = 0; j < act->pre_s.size(); ++j )
			pres.insert( d->pmap[act->pre_s[j].name] );
		for ( unsigned j = 0; j < act->pre_e.size(); ++j )
			pres.insert( d->pmap[act->pre_e[j].name] );
		for ( unsigned j = 0; j < act->eff_s.size(); ++j )
			effs.insert( d->pmap[act->eff_s[j].name] );
		for ( unsigned j = 0; j < act->eff_e.size(); ++j )
			effs.insert( d->pmap[act->eff_e[j].name] );
	}
	std::vector< int >::iterator it;
	std::vector< int > prevars( pres.size() );
	it = set_intersection( pres.begin(), pres.end(), effs.begin(), effs.end(), prevars.begin() );
	pres = std::set< int >( prevars.begin(), it );
	
	// move static preconditions to pre_s !!!
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		TemporalAction * act = get( i );
		for ( unsigned j = 0; j < act->pre_o.size(); ++j )
			if ( effs.find( d->pmap[act->pre_o[j].name] ) == effs.end() &&
			     find( act->pre_s.begin(), act->pre_s.end(), act->pre_o[j] ) == act->pre_s.end() )
				act->pre_s.push_back( act->pre_o[j] );
		for ( unsigned j = 0; j < act->pre_e.size(); ++j )
			if ( effs.find( d->pmap[act->pre_e[j].name] ) == effs.end() &&
			     find( act->pre_s.begin(), act->pre_s.end(), act->pre_e[j] ) == act->pre_s.end() )
				act->pre_s.push_back( act->pre_e[j] );
	}

	// interesting idea: determine parallelism, solve rest sequentially
	// key: add fluent at start, delete fluent at end
	// check actions that have this fluent as precondition

	// first generate the domain file
	cd = new Domain;
	cd->name = d->name;
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

	std::cout << "\t(:PREDICATES\n";
	for ( unsigned i = 0; i < d->preds.size(); ++i ) {
		printPredicate( d->preds[i], "", false );
		if ( pres.find( i ) != pres.end() )
			printPredicate( d->preds[i], "LOCK-", 0 );
		if ( effs.find( i ) != effs.end() ) {
			printPredicate( d->preds[i], "POS-", 0 );
			printPredicate( d->preds[i], "NEG-", 0 );
			printPredicate( d->preds[i], "CUR-", 0 );
		}
	}
	std::cout << "\n\t\t(DONE)\n";
	std::cout << "\t\t(PARFREE)\n";
	std::cout << "\t\t(TIMEFREE)\n";
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		printPredicate( d->actions[i], "ACTIVE-", 0 );
		printPredicate( d->actions[i], "TEMP-", 0 );
		printPredicate( d->actions[i], "REMAINING-", 1 );
	}
	std::cout << "\t\t(CONSECUTIVE-TIMES ?TIME0 ?TIME1 - TIME)\n";
	std::cout << "\t)\n";

	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		// possibly other actions with precondition "NOT POS-/NEG-" instead of "NOT CUR-"
		std::cout << "\t(:ACTION START-" << d->actions[i].name << "\n";
		std::cout << "\t\t:PARAMETERS (";
		std::vector< std::string > pars = printParams( d->actions[i], 0, false );
		std::cout << "\t\t:PRECONDITION (AND\n";
		for ( unsigned j = 0; j < d->actions[i].pre_s.size(); ++j ) {
			printCond( d->actions[i].pre_s[j], "", pars, false );
			if ( pres.find( d->pmap[d->actions[i].pre_s[j].name] ) != pres.end() )
				printCond( d->actions[i].pre_s[j], "CUR-", pars, true );
		}
		std::cout << "\n";
		for ( unsigned j = 0; j < d->actions[i].eff_s.size(); ++j )
			if ( pres.find( d->pmap[d->actions[i].eff_s[j].name] ) != pres.end() )
				printCond( d->actions[i].eff_s[j], "LOCK-", pars, true );
		std::cout << "\n\t\t\t(NOT (DONE))\n";
		std::cout << "\t\t\t(PARFREE)\n";
		std::cout << "\t\t\t(NOT (ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << "))\n";
		std::cout << "\t\t)\n";
		std::cout << "\t\t:EFFECT (AND\n";
		for ( unsigned j = 0; j < d->actions[i].pre_s.size(); ++j )
			if ( pres.find( d->pmap[d->actions[i].pre_s[j].name] ) != pres.end() )
				printCond( d->actions[i].pre_s[j], "LOCK-", pars, false );
		std::cout << "\n";
		for ( unsigned j = 0; j < d->actions[i].eff_s.size(); ++j ) {
			Condition & c = d->actions[i].eff_s[j];
			printCond( c, c.neg ? "NEG-" : "POS-", pars, false );
			printCond( c, "CUR-", pars, false );
		}
		std::cout << "\n\t\t\t(NOT (TIMEFREE))\n";
		std::cout << "\t\t\t(ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << ")\n";
		std::cout << "\t\t\t(REMAINING-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << " TIME" << d->actions[i].duration << ")\n";
		std::cout << "\t\t)\n";
		std::cout << "\t)\n";

		std::cout << "\t(:ACTION END-" << d->actions[i].name << "\n";
		std::cout << "\t\t:PARAMETERS (";
		printParams( d->actions[i], 0, false );
		std::cout << "\t\t:PRECONDITION (AND\n";
		for ( unsigned j = 0; j < d->actions[i].pre_e.size(); ++j ) {
			printCond( d->actions[i].pre_e[j], "", pars, false );
			if ( pres.find( d->pmap[d->actions[i].pre_e[j].name] ) != pres.end() )
				printCond( d->actions[i].pre_e[j], "CUR-", pars, true );
		}
		std::cout << "\n";
		for ( unsigned j = 0; j < d->actions[i].eff_e.size(); ++j )
			if ( pres.find( d->pmap[d->actions[i].eff_e[j].name] ) != pres.end() )
				printCond( d->actions[i].eff_e[j], "LOCK-", pars, true );
		std::cout << "\n\t\t\t(PARFREE)\n";
		std::cout << "\t\t\t(REMAINING-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << " TIME0)\n";
		std::cout << "\t\t)\n";
		std::cout << "\t\t:EFFECT (AND\n";
		for ( unsigned j = 0; j < d->actions[i].pre_e.size(); ++j )
			if ( pres.find( d->pmap[d->actions[i].eff_e[j].name] ) != pres.end() )
				printCond( d->actions[i].pre_e[j], "LOCK-", pars, false );
		std::cout << "\n";
		for ( unsigned j = 0; j < d->actions[i].eff_e.size(); ++j ) {
			Condition & c = d->actions[i].eff_e[j];
			printCond( c, c.neg ? "NEG-" : "POS-", pars, false );
			printCond( c, "CUR-", pars, false );
		}
		std::cout << "\n\t\t\t(NOT (TIMEFREE))\n";
		std::cout << "\t\t\t(NOT (ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << "))\n";
		std::cout << "\t\t\t(NOT (REMAINING-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << " TIME0))\n";
		std::cout << "\t\t)\n";
		std::cout << "\t)\n";

		std::cout << "\t(:ACTION ADVANCE-" << d->actions[i].name << "\n";
		std::cout << "\t\t:PARAMETERS (";
		std::vector< std::string > pars2 = printParams( d->actions[i], 2, false );
		std::cout << "\t\t:PRECONDITION (AND\n";
		for ( unsigned j = 0; j < d->actions[i].pre_o.size(); ++j )
			printCond( d->actions[i].pre_o[j], "", pars, false );
		std::cout << "\n\t\t\t(TIMEFREE)\n";
		std::cout << "\t\t\t(ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << ")\n";
		std::cout << "\t\t\t(REMAINING-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << " " << pars2[pars2.size() - 1] << ")\n";
		std::cout << "\t\t\t(CONSECUTIVE-TIMES " << pars2[pars2.size() - 2] << " " << pars2[pars2.size() - 1] << ")\n";
		std::cout << "\t\t)\n";
		std::cout << "\t\t:EFFECT (AND\n";
		std::cout << "\t\t\t(NOT (PARFREE))\n";
		std::cout << "\t\t\t(NOT (ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << "))\n";
		std::cout << "\t\t\t(TEMP-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << ")\n";
		std::cout << "\t\t\t(REMAINING-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << " " << pars2[pars2.size() - 2] << ")\n";
		std::cout << "\t\t\t(NOT (REMAINING-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << " " << pars2[pars2.size() - 1] << "))\n";
		std::cout << "\t\t)\n";
		std::cout << "\t)\n";
	}

	std::cout << "\t(:ACTION COPY-EFFECTS\n";
	std::cout << "\t\t:PARAMETERS ()\n";
	std::cout << "\t\t:EFFECT (AND\n";
	std::cout << "\t\t\t(NOT (PARFREE))\n";
	for ( unsigned i = 0; i < d->preds.size(); ++i ) {
		if ( effs.find( i ) != effs.end() ) {
			printConditional( i, false );
			printConditional( i, true );
		}
		if ( pres.find( i ) != pres.end() ) {
			std::cout << "\t\t\t(FORALL (";
			std::vector< std::string > pars = printParams( d->preds[i], 0, false );
			std::cout << "\t\t\t\t(WHEN\n";
			printCond2( d->preds[i], "LOCK-", pars, false );
			printCond2( d->preds[i], "LOCK-", pars, true );
			std::cout << "\t\t\t\t)\n";
			std::cout << "\t\t\t)\n";
		}
	}
	std::cout << "\t\t)\n";
	std::cout << "\t)\n";

	std::cout << "\t(:ACTION FINISH-COPY\n";
	std::cout << "\t\t:PARAMETERS ()\n";
	std::cout << "\t\t:PRECONDITION (AND\n";
	for ( unsigned i = 0; i < d->preds.size(); ++i )
		if ( effs.find( i ) != effs.end() ) {
			std::cout << "\t\t\t(FORALL (";
			std::vector< std::string > pars = printParams( d->preds[i], 0, false );
			std::cout << "\t\t\t\t(NOT (CUR-" << d->preds[i].name;
			for ( unsigned j = 0; j < pars.size(); ++j )
				std::cout << " " << pars[j];
			std::cout << "))\n";
			std::cout << "\t\t\t)\n";
		}
	std::cout << "\t\t)\n";
	std::cout << "\t\t:EFFECT\n";
	std::cout << "\t\t\t(TIMEFREE)\n";
	std::cout << "\t)\n";

	std::cout << "\t(:ACTION FINISH-ADVANCE\n";
	std::cout << "\t\t:PARAMETERS ()\n";
	std::cout << "\t\t:PRECONDITION (AND\n";
	std::cout << "\t\t\t(TIMEFREE)\n";
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		std::cout << "\t\t\t(FORALL (";
		std::vector< std::string > pars = printParams( d->actions[i], 0, false );
		std::cout << "\t\t\t\t(NOT (ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << "))\n";
		std::cout << "\t\t\t)\n";
	}
	std::cout << "\t\t)\n";
	std::cout << "\t\t:EFFECT (AND\n";
	std::cout << "\t\t\t(PARFREE)\n";
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		std::cout << "\t\t\t(FORALL (";
		std::vector< std::string > pars = printParams( d->actions[i], 0, false );
		std::cout << "\t\t\t\t(WHEN\n";
		printCond2( d->actions[i], "TEMP-", pars, false );
		std::cout << "\t\t\t\t\t(AND\n";
		std::cout << "\t";
		printCond2( d->actions[i], "ACTIVE-", pars, false );
		std::cout << "\t";
		printCond2( d->actions[i], "TEMP-", pars, true );
		std::cout << "\t\t\t\t\t)\n";
		std::cout << "\t\t\t\t)\n";
		std::cout << "\t\t\t)\n";
	}
	std::cout << "\t\t)\n";
	std::cout << "\t)\n";

	std::cout << "\t(:ACTION FINISH\n";
	std::cout << "\t\t:PARAMETERS ()\n";
	std::cout << "\t\t:PRECONDITION (AND\n";
	std::cout << "\t\t\t(PARFREE)\n";
	std::cout << "\t\t\t(TIMEFREE)\n";
	for ( unsigned i = 0; i < d->actions.size(); ++i ) {
		std::cout << "\t\t\t(FORALL (";
		std::vector< std::string > pars = printParams( d->actions[i], 0, false );
		std::cout << "\t\t\t\t(NOT (ACTIVE-" << d->actions[i].name;
		for ( unsigned j = 0; j < pars.size(); ++j )
			std::cout << " " << pars[j];
		std::cout << "))\n";
		std::cout << "\t\t\t)\n";
	}
	std::cout << "\t\t)\n";
	std::cout << "\t\t:EFFECT\n";
	std::cout << "\t\t\t(DONE)\n";
	std::cout << "\t)\n";

	std::cout << ")\n";

	// now output the instance file to stderr
	std::cerr << "(DEFINE (PROBLEM " << ins->name << ")\n";
	std::cerr << "\t(:DOMAIN " << d->name << ")\n";
	std::cerr << "\t(:OBJECTS)\n";
	std::cerr << "\t(:INIT\n";
	for ( unsigned i = 0; i < ins->init.size(); ++i ) {
		std::cerr << "\t\t(" << ins->init[i].name;
		Condition & c = d->preds[d->pmap[ins->init[i].name]];
		for ( unsigned j = 0; j < c.params.size(); ++j )
			std::cerr << " " << ins->objects[c.params[j]][ins->init[i].params[j]];
		std::cerr << ")\n";
	}
	std::cerr << "\n\t\t(PARFREE)\n";
	std::cerr << "\t\t(TIMEFREE)\n";
	for ( unsigned i = 0; i < duration; ++i )
		std::cerr << "\t\t(CONSECUTIVE-TIMES TIME" << i << " TIME" << i+1 << ")\n";
	std::cerr << "\t)\n";
	std::cerr << "\t(:GOAL (AND\n";
	for ( unsigned i = 0; i < ins->goal.size(); ++i ) {
		std::cerr << "\t\t(" << ins->goal[i].name;
		Condition & c = d->preds[d->pmap[ins->goal[i].name]];
		for ( unsigned j = 0; j < c.params.size(); ++j )
			std::cerr << " " << ins->objects[c.params[j]][ins->goal[i].params[j]];
		std::cerr << ")\n";
	}
	std::cerr << "\n\t\t(DONE)\n";
	std::cerr << "\t))\n";
	std::cerr << ")\n";

	delete ins;
	delete d;
}
