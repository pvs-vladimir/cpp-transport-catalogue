#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

struct Stop {
	std::string name;
	Coordinates coordinates;
};

struct Bus {
	std::string name;
	std::vector<Stop*> stops;
};

struct Info {
	int stops_count;
	int unique_stops_count;
	double route_length;
};

class TransportCatalogue {
public:
	void AddStop(const Stop& stop);
	Stop* FindStop(std::string_view stop_name) const;
	void AddBus(const Bus& bus);
	Bus* FindBus(std::string_view bus_name) const;
	std::optional<Info> GetBusInfo(std::string_view bus_name) const;
	std::optional<std::set<std::string_view>> GetStopInfo(std::string_view stop_name) const;
private:
	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;
	std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_busnames_;
};

}