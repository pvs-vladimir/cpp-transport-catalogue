#include "json_reader.h"

namespace transport_catalogue {
namespace json_reader {

using namespace std::literals;

JsonReader::JsonReader(std::istream& input) : data_(json::Load(input)) {
}

void JsonReader::LoadCatalogueData(TransportCatalogue& catalogue)  const {
    if (data_.GetRoot().AsMap().count("base_requests"s) > 0) {
        const auto& base_requests = data_.GetRoot().AsMap().at("base_requests"s).AsArray();
        for (const auto& request : base_requests) {
            const auto& map = request.AsMap();
            if (map.count("type"s) == 0) {
                throw std::invalid_argument("Invalid base request: Invalid type"s);
            }

            if (map.at("type"s) == "Stop"s) {
                std::string error;
                if (!CheckStopData(map, error)) {
                    throw std::invalid_argument("Invalid base request: "s + error);
                }
                LoadStopData(catalogue, map);
            }
        }

        for (const auto& request : base_requests) {
            const auto& map = request.AsMap();
            if (map.at("type"s) == "Stop"s) {
                LoadStopDistances(catalogue, map.at("name"s).AsString(), map.at("road_distances"s).AsMap());
            }
        }

        for (const auto& request : base_requests) {
            const auto& map = request.AsMap();
            if (map.at("type"s) == "Bus"s) {
                std::string error;
                if (!CheckBusData(map, error)) {
                    throw std::invalid_argument("Invalid base request: "s + error);
                }
                LoadBusData(catalogue, map);
            }
        }
    }
}

std::vector<request_handler::Request> JsonReader::LoadStatRequests() const {
    std::vector<request_handler::Request> result;
    if (data_.GetRoot().AsMap().count("stat_requests"s) > 0) {
        const auto& stat_requests = data_.GetRoot().AsMap().at("stat_requests"s).AsArray();
        for (const auto& stat_request : stat_requests) {
            const auto& map = stat_request.AsMap();
            std::string error;
            if (!CheckStatRequest(map, error)) {
                throw std::invalid_argument("Invalid stat request: "s + error);
            }
            request_handler::Request request;
            request.id = map.at("id"s).AsInt();
            request.type = map.at("type"s).AsString();
            if (request.type == "Bus"s || request.type == "Stop"s) {
                request.name = map.at("name"s).AsString();
            }
            result.push_back(request);
        }
    }

    return result;
}

map_renderer::RenderSettings JsonReader::LoadRenderSettings() const {
    map_renderer::RenderSettings result;
    if (data_.GetRoot().AsMap().count("render_settings"s)) {
        const auto& render_settings = data_.GetRoot().AsMap().at("render_settings"s).AsMap();
        std::string error;
        if (!CheckRenderSettings(render_settings, error)) {
            throw std::invalid_argument("Invalid render settings: "s + error);
        }
        result.width = render_settings.at("width"s).AsDouble();
        result.height = render_settings.at("height"s).AsDouble();
        result.padding = render_settings.at("padding"s).AsDouble();
        result.line_width = render_settings.at("line_width"s).AsDouble();
        result.stop_radius = render_settings.at("stop_radius"s).AsDouble();
        result.bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
        result.bus_label_offset = LoadRenderOffset(render_settings.at("bus_label_offset"s).AsArray());
        result.stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();
        result.stop_label_offset = LoadRenderOffset(render_settings.at("stop_label_offset"s).AsArray());
        result.underlayer_color = LoadRenderColor(render_settings.at("underlayer_color"s));
        result.underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
        result.color_palette = LoadRenderColorPalette(render_settings.at("color_palette"s).AsArray());
    }

    return result;
}

bool JsonReader::CheckStopData(const json::Dict& stop, std::string& error) const {
    if (!(stop.count("name"s) > 0 && stop.at("name"s).IsString())) {
        error = "The stop name is not found or has an incorrect format"s;
        return false;
    }
    if (!(stop.count("latitude"s) > 0 && stop.at("latitude"s).IsDouble())) {
        error = "The stop latitude is not found or has an incorrect format"s; 
        return false;
    }
    if (!(stop.count("longitude"s) > 0 && stop.at("longitude"s).IsDouble())) {
        error = "The stop longitude is not found or has an incorrect format"s;
        return false;
    }
    if (!(stop.count("road_distances"s) > 0 && CheckStopDistances(stop.at("road_distances"s).AsMap()))) {
        error = "The stop road_distances is not found or has an incorrect format"s;
        return false;
    }
    return true;
}

bool JsonReader::CheckStopDistances(const json::Dict& distances) const {
    for (const auto& [stop, distance] : distances) {
        if (!distance.IsInt()) {
            return false;
        }
    }
    return true;
}

bool JsonReader::CheckBusData(const json::Dict& bus, std::string& error) const {
    if (!(bus.count("name"s) > 0 && bus.at("name"s).IsString())) {
        error = "The bus name is not found or has an incorrect format"s;
        return false;
    }
    if (!(bus.count("stops"s) > 0 && CheckBusStops(bus.at("stops"s).AsArray()))) {
        error = "The bus stops is not found or has an incorrect format"s;
        return false;
    }
    if (!(bus.count("is_roundtrip"s) > 0 && bus.at("is_roundtrip"s).IsBool())) {
        error = "The bus is_roundtrip is not found or has an incorrect format"s;
        return false;
    }
    return true;
}

bool JsonReader::CheckBusStops(const json::Array& stops) const {
    for (const auto& stop : stops) {
        if (!stop.IsString()) {
            return false;
        }
    }
    return true;
}

bool JsonReader::CheckStatRequest(const json::Dict& request, std::string& error) const {
    if (!(request.count("id"s) > 0 && request.at("id"s).IsInt())) {
        error = "Id is not found or has an incorrect format"s;
        return false;
    }
    if (!(request.count("type"s) > 0 && request.at("type"s).IsString())) {
        error = "Type is not found or has an incorrect format"s;
        return false;
    }
    if (request.at("type"s).AsString() == "Stop"s || request.at("type"s).AsString() == "Bus"s) {
        if (!(request.count("name"s) > 0 && request.at("name"s).IsString())) {
            error = "Stop/Bus name is not found or has an incorrect format"s;
            return false;
        }
    }
    return true;
}

namespace detail {
    
