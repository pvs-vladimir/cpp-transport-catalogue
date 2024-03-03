#include "request_handler.h"

namespace transport_catalogue {
namespace request_handler {

using namespace std::literals;

RequestHandler::RequestHandler(const TransportCatalogue& catalogue, const map_renderer::MapRenderer& renderer) 
    : catalogue_(catalogue), renderer_(renderer) {
}

std::optional<BusInfo> RequestHandler::GetBusInfo(const std::string_view& bus_name) const {
    return catalogue_.GetBusInfo(bus_name);
}

std::optional<std::set<std::string_view>> RequestHandler::GetStopInfo(const std::string_view& stop_name) const {
    return catalogue_.GetStopInfo(stop_name);
}

json::Document RequestHandler::RenderAnswersJson(const std::vector<Request>& requests) const {
    json::Array answers;
    
    for (const auto& request : requests) {
        if (request.type == "Bus"s) {
            answers.push_back({GetBusJsonData(request)});
        } else if (request.type == "Stop"s) {
            answers.push_back({GetStopJsonData(request)});
        } else if (request.type == "Map"s) {
            answers.push_back({GetMapJsonData(request)});
        }
    }

    return json::Document(json::Node(answers));
}

svg::Document RequestHandler::RenderMap() const {
    std::map<std::string_view, Stop*> stop_list;
    for (const auto& [name, ptr] : catalogue_.GetStopList()) {
        if (!catalogue_.GetStopInfo(name).value().empty()) {
            stop_list[name] = ptr;
        }
    }
    std::map<std::string_view, Bus*> bus_list;
    for (const auto& [name, ptr] : catalogue_.GetBusList()) {
        bus_list[name] = ptr;
    }
    return renderer_.RenderMap(bus_list, stop_list);
}

json::Dict RequestHandler::GetBusJsonData(const Request& request) const {
    json::Dict map;
    map["request_id"s] = json::Node(request.id);

    auto bus_stat = catalogue_.GetBusInfo(request.name);
    if (bus_stat.has_value()) {
        map["stop_count"s] = json::Node(bus_stat.value().stops_count);
        map["unique_stop_count"s] = json::Node(bus_stat.value().unique_stops_count);
        map["route_length"s] = json::Node(bus_stat.value().route_length);
        map["curvature"s] = json::Node(bus_stat.value().curvature);
    } else {
        map["error_message"s] = json::Node("not found"s);
    }

    return map;
}

json::Dict RequestHandler::GetStopJsonData(const Request& request) const {
    json::Dict map;
    map["request_id"s] = json::Node(request.id);

    auto stop_stat = catalogue_.GetStopInfo(request.name);
    if (stop_stat.has_value()) {
        json::Array buses;
        for (auto bus : stop_stat.value()) {
            buses.push_back(json::Node(std::string{bus}));
        }
        map["buses"s] = json::Node(buses);
    } else {
        map["error_message"s] = json::Node("not found"s);
    }

    return map;
}

json::Dict RequestHandler::GetMapJsonData(const Request& request) const {
    json::Dict map;
    map["request_id"s] = json::Node(request.id);

    auto map_svg = RenderMap();
    std::stringstream strm;
    map_svg.Render(strm);
    map["map"s] = json::Node(strm.str());

    return map;
}

} // request_handler
} // transport_catalogue