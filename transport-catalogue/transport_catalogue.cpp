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

std::optional<Info> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    if (busname_to_bus_.count(bus_name) == 0) {
        return std::nullopt;
    }
    Info info;
    Bus bus = *busname_to_bus_.at(bus_name);
    info.stops_count = bus.stops.size();
    std::unordered_set<Stop*> unique_stops(bus.stops.begin(), bus.stops.end());
    info.unique_stops_count = unique_stops.size();
    double route_length = 0.0;
    for (size_t i = 0; i < bus.stops.size() - 1; ++i) {
        route_length += ComputeDistance(bus.stops[i]->coordinates, bus.stops[i + 1]->coordinates);
    }
    info.route_length = route_length;
    return info;
}

std::optional<std::set<std::string_view>> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    if (stopname_to_busnames_.count(stop_name) == 0) {
        return std::nullopt;
    }
    return stopname_to_busnames_.at(stop_name);
}

}