#include "transport_catalogue.h"

#include <unordered_set>

namespace transport_catalogue {

void TransportCatalogue::AddStop(const Stop& stop) {
    stops_.push_back(stop);
    std::string_view stop_name{stops_.back().name};
    stopname_to_stop_[stop_name] = &stops_.back();
    stopname_to_busnames_[stop_name] = {};
}

Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
    if (stopname_to_stop_.count(stop_name) > 0) {
        return stopname_to_stop_.at(stop_name);
    }
    return nullptr;
}

std::unordered_map<std::string_view, Stop*> TransportCatalogue::GetStopList() const {
    return stopname_to_stop_;
}

void TransportCatalogue::AddBus(const Bus& bus) {
    buses_.push_back(bus);
    std::string_view bus_name{buses_.back().name};
    busname_to_bus_[bus_name] = &buses_.back();
    for (auto stop : buses_.back().stops) {
        std::string_view stop_name{stop->name};
        stopname_to_busnames_.at(stop_name).insert(bus_name);
    }
}

Bus* TransportCatalogue::FindBus(std::string_view bus_name) const {
    if (busname_to_bus_.count(bus_name) > 0) {
        return busname_to_bus_.at(bus_name);
    }
    return nullptr;
}

std::unordered_map<std::string_view, Bus*> TransportCatalogue::GetBusList() const {
    return busname_to_bus_;
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    if (busname_to_bus_.count(bus_name) == 0) {
        return std::nullopt;
    }
    BusInfo info;
    Bus bus = *busname_to_bus_.at(bus_name);
    info.stops_count = bus.stops.size();
    std::unordered_set<Stop*> unique_stops(bus.stops.begin(), bus.stops.end());
    info.unique_stops_count = unique_stops.size();
    int route_length = 0;
    double route_length_geo = 0.0;
    for (size_t i = 0; i < bus.stops.size() - 1; ++i) {
        route_length += GetDistance(bus.stops[i]->name, bus.stops[i + 1]->name).value();
        route_length_geo += ComputeDistance(bus.stops[i]->coordinates, bus.stops[i + 1]->coordinates);
    }
    info.route_length = route_length;
    info.curvature = route_length / route_length_geo;
    return info;
}

std::optional<std::set<std::string_view>> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    if (stopname_to_busnames_.count(stop_name) == 0) {
        return std::nullopt;
    }
    return stopname_to_busnames_.at(stop_name);
}

void TransportCatalogue::AddDistance(std::string_view from, std::string_view to, int distance) {
    std::pair<Stop*, Stop*> way = {FindStop(from), FindStop(to)};
    distances_[way] = distance;
}

std::optional<int> TransportCatalogue::GetDistance(std::string_view stop1, std::string_view stop2) const {
    Stop* stop1_p = FindStop(stop1);
    Stop* stop2_p = FindStop(stop2);
    if (stop1_p && stop2_p) {
        if (distances_.count({stop1_p, stop2_p}) > 0) {
            return distances_.at({stop1_p, stop2_p});
        } else if (distances_.count({stop2_p, stop1_p}) > 0) {
            return distances_.at({stop2_p, stop1_p});
        }
    }
    return std::nullopt;
}

}