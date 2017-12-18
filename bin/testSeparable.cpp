
// Old version at c67533c
// ./testSeparable <temporal_domain> <temporal_ins> output.sas

#include "compile.h"

ActionVec actions;
CondVec init, goal;
std::vector< CondVec > mutexes;

bool seps = true, sepe = true;

std::map< unsigned, std::set< unsigned > > prods;       // producibles
std::map< unsigned, std::set< unsigned > > envs, conts; // envelopes and contents

bool mutex( unsigned k, Condition * c, Condition * d ) {
	bool a = false, b = false;
	for ( unsigned i = 0; i < mutexes[k].size(); ++i ) {
		a |= mutexes[k][i] == c;
		b |= mutexes[k][i] == d;
	}
	return a && b;
}

void parseTranslation( const std::string & s, std::vector< CondVec > & v ) {
	Filereader f( s );
	unsigned m, n, x, y, z;
	while ( f.s != "end_metric" ) std::getline( f.f, f.s );
	std::getline( f.f, f.s );
	std::istringstream is( f.getToken() );
	f.next();
	is >> m;
	for ( unsigned i = 0; i < m; ++i, f.c = 0 ) {
		f.assert( "begin_variable" );
		f.getToken();
		f.next();
		f.getToken();
		f.next();

		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> n;

		CondVec u;
		for ( unsigned j = 0; j < n; ++j ) {
			std::string t = f.getToken();
			f.next();
			bool b = t == "NegatedAtom";
			if ( t[0] != '<' ) {
				Condition * c = new Condition( f.getToken() );
				c->neg = b;
				f.assert( "(" );
				for ( unsigned k = 0; k < d->preds[d->pmap[c->name]]->params.size(); ++k ) {
					if ( k > 0 ) f.assert( "," );
					c->params.push_back( ins->omap[d->preds[d->pmap[c->name]]->params[k]][f.getToken()] );
				}
				f.assert( ")" );
				u.push_back( c );
			}
			else {
				f.getToken();
				f.next();
				f.getToken();
				f.next();
			}
		}
		f.assert( "end_variable" );
		v.push_back( u );
	}
	is.clear();
	is.str( f.getToken() );
	f.next();
	is >> m;
	for ( unsigned i = 0; i < m; ++i ) {
		f.assert( "begin_mutex_group" );

		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> n;

		CondVec u;
		for ( unsigned j = 0; j < n; ++j ) {
			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> x;

			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> y;

			u.push_back( v[x][y] );
		}
		f.assert( "end_mutex_group" );
		mutexes.push_back( u );
	}
//	for ( unsigned i = 0; i < mutexes.size(); ++i ) {
//		for ( unsigned j = 0; j < mutexes[i].size(); ++j )
//			std::cout << mutexes[i][j]->name << mutexes[i][j]->params << " ";
//		std::cout << "\n";
//	}

	f.assert( "begin_state" );
	for ( unsigned i = 0; i < v.size(); ++i ) {
		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> x;
		init.push_back( v[i][x] );
	}
	f.assert( "end_state" );
	f.assert( "begin_goal" );
	is.clear();
	is.str( f.getToken() );
	f.next();
	is >> m;
	for ( unsigned i = 0; i < m; ++i ) {
		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> x;

		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> y;

		goal.push_back( v[x][y] );
	}
	f.assert( "end_goal" );

	is.clear();
	is.str( f.getToken() );
	f.next();
	is >> m;

	for ( unsigned i = 0; i < m; ++i ) {
		f.assert( "begin_operator" );
	
		Action * a = new Action( f.getToken().substr( 3 ) );
		Action * act = d->actions[d->amap[a->name]];
		f.next();
		for ( unsigned j = 0; j < act->params.size(); ++j ) {
			a->params.push_back( ins->omap[act->params[j]][f.getToken()] );
			f.next();
		}

		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> n;

		for ( unsigned j = 0; j < n; ++j ) {
			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> x;

			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> y;
		}

		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> n;

		std::vector< std::pair< unsigned, unsigned > > pres, effs;
		for ( unsigned j = 0; j < n; ++j ) {
			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> x;

			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> x;

			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> y;

			is.clear();
			is.str( f.getToken() );
			f.next();
			is >> z;

			pres.push_back( std::make_pair( x, y ) );
			if ( z < v[x].size() ) effs.push_back( std::make_pair( x, z ) );
		}

		Condition * c = new Condition( "dummy" );
		for ( unsigned j = 0; j < pres.size(); ++j )
			for ( unsigned k = 0; k < effs.size(); ++k )
				for ( unsigned l = 0; l < mutexes.size(); ++l )
					if ( mutex( l, v[pres[j].first][pres[j].second], v[effs[k].first][effs[k].second] ) )
						c->params.push_back( l );
		a->pre_s.push_back( c );

		is.clear();
		is.str( f.getToken() );
		f.next();
		is >> n;

		std::cout << a->name << a->params << ": " << a->pre_s[0]->params << "\n";

		actions.push_back( a );

		f.assert( "end_operator" );
	}
}

