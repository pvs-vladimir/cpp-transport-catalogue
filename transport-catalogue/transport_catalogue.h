#pragma once

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <vector>

#include "domain.h"
#include "geo.h"

namespace transport_catalogue {

struct BusInfo {
	int stops_count;
	int unique_stops_count;
	int route_length;
	double curvature;
};

class PairPointersHasher {
public:

	std::size_t operator()(const std::pair<Stop*, Stop*>& pair) const {
		return hasher_(pair.first) * 37 + hasher_(pair.second);
	}

private:
	std::hash<const void*> hasher_;
};

class TransportCatalogue {
public:
	void AddStop(const Stop& stop);
	Stop* FindStop(std::string_view stop_name) const;
	std::unordered_map<std::string_view, Stop*> GetStopList() const;
	void AddBus(const Bus& bus);
	Bus* FindBus(std::string_view bus_name) const;
	std::unordered_map<std::string_view, Bus*> GetBusList() const;
	std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
	std::optional<std::set<std::string_view>> GetStopInfo(std::string_view stop_name) const;
	void AddDistance(std::string_view from, std::string_view to, int distance);
	std::optional<int> GetDistance(std::string_view from, std::string_view to) const;
private:
	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;
	std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_busnames_;
	std::unordered_map<std::pair<Stop*, Stop*>, int, PairPointersHasher> distances_;
};

}