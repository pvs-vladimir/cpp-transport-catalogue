#pragma once

#include <iostream>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace transport_catalogue {
namespace json_reader {

struct Request {
    int id;
    std::string type;
    std::string name;
    std::pair<std::string, std::string> route;
};

class JsonReader {
public:
    JsonReader(std::istream& input);
    void LoadCatalogueData(TransportCatalogue& catalogue) const;
    std::vector<Request> LoadStatRequests() const;
    map_renderer::RenderSettings LoadRenderSettings() const;
    transport_router::RouterSettings LoadRouterSettings() const;
    json::Document RenderAnswersJson(const request_handler::RequestHandler& handler, const std::vector<Request>& requests) const;

private:
    json::Document data_;

    std::optional<std::string> CheckStopData(const json::Dict& stop) const;
    bool CheckStopDistances(const json::Dict& distances) const;
    std::optional<std::string> CheckBusData(const json::Dict& bus) const;
    bool CheckBusStops(const json::Array& stops) const;
    std::optional<std::string> CheckStatRequest(const json::Dict& request) const;
    std::optional<std::string> CheckRenderSettings(const json::Dict& settings) const;
    std::optional<std::string> CheckRouterSettings(const json::Dict& settings) const;
    void LoadStopsData(TransportCatalogue& catalogue, const json::Array& requests) const;
    void LoadStopData(TransportCatalogue& catalogue, const json::Dict& stop) const;
    void LoadStopsDistances(TransportCatalogue& catalogue, const json::Array& requests) const;
    void LoadStopDistances(TransportCatalogue& catalogue, const std::string& from, const json::Dict& distances) const;
    void LoadBusesData(TransportCatalogue& catalogue, const json::Array& requests) const;
    void LoadBusData(TransportCatalogue& catalogue, const json::Dict& bus) const;
    svg::Point LoadRenderOffset(const json::Array& offset) const;
    svg::Color LoadRenderColor(const json::Node& color) const;
    std::vector<svg::Color> LoadRenderColorPalette(const json::Array& pallete) const;
    json::Node GetBusJsonData(const request_handler::RequestHandler& handler, const Request& request) const;
    json::Node GetStopJsonData(const request_handler::RequestHandler& handler, const Request& request) const;
    json::Node GetMapJsonData(const request_handler::RequestHandler& handler, const Request& request) const;
    json::Node GetRouteJsonData(const request_handler::RequestHandler& handler, const Request& request) const;
};

} // json_reader
} // transport_catalogue