bool isProducer( Action & a, Condition & c, bool test ) {
	std::set< unsigned > seff, eeff;
	for ( unsigned i = 0; i < a.eff_s.size(); ++i )
		if ( a.eff_s[i]->name == c.name ) seff.insert( i );
	for ( unsigned i = 0; i < a.eff_e.size(); ++i )
		if ( a.eff_e[i]->name == c.name ) eeff.insert( i );

	if ( seff.size() != eeff.size() ) return false;

	for ( std::set< unsigned >::iterator i = seff.begin(); i != seff.end(); ++i ) {
		if ( a.eff_s[*i]->neg ) return false;
		bool b = false;
		for ( std::set< unsigned >::iterator j = eeff.begin(); j != eeff.end(); ++j )
			b |= a.eff_e[*j]->neg && a.eff_s[*i]->params == a.eff_e[*j]->params;
		if ( !b ) return false;
	}
	return test || seff.size();
}

void identifyProducibles() {
	std::set< unsigned > pcand;
	for ( unsigned i = 0; i < d->preds.size(); ++i ) {
		bool b = true;
		for ( unsigned j = 0; j < ins->init.size(); ++j )
			b &= ins->init[j]->name != d->preds[i]->name;
		if ( b ) pcand.insert( i );
	}

//	std::cout << pcand << "\n";

	for ( std::set< unsigned >::iterator i = pcand.begin(); i != pcand.end(); ++i ) {
		bool b = true;
		for ( unsigned j = 0; j < d->actions.size(); ++j )
			b &= isProducer( *d->actions[j], *d->preds[*i], true );
		for ( unsigned j = 0; b && j < d->actions.size(); ++j )
			if ( isProducer( *d->actions[j], *d->preds[*i], false ) )
				prods[*i].insert( j );
	}
}

void identifyEnvelopes() {
	for ( std::map< unsigned, std::set< unsigned > >::iterator i = prods.begin(); i != prods.end(); ++i )
		for ( std::set< unsigned >::const_iterator j = i->second.begin(); j != i->second.end(); ++j ) {
//			std::cout << "Producer " << d->actions[*j]->name << " of resource " << d->preds[i->first]->name << "\n";
			for ( unsigned k = 0; k < d->actions.size(); ++k )
				if ( d->actions[k]->duration() < d->actions[*j]->duration() )
					for ( unsigned l = 0; l < d->actions[k]->pre_o.size(); ++l )
						if ( d->pmap[d->actions[k]->pre_o[l]->name] == (int)i->first ) {
							envs[*j].insert( i->first );
							conts[k].insert( i->first );
						}
		}
}

void typify( Action * a, Action & b, std::vector< int > & u, std::vector< std::string > & w ) {
	for ( unsigned i = 0; i < u.size(); ++i )
		w[i] = ins->objects[a->params[u[i]]][b.params[u[i]]];
}

