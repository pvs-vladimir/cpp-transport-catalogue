#pragma once

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace transport_catalogue {
namespace request_handler {

class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& catalogue, const map_renderer::MapRenderer& renderer);
    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusInfo> GetBusInfo(const std::string_view& bus_name) const;
    // Возвращает маршруты, проходящие через остановку (запрос Stop)
    std::optional<std::set<std::string_view>> GetStopInfo(const std::string_view& stop_name) const;
    // Возвращает svg-документ карты
    svg::Document RenderMap() const;

private:
    const TransportCatalogue& catalogue_;
    const map_renderer::MapRenderer& renderer_;
};

} // request_handler
} // transport_catalogue