#pragma once

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace transport_catalogue {
namespace request_handler {

class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& catalogue, const map_renderer::MapRenderer& renderer,
                   const transport_router::TransportRouter& router);
    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
    // Возвращает маршруты, проходящие через остановку (запрос Stop)
    std::optional<std::set<std::string_view>> GetStopInfo(std::string_view stop_name) const;
    // Возвращает svg-документ карты
    svg::Document RenderMap() const;
    // Возвращает описание маршрута между двумя остановками
    std::optional<transport_router::RouteInfo> GetRouteInfo(std::string_view from, std::string_view to) const;

private:
    const TransportCatalogue& catalogue_;
    const map_renderer::MapRenderer& renderer_;
    const transport_router::TransportRouter& router_;
};

} // request_handler
} // transport_catalogue