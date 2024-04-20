#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <optional>
#include <unordered_map>
#include <utility>
#include <string>
#include <string_view>
#include <vector>

namespace transport_catalogue {
namespace transport_router {

struct RouterSettings {
    double bus_wait_time;
    double bus_velocity;
};

enum class EdgeType {WAIT, BUS};

struct EdgeInfo {
    EdgeType type;
    std::string_view name;
    std::optional<int> span_count;
    double time;
};

struct RouteInfo {
    double total_time;
    std::vector<EdgeInfo> items;
};

class TransportRouter {
public:
    explicit TransportRouter(const TransportCatalogue& catalogue, RouterSettings settings);
    std::optional<RouteInfo> GetRouteInfo(std::string_view from, std::string_view to) const;

private:
    RouterSettings settings_;
    std::unordered_map<std::string_view, std::pair<size_t, size_t>> stop_to_id_;
    std::unordered_map<size_t, EdgeInfo> edges_;
    graph::DirectedWeightedGraph<double> graph_;
    graph::Router<double> router_;

    double ComputeBusTime(double distance) const;
    graph::DirectedWeightedGraph<double> BuildGraph(const TransportCatalogue& catalogue);
    void AddVerticesToGraph(const std::unordered_map<std::string_view, Stop*>& stops);
    void AddWaitEdgesToGraph(graph::DirectedWeightedGraph<double>& graph, const std::unordered_map<std::string_view, Stop*>& stops);
    void AddBusesEdgesToGraph(graph::DirectedWeightedGraph<double>& graph, const TransportCatalogue& catalogue);
    void AddBusEdgesToGraph(graph::DirectedWeightedGraph<double>& graph, const TransportCatalogue& catalogue, std::string_view busname,
                            const std::vector<Stop*>& stops, size_t start_stop, size_t end_stop);
};

} // transport_router
} // transport_catalogue