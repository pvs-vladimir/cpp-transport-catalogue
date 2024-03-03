#pragma once

#include <string>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};

struct Bus {
	std::string name;
	std::vector<Stop*> stops;
	bool is_round;
};

} // transport_catalogue