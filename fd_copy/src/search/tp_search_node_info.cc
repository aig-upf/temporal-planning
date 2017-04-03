#include "tp_search_node_info.h"

bool TPSearchNodeInfo::is_applicable(const GlobalOperator * op) const {
	if ( TP_DEBUG )
		std::cout << "Checking applicability of " << op->get_name() << "\n";

	unsigned ix = op->get_name().find( '-' );
	std::string command = op->get_name().substr( 0, ix );
	std::string real_name = op->get_name().substr( ix + 1 );

	if ( command == "pop" || command == "end" ) {
		// find active action to be ended
		std::list< TemporalNode >::const_iterator i, k;
		for ( k = active_actions.begin(); k != active_actions.end(); ++k )
			if ( k->action == real_name ) break;

		if ( TP_DEBUG ) {
			std::cout << last_event.first << "," << last_event.second << "," << active_actions.size() << "\n";
			for ( unsigned i = 0; i < distance_matrix.size(); ++i ) {
				for ( unsigned j = 0; j < distance_matrix.size(); ++j )
					std::cout << distance_matrix[i][j] << " ";
				std::cout << "\n";
			}
		}

		// MORE EFFICIENT UPDATES !!! ???

		vector< vector< int > > temp_matrix = distance_matrix;

		// update STN and recheck for negative cycles
		if ( k->duration - last_event.second < temp_matrix[k->index][last_event.first] ) {
			temp_matrix[k->index][last_event.first] = k->duration - last_event.second;
			if ( !schedule( k->index, last_event.first, temp_matrix ) ) return false;
		}

		// this I'm not clear on
		if ( first_parallel_event.first >= 0 &&
		     first_parallel_event.second - k->duration < temp_matrix[first_parallel_event.first][k->index] ) {
			temp_matrix[first_parallel_event.first][k->index] = first_parallel_event.second - k->duration;
			if ( !schedule( first_parallel_event.first, k->index, temp_matrix ) ) return false;
		}

		// tentatively attempt to end other actions after current
		for ( i = active_actions.begin(); i != active_actions.end(); ++i )
			if ( i->action != real_name &&
			     i->duration - k->duration < temp_matrix[i->index][k->index] ) {
				temp_matrix[i->index][k->index] = i->duration - k->duration;
				if ( !schedule( i->index, k->index, temp_matrix ) ) return false;
			}
	}

	return true;
}

bool TPSearchNodeInfo::schedule(unsigned k, unsigned n, vector< vector< int > > & temp_matrix) const {
	// possible optimization: consider future "end" actions of current active actions

	// first update row k
	for ( unsigned i = 0; i < temp_matrix.size(); ++i )
		if ( temp_matrix[k][n] + temp_matrix[n][i] < temp_matrix[k][i] ) {
			temp_matrix[k][i] = temp_matrix[k][n] + temp_matrix[n][i];
			if ( i == k && temp_matrix[i][i] < 0 ) {
				if ( TP_DEBUG ) std::cout << "Failed during update 1\n";
				return false;
			}
		}

	// then update all paths via k
	for ( unsigned i = 0; i < temp_matrix.size(); ++i )
		for ( unsigned j = 0; j < temp_matrix.size(); ++j )
			if ( temp_matrix[i][k] + temp_matrix[k][j] < temp_matrix[i][j] ) {
				temp_matrix[i][j] = temp_matrix[i][k] + temp_matrix[k][j];
				if ( i == j && temp_matrix[i][i] < 0 ) {
					if ( TP_DEBUG ) std::cout << "Failed during update 2\n";
					return false;
				}
			}

	if ( TP_DEBUG ) {
		std::cout << "Updated matrix:\n";
		for ( unsigned i = 0; i < temp_matrix.size(); ++i ) {
			for ( unsigned j = 0; j < temp_matrix.size(); ++j )
				std::cout << temp_matrix[i][j] << " ";
			std::cout << "\n";
		}
	}

	return true;
}