// detect concurrency conditions on two actions
bool detectDependency( Action & a, Action & b ) {
//	std::cout << "detect " << a << "," << b << "\n";

	Action * A = d->actions[d->amap[a.name]];
	Action * B = d->actions[d->amap[b.name]];

	bool mutex = false;
	for ( unsigned i = 0; i < a.pre_s[0]->params.size(); ++i )
		for ( unsigned j = 0; j < b.pre_s[0]->params.size(); ++j )
			mutex |= a.pre_s[0]->params[i] == b.pre_s[0]->params[j];

//	if ( mutex ) std::cout << A->name << a.params << " and " << B->name << b.params << " are mutex!\n";

	if ( !mutex ) {
		std::map< unsigned, std::set< unsigned > >::iterator ia = conts.find( d->amap[a.name] );
		std::map< unsigned, std::set< unsigned > >::iterator ib = conts.find( d->amap[b.name] );

		for ( unsigned i = 0; i < A->pre_s.size(); ++i ) {
			Condition * c1 = A->pre_s[i];
			std::vector< std::string > p1( c1->params.size() );
			typify( A, a, c1->params, p1 );

			for ( unsigned j = 0; j < B->eff_s.size(); ++j ) {
				Condition * c2 = B->eff_s[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only if d(b) < d(a)
				if ( c1->name == c2->name && p1 == p2 && c2->neg && B->duration() < A->duration() ) {
					sepe = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.d.i of Sep(e)\n";
					if ( !seps && !sepe ) return true;
				}
				// only for contents
				if ( c1->name == c2->name && p1 == p2 && !c2->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.i of SHE(s)\n";
				}
			}
			for ( unsigned j = 0; j < B->eff_e.size(); ++j ) {
				Condition * c2 = B->eff_e[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// always
				if ( c1->name == c2->name && p1 == p2 && c2->neg ) {
					sepe = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.a of Sep(e)\n";
					if ( !seps && !sepe ) return true;
				}
				// only for contents
				if ( c1->name == c2->name && p1 == p2 && !c2->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.i of SHE(s)\n";
				}
			}
		}

		for ( unsigned i = 0; i < A->eff_s.size(); ++i ) {
			Condition * c1 = A->eff_s[i];
			std::vector< std::string > p1( c1->params.size() );
			typify( A, a, c1->params, p1 );

			for ( unsigned j = 0; j < B->pre_s.size(); ++j ) {
				Condition * c2 = B->pre_s[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only if d(b) < d(a)
				if ( c1->name == c2->name && p1 == p2 && !c1->neg && B->duration() < A->duration() ) {
					sepe = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.d.ii of Sep(e)\n";
					if ( !seps && !sepe ) return true;
				}
				// only for contents
				if ( c1->name == c2->name && p1 == p2 && c1->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.ii of SHE(s)\n";
				}
			}
			for ( unsigned j = 0; j < B->pre_o.size(); ++j ) {
				Condition * c2 = B->pre_o[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only if c2 is not an invariant provided by a content
				if ( ib == conts.end() || ib->second.find( d->pmap[c2->name] ) == ib->second.end() ) {
					// only if d(b) < d(a)
					if ( c1->name == c2->name && p1 == p2 && !c1->neg && B->duration() < A->duration() ) {
						sepe = false;
						std::cout << a.name << a.params << " " << b.name << b.params << ": ";
						std::cout << c1->name << p1 << " violates Condition 2.d.ii of Sep(e)\n";
						if ( !seps && !sepe ) return true;
					}
					// only for contents
					if ( c1->name == c2->name && p1 == p2 && c1->neg && ia != conts.end() ) {
						std::cout << a.name << a.params << " " << b.name << b.params << ": ";
						std::cout << c1->name << p1 << " violates Condition 2.e.ii of SHE(s)\n";
					}
				}
			}
			for ( unsigned j = 0; j < B->pre_e.size(); ++j ) {
				Condition * c2 = B->pre_e[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// always
				if ( c1->name == c2->name && p1 == p2 && !c1->neg ) {
					sepe = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.b of Sep(e)\n";
					if ( !seps && !sepe ) return true;
				}
				// only for contents
				if ( c1->name == c2->name && p1 == p2 && c1->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.ii of SHE(s)\n";
				}
			}
			for ( unsigned j = 0; j < B->eff_s.size(); ++j ) {
				Condition * c2 = B->eff_s[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				if ( a != b || c1->params != c2->params || c1->params.size() < A->params.size() ) {
					if ( c1->name == c2->name && p1 == p2 && c1->neg != c2->neg ) {
						// only if d(b) < d(a)
						if ( B->duration() < A->duration() ) {
							sepe = false;
							std::cout << a.name << a.params << " " << b.name << b.params << ": ";
							std::cout << c1->name << p1 << " violates Condition 2.d.iii of Sep(e)\n";
							if ( !seps && !sepe ) return true;
						}
						// only for contents
						else if ( ia != conts.end() ) {
							std::cout << a.name << a.params << " " << b.name << b.params << ": ";
							std::cout << c1->name << p1 << " violates Condition 2.e.iii or 2.e.iv of SHE(s)\n";
						}
					}
				}
			}
			for ( unsigned j = 0; j < B->eff_e.size(); ++j ) {
				Condition * c2 = B->eff_e[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				if ( a != b || c1->params != c2->params || c1->params.size() < A->params.size() ) {
					if ( c1->name == c2->name && p1 == p2 && c1->neg != c2->neg ) {
						// always
						sepe = false;
						std::cout << a.name << a.params << " " << b.name << b.params << ": ";
						std::cout << c1->name << p1 << " violates Condition 2.c of Sep(e)\n";
						if ( !seps && !sepe ) return true;
					}
				}
			}
		}

		for ( unsigned i = 0; i < A->pre_e.size(); ++i ) {
			Condition * c1 = A->pre_e[i];
			std::vector< std::string > p1( c1->params.size() );
			typify( A, a, c1->params, p1 );

			for ( unsigned j = 0; j < B->eff_s.size(); ++j ) {
				Condition * c2 = B->eff_s[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only for contents
				if ( c1->name == c2->name && p1 == p2 && c2->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.i of SHE(e)\n";
				}
				// always
				if ( c1->name == c2->name && p1 == p2 && !c2->neg ) {
					seps = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.a of Sep(s)\n";
					if ( !seps && !sepe ) return true;
				}
			}
			for ( unsigned j = 0; j < B->eff_e.size(); ++j ) {
				Condition * c2 = B->eff_e[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only for contents
				if ( c1->name == c2->name && p1 == p2 && c2->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.i of SHE(e)\n";
				}
				// only if d(b) < d(a)
				if ( c1->name == c2->name && p1 == p2 && !c2->neg && B->duration() < A->duration() ) {
					seps = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.d.i of Sep(s)\n";
					if ( !seps && !sepe ) return true;
				}
			}
		}

		for ( unsigned i = 0; i < A->eff_e.size(); ++i ) {
			Condition * c1 = A->eff_e[i];
			std::vector< std::string > p1( c1->params.size() );
			typify( A, a, c1->params, p1 );

			for ( unsigned j = 0; j < B->pre_s.size(); ++j ) {
				Condition * c2 = B->pre_s[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only for contents
				if ( c1->name == c2->name && p1 == p2 && !c1->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.ii of SHE(e)\n";
				}
				// always
				if ( c1->name == c2->name && p1 == p2 && c1->neg ) {
					seps = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.b of Sep(s)\n";
					if ( !seps && !sepe ) return true;
				}
			}
			for ( unsigned j = 0; j < B->pre_o.size(); ++j ) {
				Condition * c2 = B->pre_o[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only if c2 is not an invariant provided by a content
				if ( ib == conts.end() || ib->second.find( d->pmap[c2->name] ) == ib->second.end() ) {
					// only for contents
					if ( c1->name == c2->name && p1 == p2 && !c1->neg && ia != conts.end() ) {
						std::cout << a.name << a.params << " " << b.name << b.params << ": ";
						std::cout << c1->name << p1 << " violates Condition 2.e.ii of SHE(e)\n";
					}
					// only if d(b) < d(a)
					if ( c1->name == c2->name && p1 == p2 && c1->neg && B->duration() < A->duration() ) {
						seps = false;
						std::cout << a.name << a.params << " " << b.name << b.params << ": ";
						std::cout << c1->name << p1 << " violates Condition 2.d.ii of Sep(s)\n";
						if ( !seps && !sepe ) return true;
					}
				}
			}
			for ( unsigned j = 0; j < B->pre_e.size(); ++j ) {
				Condition * c2 = B->pre_e[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				// only for contents
				if ( c1->name == c2->name && p1 == p2 && !c1->neg && ia != conts.end() ) {
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.e.ii of SHE(e)\n";
				}
				// only if d(b) < d(a)
				if ( c1->name == c2->name && p1 == p2 && c1->neg && B->duration() < A->duration() ) {
					seps = false;
					std::cout << a.name << a.params << " " << b.name << b.params << ": ";
					std::cout << c1->name << p1 << " violates Condition 2.d.ii of Sep(s)\n";
					if ( !seps && !sepe ) return true;
				}
			}
			for ( unsigned j = 0; j < B->eff_s.size(); ++j ) {
				Condition * c2 = B->eff_s[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				if ( a != b || c1->params != c2->params || c1->params.size() < A->params.size() ) {
					if ( c1->name == c2->name && p1 == p2 && c1->neg != c2->neg ) {
						// always
						seps = false;
						std::cout << a.name << a.params << " " << b.name << b.params << ": ";
						std::cout << c1->name << p1 << " violates Condition 2.c of Sep(s)\n";
						if ( !seps && !sepe ) return true;
					}
				}
			}
			for ( unsigned j = 0; j < B->eff_e.size(); ++j ) {
				Condition * c2 = B->eff_e[j];
				std::vector< std::string > p2( c2->params.size() );
				typify( B, b, c2->params, p2 );

				if ( a != b || c1->params != c2->params || c1->params.size() < A->params.size() ) {
					if ( c1->name == c2->name && p1 == p2 && c1->neg != c2->neg ) {
						// only if d(b) < d(a)
						if ( B->duration() < A->duration() ) {
							seps = false;
							std::cout << a.name << a.params << " " << b.name << b.params << ": ";
							std::cout << c1->name << p1 << " violates Condition 2.d.iii of Sep(s)\n";
							if ( !seps && !sepe ) return true;
						}
						// only for contents
						else if ( ia != conts.end() ) {
							std::cout << a.name << a.params << " " << b.name << b.params << ": ";
							std::cout << c1->name << p1 << " violates Condition 2.e.iii or 2.e.iv of SHE(e)\n";
						}
					}
				}
			}
		}
	}
	return false;
}

void detectDependencies() {
	for ( unsigned i = 0; i < actions.size(); ++i )
		for ( unsigned j = 0; j < actions.size(); ++j )
			if ( i != j && detectDependency( *actions[i], *actions[j] ) )
				return;
}

int findIndex( const std::string & s, const std::vector< int > & w ) {
	for ( unsigned i = 0; i < actions.size(); ++i )
		if ( actions[i]->name == s && actions[i]->params == w )
			return i;
	return -1;
}

int main( int argc, char *argv[] ) {
	if ( argc < 4 ) {
		std::cout << "Usage: ./testSeparable <domain.pddl> <task.pddl> <output.sas>\n";
		exit( 1 );
	}

	d = new Domain( argv[1] );
	ins = new Instance( *d, argv[2] );

	std::vector< CondVec > v;
	parseTranslation( argv[3], v );

	identifyProducibles();

//	std::cout << prods << "\n";

	identifyEnvelopes();

//	std::cout << envs << " " << conts << "\n";

	// Detect dependencies among actions
	detectDependencies();

//	Action & a = *d->actions[d->amap["SWITCH_ON"]];
//	Action & b = *d->actions[d->amap["SWITCH_OFF"]];
//	for ( unsigned i = 0; i < a.params.size(); ++i )
//		std::cout << i << " " << a.params[i] << " " << ins->objects[a.params[i]] << "\n";
//	for ( unsigned i = 0; i < b.params.size(); ++i )
//		std::cout << i << " " << b.params[i] << " " << ins->objects[b.params[i]] << "\n";

//	std::vector< int > w( 2, 0 );
//	int x = findIndex( "SWITCH_ON", w );
//	int y = findIndex( "SWITCH_OFF", w );
//	std::cout << x << " " << y << "\n";

//	detectDependency( *actions[x], *actions[y] );
//	detectDependency( *actions[y], *actions[x] );

	for ( unsigned i = 0; i < actions.size(); ++i )
		delete actions[i];
	for ( unsigned i = 0; i < v.size(); ++i )
		for ( unsigned j = 0; j < v[i].size(); ++j )
			delete v[i][j];
	delete ins;
	delete d;
}
