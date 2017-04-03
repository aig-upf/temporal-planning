#ifndef TP_ACTIONS_H
#define TP_ACTIONS_H

#define TP_INCREMENT 20
#define TP_UNITTIME 10000
#define TP_DEBUG false

#include <iomanip>
#include <iostream>

// print with three decimal points
inline void decimal_print(std::ostream & stream, unsigned n) {
	double res = (double)n/TP_UNITTIME;
	stream << std::setprecision(3) << std::fixed << res;
}

struct TemporalNode {
	std::string action;
	int duration;
	int index;
};

inline std::ostream & operator<<(std::ostream & stream, const TemporalNode & n) {
	return stream << n.action << " " << n.duration << " " << n.index;
}

#endif
