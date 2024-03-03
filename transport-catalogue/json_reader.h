#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace transport_catalogue {
namespace json_reader {

class JsonReader {
public:
    JsonReader(std::istream& input);
    void LoadCatalogueData(TransportCatalogue& catalogue) const;
    std::vector<request_handler::Request> LoadStatRequests() const;
    map_renderer::RenderSettings LoadRenderSettings() const;

private:
    json::Document data_;

    bool CheckStopData(const json::Dict& stop, std::string& error) const;
    bool CheckStopDistances(const json::Dict& distances) const;
    bool CheckBusData(const json::Dict& bus, std::string& error) const;
    bool CheckBusStops(const json::Array& stops) const;
    bool CheckStatRequest(const json::Dict& request, std::string& error) const;
    bool CheckRenderSettings(const json::Dict& settings, std::string& error) const;
    void LoadStopData(TransportCatalogue& catalogue, const json::Dict& stop) const;
    void LoadStopDistances(TransportCatalogue& catalogue, const std::string& from, const json::Dict& distances) const;
    void LoadBusData(TransportCatalogue& catalogue, const json::Dict& bus) const;
    svg::Point LoadRenderOffset(const json::Array& offset) const;
    svg::Color LoadRenderColor(const json::Node& color) const;
    std::vector<svg::Color> LoadRenderColorPalette(const json::Array& pallete) const;
};

} // json_reader
} // transport_catalogue