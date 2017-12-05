
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

#endif