    bool CheckWidth(const json::Dict& settings, std::string& error) {
        if (!(settings.count("width"s) > 0 && settings.at("width"s).IsDouble())) {
            error = "The width is not found or has an incorrect format"s;
            return false;
        }
        double width = settings.at("width"s).AsDouble();
        if (!(width >= 0.0 && width <= 100000.0)) {
            error = "The width is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckHeight(const json::Dict& settings, std::string& error) {
        if (!(settings.count("height"s) > 0 && settings.at("height"s).IsDouble())) {
            error = "The height is not found or has an incorrect format"s;
            return false;
        }
        double height = settings.at("height"s).AsDouble();
        if (!(height >= 0.0 && height <= 100000.0)) {
            error = "The height is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckPadding(const json::Dict& settings, std::string& error) {
        if (!(settings.count("padding"s) > 0 && settings.at("padding"s).IsDouble())) {
            error = "The padding is not found or has an incorrect format"s;
            return false;
        }
        double padding = settings.at("padding"s).AsDouble();
        if (!(padding >= 0.0 && padding < std::min(settings.at("width"s).AsDouble(), settings.at("height"s).AsDouble()) / 2)) {
            error = "The padding is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckLineWidth(const json::Dict& settings, std::string& error) {
        if (!(settings.count("line_width"s) > 0 && settings.at("line_width"s).IsDouble())) {
            error = "The line_width is not found or has an incorrect format"s;
            return false;
        }
        double line_width = settings.at("line_width"s).AsDouble();
        if (!(line_width >= 0.0 && line_width <= 100000.0)) {
            error = "The line_width is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckStopRadius(const json::Dict& settings, std::string& error) {
        if (!(settings.count("stop_radius"s) > 0 && settings.at("stop_radius"s).IsDouble())) {
            error = "The stop_radius is not found or has an incorrect format"s;
            return false;
        }
        double stop_radius = settings.at("stop_radius"s).AsDouble();
        if (!(stop_radius >= 0.0 && stop_radius <= 100000.0)) {
            error = "The stop_radius is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckBusLabelFontSize(const json::Dict& settings, std::string& error) {
        if (!(settings.count("bus_label_font_size"s) > 0 && settings.at("bus_label_font_size"s).IsInt())) {
            error = "The bus_label_font_size is not found or has an incorrect format"s;
            return false;
        }
        int bus_label_font_size = settings.at("bus_label_font_size"s).AsInt();
        if (!(bus_label_font_size >= 0 && bus_label_font_size <= 100000)) {
            error = "The bus_label_font_size is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckBusLabelOffset(const json::Dict& settings, std::string& error) {
        if (!(settings.count("bus_label_offset"s) > 0 && settings.at("bus_label_offset"s).IsArray())) {
            error = "The bus_label_offset is not found or has an incorrect format"s;
            return false;
        }
        const auto& offset = settings.at("bus_label_offset"s).AsArray();
        if (offset.size() != 2) {
            error = "The number of bus_label_offset is not equal to 2"s;
            return false;
        }
        for (auto d : offset) {
            if (!(d.IsDouble() && d.AsDouble() >= -100000.0 && d.AsDouble() <= 100000.0)) {
                error = "The bus_label_offset has incorrect offset"s;
                return false;
            }
        }
        return true;
    }

    bool CheckStopLabelFontSize(const json::Dict& settings, std::string& error) {
        if (!(settings.count("stop_label_font_size"s) > 0 && settings.at("stop_label_font_size"s).IsInt())) {
            error = "The stop_label_font_size is not found or has an incorrect format"s;
            return false;
        }
        int stop_label_font_size = settings.at("stop_label_font_size"s).AsInt();
        if (!(stop_label_font_size >= 0 && stop_label_font_size <= 100000)) {
            error = "The stop_label_font_size is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckStopLabelOffset(const json::Dict& settings, std::string& error) {
        if (!(settings.count("stop_label_offset"s) > 0 && settings.at("stop_label_offset"s).IsArray())) {
            error = "The stop_label_offset is not found or has an incorrect format"s;
            return false;
        }
        const auto& offset = settings.at("stop_label_offset"s).AsArray();
        if (offset.size() != 2) {
            error = "The number of stop_label_offset is not equal to 2"s;
            return false;
        }
        for (auto d : offset) {
            if (!(d.IsDouble() && d.AsDouble() >= -100000.0 && d.AsDouble() <= 100000.0)) {
                error = "The stop_label_offset has incorrect offset"s;
                return false;
            }
        }
        return true;
    }

    bool CheckColor(const json::Node& color, std::string& error) {
        if (color.IsString()) {
            return true;
        }
        if (color.IsArray()) {
            const auto& array = color.AsArray();
            if (array.size() == 3) {
                for (const auto& elem : array) {
                    if (!(elem.IsInt() && elem.AsInt() >= 0 && elem.AsInt() <= 255)) {
                        error += "incorrect rgb color"s;
                        return false;
                    }
                }
                return true;
            } else if (array.size() == 4) {
                for (int i = 0; i < 3; ++i) {
                    if (!(array[i].IsInt() && array[i].AsInt() >= 0 && array[i].AsInt() <= 255)) {
                        error += "incorrect rgba color"s;
                        return false;
                    }
                }
                if (!(array[3].IsDouble() && array[3].AsDouble() >= 0.0 && array[3].AsDouble() <= 1.0)) {
                    error += "incorrect rgba color"s;
                    return false;
                }
                return true;
            }
        }
        error += "incorrect format color"s;
        return false;
    }

    bool CheckUnderlayerColor(const json::Dict& settings, std::string& error) {
        if (settings.count("underlayer_color"s) == 0) {
            error = "The underlayer_color is not found"s;
            return false;
        }
        error = "The underlayer_color has an "s;
        return CheckColor(settings.at("underlayer_color"s), error);
    }

    bool CheckUnderlayerWidth(const json::Dict& settings, std::string& error) {
        if (!(settings.count("underlayer_width"s) > 0 && settings.at("underlayer_width"s).IsDouble())) {
            error = "The underlayer_width is not found or has an incorrect format"s;
            return false;
        }
        double underlayer_width = settings.at("underlayer_width"s).AsDouble();
        if (!(underlayer_width >= 0.0 && underlayer_width <= 100000.0)) {
            error = "The underlayer_width is out of range"s;
            return false;
        }
        return true;
    }

    bool CheckColorPalette(const json::Dict& settings, std::string& error) {
        if (!(settings.count("color_palette"s) > 0 && settings.at("color_palette"s).IsArray())) {
            error = "The color_palette is not found or has an incorrect format"s;
            return false;
        }
        error = "The color_palette has an "s;
        for (const auto& color : settings.at("color_palette"s).AsArray()) {
            if (!CheckColor(color, error)) {
                return false;
            }
        }
        return true;
    }

} // namespace detail

bool JsonReader::CheckRenderSettings(const json::Dict& settings, std::string& error) const {
    if (!detail::CheckWidth(settings, error)) {return false;}
    if (!detail::CheckHeight(settings, error)) {return false;}
    if (!detail::CheckPadding(settings, error)) {return false;}
    if (!detail::CheckLineWidth(settings, error)) {return false;}
    if (!detail::CheckStopRadius(settings, error)) {return false;}
    if (!detail::CheckBusLabelFontSize(settings, error)) {return false;}
    if (!detail::CheckBusLabelOffset(settings, error)) {return false;}
    if (!detail::CheckStopLabelFontSize(settings, error)) {return false;}
    if (!detail::CheckStopLabelOffset(settings, error)) {return false;}
    if (!detail::CheckUnderlayerColor(settings, error)) {return false;}
    if (!detail::CheckUnderlayerWidth(settings, error)) {return false;}
    if (!detail::CheckColorPalette(settings, error)) {return false;}
    return true;
}

void JsonReader::LoadStopData(TransportCatalogue& catalogue, const json::Dict& stop) const {
    geo::Coordinates coordinates = {stop.at("latitude"s).AsDouble(), stop.at("longitude"s).AsDouble()};
    catalogue.AddStop({stop.at("name"s).AsString(), coordinates});
}

void JsonReader::LoadStopDistances(TransportCatalogue& catalogue, const std::string& from, const json::Dict& distances) const {
    for (const auto& [to, distance] : distances) {
        catalogue.AddDistance(from, to, distance.AsInt());
    }
}

void JsonReader::LoadBusData(TransportCatalogue& catalogue, const json::Dict& bus) const {
    std::vector<std::string> route;

    if (bus.at("is_roundtrip"s).AsBool()) {
        for (const auto& stop : bus.at("stops"s).AsArray()) {
            route.push_back(stop.AsString());
        }
    } else {
        const auto& array = bus.at("stops"s).AsArray();
        for (const auto& stop : array) {
            route.push_back(stop.AsString());
        }
        for (int i = array.size() - 2; i >= 0; --i) {
            route.push_back(array[i].AsString());
        }
    }

    std::vector<Stop*> stops;
    for (const std::string& stop : route) {
        stops.push_back(catalogue.FindStop(stop));
    }

    catalogue.AddBus({bus.at("name"s).AsString(), stops, bus.at("is_roundtrip"s).AsBool()});
}

svg::Point JsonReader::LoadRenderOffset(const json::Array& offset) const {
    return svg::Point(offset[0].AsDouble(), offset[1].AsDouble());
}

svg::Color JsonReader::LoadRenderColor(const json::Node& color) const {
    if (color.IsString()) {
        return color.AsString();
    }
    if (color.IsArray()) {
        const auto& array = color.AsArray();
        if (array.size() == 3) {
            return svg::Rgb(array[0].AsInt(), array[1].AsInt(), array[2].AsInt());
        } else {
            return svg::Rgba(array[0].AsInt(), array[1].AsInt(), array[2].AsInt(), array[3].AsDouble());
        }
    }
    return nullptr;
}

std::vector<svg::Color> JsonReader::LoadRenderColorPalette(const json::Array& pallete) const {
    std::vector<svg::Color> result;

    for (const auto& color : pallete) {
        result.push_back(LoadRenderColor(color));
    }
    return result;
}

} // json_reader
} // transport_catalogue