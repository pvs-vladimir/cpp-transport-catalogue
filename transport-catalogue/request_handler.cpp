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

} // request_handler
} // transport_catalogue