void TPSearchNodeInfo::update_temporal(const TPSearchNodeInfo & other) {
	if ( TP_DEBUG )
		std::cout << "Updating " << creating_operator->get_name() << "\n";

	unsigned ix = creating_operator->get_name().find( '-' );
	std::string command = creating_operator->get_name().substr( 0, ix );
	std::string real_name = creating_operator->get_name().substr( ix + 1 );

	last_event = other.last_event;
	active_actions = other.active_actions;
	first_parallel_event = other.first_parallel_event;
	distance_matrix = other.distance_matrix;

	if ( TP_DEBUG )
		std::cout << command << " " << creating_operator->get_cost() << " " << real_name << "\n";

	if ( command == "push" || command == "start" ) {
		if ( command == "start" && !active_actions.size() )
			distance_matrix.clear();

		TemporalNode node;
		node.action = real_name;
		node.duration = creating_operator->get_cost();
		node.index = distance_matrix.size();

		vector< int > v;
		for ( int i = 0; i < node.index; ++i )
			v.push_back( distance_matrix[other.last_event.first][i] - other.last_event.second );
		if ( node.index ) v[other.last_event.first] = 0 - other.last_event.second;

		distance_matrix.push_back( v );
		for ( int i = 0; i <= node.index; ++i ) {
			if ( i == first_parallel_event.first ) distance_matrix[i].push_back( first_parallel_event.second );
			else distance_matrix[i].push_back( 1000000000 );
		}

		if ( TP_DEBUG ) {
			std::cout << last_event.first << "," << last_event.second << "," << active_actions.size() << "\n";
			for ( unsigned i = 0; i < distance_matrix.size(); ++i ) {
				for ( unsigned j = 0; j < distance_matrix.size(); ++j )
					std::cout << distance_matrix[i][j] << " ";
				std::cout << "\n";
			}
		}

		if ( command == "push" ) {
			last_event = make_pair( node.index, 0 );

			if ( first_parallel_event.first < 0 )
				first_parallel_event = make_pair( node.index, 0 );
			// COULD THERE BE A NEED TO SCHEDULE ???
			//else
			//	schedule( first_parallel_event.first, node.index, distance_matrix );
		}
		else last_event = make_pair( node.index, TP_INCREMENT );

		active_actions.push_back( node );
	}
	else if ( command == "pop" || command == "end" ) {
		std::list< TemporalNode >::iterator i, k;
		for ( k = active_actions.begin(); k != active_actions.end(); ++k )
			if ( k->action == real_name ) break;

		if ( k->duration - other.last_event.second < distance_matrix[k->index][other.last_event.first] ) {
			distance_matrix[k->index][other.last_event.first] = k->duration - other.last_event.second;
			schedule( k->index, other.last_event.first, distance_matrix );
		}

		if ( command == "pop" ) {
			last_event = make_pair( k->index, k->duration );

			if ( first_parallel_event.first < 0 )
				first_parallel_event = make_pair( k->index, k->duration );
			else if ( first_parallel_event.second - k->duration < distance_matrix[first_parallel_event.first][k->index] ) {
				distance_matrix[first_parallel_event.first][k->index] = first_parallel_event.second - k->duration;
				schedule( first_parallel_event.first, k->index, distance_matrix );
			}
		}
		else last_event = make_pair( k->index, k->duration + TP_INCREMENT );

		if ( TP_DEBUG ) {
			std::cout << last_event.first << "," << last_event.second << "," << active_actions.size() << "\n";
			for ( unsigned i = 0; i < distance_matrix.size(); ++i ) {
				for ( unsigned j = 0; j < distance_matrix.size(); ++j )
					std::cout << distance_matrix[i][j] << " ";
				std::cout << "\n";
			}
		}
		
		active_actions.erase( k );

		for ( i = active_actions.begin(); i != active_actions.end(); ++i )
			if ( i->duration - last_event.second < distance_matrix[i->index][k->index] ) {
				distance_matrix[i->index][k->index] = i->duration - last_event.second;
				schedule( i->index, k->index, distance_matrix );
			}
	}
	else if ( command == "starting" ) {
		first_parallel_event = make_pair( -1, 0 );
		if ( !active_actions.size() ) distance_matrix.clear();
		else last_event.second += TP_INCREMENT;
	}

	if ( TP_DEBUG )
		std::cout << "Done updating\n";
}
