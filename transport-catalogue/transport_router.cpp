#include "transport_router.h"

namespace transport_catalogue {
namespace transport_router {

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, RouterSettings settings) 
    : settings_(std::move(settings))
    , stop_to_id_({})
    , edges_({})
    , graph_(BuildGraph(catalogue))
    , router_(graph_) {
}

std::optional<RouteInfo> TransportRouter::GetRouteInfo(std::string_view from, std::string_view to) const {
    RouteInfo result;
    auto route = router_.BuildRoute(stop_to_id_.at(from).first, stop_to_id_.at(to).first);
    if (!route) {
        return std::nullopt;
    }
    result.total_time = route->weight;
    for (auto id : route->edges) {
        result.items.push_back(edges_.at(id));
    }
    return result;
}

double TransportRouter::ComputeBusTime(double distance) const {
    const int m_in_km = 1000;
    const int min_in_hour = 60;
    double distance_in_km = distance / m_in_km;
    double time_in_hour = distance_in_km / settings_.bus_velocity;
    return time_in_hour * min_in_hour;
}

graph::DirectedWeightedGraph<double> TransportRouter::BuildGraph(const TransportCatalogue& catalogue) {
    const auto stops = catalogue.GetStopList();
    graph::DirectedWeightedGraph<double> graph(stops.size() * 2);
    AddVerticesToGraph(stops);
    AddWaitEdgesToGraph(graph, stops);
    AddBusesEdgesToGraph(graph, catalogue);
    return graph;
}

void TransportRouter::AddVerticesToGraph(const std::unordered_map<std::string_view, Stop*>& stops) {
    size_t id = 0;
    for (const auto& [stopname, _] : stops) {
        stop_to_id_[stopname] = {id , id + 1};
        id += 2;
    }
}

void TransportRouter::AddWaitEdgesToGraph(graph::DirectedWeightedGraph<double>& graph,
                                          const std::unordered_map<std::string_view, Stop*>& stops) {
    for (const auto& [stopname, _] : stops) {
        const auto [from, to] = stop_to_id_[stopname];
        size_t id = graph.AddEdge({from, to, settings_.bus_wait_time});
        edges_[id] = {EdgeType::WAIT, stopname, std::nullopt, settings_.bus_wait_time};
    }
}

void TransportRouter::AddBusesEdgesToGraph(graph::DirectedWeightedGraph<double>& graph, const TransportCatalogue& catalogue) {
    for (const auto& [busname, bus] : catalogue.GetBusList()) {
        if (bus->is_round) {
            AddBusEdgesToGraph(graph, catalogue, busname, bus->stops, 0, bus->stops.size() - 1);
        } else {
            size_t one_direction = bus->stops.size() / 2;
            AddBusEdgesToGraph(graph, catalogue, busname, bus->stops, 0, one_direction);
            AddBusEdgesToGraph(graph, catalogue, busname, bus->stops, one_direction, bus->stops.size() - 1);
        }
    }
}

void TransportRouter::AddBusEdgesToGraph(graph::DirectedWeightedGraph<double>& graph, const TransportCatalogue& catalogue, 
                                         std::string_view busname, const std::vector<Stop*>& stops, size_t start_stop, size_t end_stop) {
    for (size_t i = start_stop; i <= end_stop - 1; ++i) {
        auto stop_from = stops[i]->name;
        size_t id_from = stop_to_id_.at(stop_from).second;
        int span_count = 1;
        double weight = 0.0;
        for (size_t j = i + 1; j <= end_stop; ++j) {
            auto stop_to = stops[j]->name;
            size_t id_to = stop_to_id_.at(stop_to).first;
            weight += ComputeBusTime(*catalogue.GetDistance(stops[j - 1]->name, stop_to));
            size_t id = graph.AddEdge({id_from, id_to, weight});
            edges_[id] = {EdgeType::BUS, busname, span_count++, weight};
        }
    }
}

} // transport_router
} // transport_catalogue