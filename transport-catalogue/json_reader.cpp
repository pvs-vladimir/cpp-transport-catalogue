#include "json_reader.h"

namespace transport_catalogue {
namespace json_reader {

using namespace std::literals;

JsonReader::JsonReader(std::istream& input) : data_(json::Load(input)) {
}

void JsonReader::LoadCatalogueData(TransportCatalogue& catalogue)  const {
    const auto it_end = data_.GetRoot().AsMap().end();
    auto it = data_.GetRoot().AsMap().find("base_requests"s);
    if (it != it_end) {
        const auto& base_requests = it->second.AsArray();
        LoadStopsData(catalogue, base_requests);
        LoadStopsDistances(catalogue, base_requests);
        LoadBusesData(catalogue, base_requests);
    }
}

std::vector<Request> JsonReader::LoadStatRequests() const {
    std::vector<Request> result;
    const auto it_end = data_.GetRoot().AsMap().end();
    auto it = data_.GetRoot().AsMap().find("stat_requests"s);
    if (it != it_end) {
        const auto& stat_requests = it->second.AsArray();
        for (const auto& stat_request : stat_requests) {
            const auto& map = stat_request.AsMap();
            if (auto error = CheckStatRequest(map); error.has_value()) {
                throw std::invalid_argument("Invalid stat request: "s + error.value());
            }
            Request request;
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
    const auto it_end = data_.GetRoot().AsMap().end();
    auto it = data_.GetRoot().AsMap().find("render_settings"s);
    if (it != it_end) {
        const auto& render_settings = it->second.AsMap();
        if (auto error = CheckRenderSettings(render_settings); error.has_value()) {
            throw std::invalid_argument("Invalid render settings: "s + error.value());
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

json::Document JsonReader::RenderAnswersJson(const request_handler::RequestHandler& handler, const std::vector<Request>& requests) const {
    json::Builder builder;

    builder.StartArray();
    for (const auto& request : requests) {
        if (request.type == "Bus"s) {
            builder.Value(GetBusJsonData(handler, request));
        } else if (request.type == "Stop"s) {
            builder.Value(GetStopJsonData(handler, request));
        } else if (request.type == "Map"s) {
            builder.Value(GetMapJsonData(handler, request));
        }
    }
    builder.EndArray();

    return json::Document(builder.Build());
}

std::optional<std::string> JsonReader::CheckStopData(const json::Dict& stop) const {
    const auto it_end = stop.end();
    auto it = stop.find("name"s);
    if (!(it != it_end && it->second.IsString())) {
        return "The stop name is not found or has an incorrect format"s;
    }
    it = stop.find("latitude"s);
    if (!(it != it_end && it->second.IsDouble())) {
        return "The stop latitude is not found or has an incorrect format"s; 
    }
    it = stop.find("longitude"s);
    if (!(it != it_end && it->second.IsDouble())) {
        return "The stop longitude is not found or has an incorrect format"s;
    }
    it = stop.find("road_distances"s);
    if (!(it != it_end && CheckStopDistances(it->second.AsMap()))) {
        return "The stop road_distances is not found or has an incorrect format"s;
    }
    return std::nullopt;
}

bool JsonReader::CheckStopDistances(const json::Dict& distances) const {
    for (const auto& [stop, distance] : distances) {
        if (!distance.IsInt()) {
            return false;
        }
    }
    return true;
}

std::optional<std::string> JsonReader::CheckBusData(const json::Dict& bus) const {
    const auto it_end = bus.end();
    auto it = bus.find("name"s);
    if (!(it != it_end && it->second.IsString())) {
        return "The bus name is not found or has an incorrect format"s;
    }
    it = bus.find("stops"s);
    if (!(it != it_end && CheckBusStops(it->second.AsArray()))) {
        return "The bus stops is not found or has an incorrect format"s;
    }
    it = bus.find("is_roundtrip"s);
    if (!(it != it_end && it->second.IsBool())) {
        return "The bus is_roundtrip is not found or has an incorrect format"s;
    }
    return std::nullopt;
}

bool JsonReader::CheckBusStops(const json::Array& stops) const {
    for (const auto& stop : stops) {
        if (!stop.IsString()) {
            return false;
        }
    }
    return true;
}

std::optional<std::string> JsonReader::CheckStatRequest(const json::Dict& request) const {
    const auto it_end = request.end();
    auto it = request.find("id"s);
    if (!(it != it_end && it->second.IsInt())) {
        return "Id is not found or has an incorrect format"s;
    }
    it = request.find("type"s);
    if (!(it != it_end && it->second.IsString())) {
        return "Type is not found or has an incorrect format"s;
    }
    if (it->second.AsString() == "Stop"s || it->second.AsString() == "Bus"s) {
        it = request.find("name"s);
        if (!(it != it_end && it->second.IsString())) {
            return "Stop/Bus name is not found or has an incorrect format"s;
        }
    }
    return std::nullopt;
}

namespace detail {
    
    std::optional<std::string> CheckWidth(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("width"s);
        if (!(it != it_end && it->second.IsDouble())) {
            return "The width is not found or has an incorrect format"s;
        }
        double width = it->second.AsDouble();
        if (!(width >= 0.0 && width <= 100000.0)) {
            return "The width is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckHeight(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("height"s);
        if (!(it != it_end && it->second.IsDouble())) {
            return "The height is not found or has an incorrect format"s;
        }
        double height = it->second.AsDouble();
        if (!(height >= 0.0 && height <= 100000.0)) {
            return "The height is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckPadding(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("padding"s);
        if (!(it != it_end && it->second.IsDouble())) {
            return "The padding is not found or has an incorrect format"s;
        }
        double padding = it->second.AsDouble();
        if (!(padding >= 0.0 && padding < std::min(settings.at("width"s).AsDouble(), settings.at("height"s).AsDouble()) / 2)) {
            return "The padding is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckLineWidth(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("line_width"s);
        if (!(it != it_end && it->second.IsDouble())) {
            return "The line_width is not found or has an incorrect format"s;
        }
        double line_width = it->second.AsDouble();
        if (!(line_width >= 0.0 && line_width <= 100000.0)) {
            return "The line_width is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckStopRadius(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("stop_radius"s);
        if (!(it != it_end && it->second.IsDouble())) {
            return "The stop_radius is not found or has an incorrect format"s;
        }
        double stop_radius = it->second.AsDouble();
        if (!(stop_radius >= 0.0 && stop_radius <= 100000.0)) {
            return "The stop_radius is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckBusLabelFontSize(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("bus_label_font_size"s);
        if (!(it != it_end && it->second.IsInt())) {
            return "The bus_label_font_size is not found or has an incorrect format"s;
        }
        int bus_label_font_size = it->second.AsInt();
        if (!(bus_label_font_size >= 0 && bus_label_font_size <= 100000)) {
            return "The bus_label_font_size is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckBusLabelOffset(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("bus_label_offset"s);
        if (!(it != it_end && it->second.IsArray())) {
            return "The bus_label_offset is not found or has an incorrect format"s;
        }
        const auto& offset = it->second.AsArray();
        if (offset.size() != 2) {
            return "The number of bus_label_offset is not equal to 2"s;
        }
        for (auto d : offset) {
            if (!(d.IsDouble() && d.AsDouble() >= -100000.0 && d.AsDouble() <= 100000.0)) {
                return "The bus_label_offset has incorrect offset"s;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckStopLabelFontSize(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("stop_label_font_size"s);
        if (!(it != it_end && it->second.IsInt())) {
            return "The stop_label_font_size is not found or has an incorrect format"s;
        }
        int stop_label_font_size = it->second.AsInt();
        if (!(stop_label_font_size >= 0 && stop_label_font_size <= 100000)) {
            return "The stop_label_font_size is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckStopLabelOffset(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("stop_label_offset"s);
        if (!(it != it_end && it->second.IsArray())) {
            return "The stop_label_offset is not found or has an incorrect format"s;
        }
        const auto& offset = it->second.AsArray();
        if (offset.size() != 2) {
            return "The number of stop_label_offset is not equal to 2"s;
        }
        for (auto d : offset) {
            if (!(d.IsDouble() && d.AsDouble() >= -100000.0 && d.AsDouble() <= 100000.0)) {
                return "The stop_label_offset has incorrect offset"s;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckColor(const json::Node& color) {
        if (color.IsString()) {
            return std::nullopt;
        }
        if (color.IsArray()) {
            const auto& array = color.AsArray();
            if (array.size() == 3) {
                for (const auto& elem : array) {
                    if (!(elem.IsInt() && elem.AsInt() >= 0 && elem.AsInt() <= 255)) {
                        return "incorrect rgb color"s;
                    }
                }
                return std::nullopt;
            } else if (array.size() == 4) {
                for (int i = 0; i < 3; ++i) {
                    if (!(array[i].IsInt() && array[i].AsInt() >= 0 && array[i].AsInt() <= 255)) {
                        return "incorrect rgba color"s;
                    }
                }
                if (!(array[3].IsDouble() && array[3].AsDouble() >= 0.0 && array[3].AsDouble() <= 1.0)) {
                    return "incorrect rgba color"s;
                }
                return std::nullopt;
            }
        }
        return "incorrect format color"s;
    }

    std::optional<std::string> CheckUnderlayerColor(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("underlayer_color"s);
        if (it == it_end) {
            return "The underlayer_color is not found"s;
        }
        if (auto error = CheckColor(it->second); error.has_value()) {
            return "The underlayer_color has an "s + error.value();
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckUnderlayerWidth(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("underlayer_width"s);
        if (!(it != it_end && it->second.IsDouble())) {
            return "The underlayer_width is not found or has an incorrect format"s;
        }
        double underlayer_width = it->second.AsDouble();
        if (!(underlayer_width >= 0.0 && underlayer_width <= 100000.0)) {
            return "The underlayer_width is out of range"s;
        }
        return std::nullopt;
    }

    std::optional<std::string> CheckColorPalette(const json::Dict& settings) {
        const auto it_end = settings.end();
        auto it = settings.find("color_palette"s);
        if (!(it != it_end && it->second.IsArray())) {
            return "The color_palette is not found or has an incorrect format"s;
        }
        for (const auto& color : it->second.AsArray()) {
            if (auto error = CheckColor(color); error.has_value()) {
                return "The color_palette has an "s + error.value();
            }
        }
        return std::nullopt;
    }

} // namespace detail

std::optional<std::string> JsonReader::CheckRenderSettings(const json::Dict& settings) const {
    if (auto error = detail::CheckWidth(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckHeight(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckPadding(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckLineWidth(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckStopRadius(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckBusLabelFontSize(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckBusLabelOffset(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckStopLabelFontSize(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckStopLabelOffset(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckUnderlayerColor(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckUnderlayerWidth(settings); error.has_value()) {return error;}
    if (auto error = detail::CheckColorPalette(settings); error.has_value()) {return error;}
    return std::nullopt;
}

void JsonReader::LoadStopsData(TransportCatalogue& catalogue, const json::Array& requests) const {
    for (const auto& request : requests) {
        const auto& map = request.AsMap();
        const auto it_end = map.end();
        auto it = map.find("type"s);
        if (!(it != it_end && it->second.IsString())) {
            throw std::invalid_argument("Invalid base request: Invalid type"s);
        }

        if (it->second.AsString() == "Stop"s) {
            if (auto error = CheckStopData(map); error.has_value()) {
                throw std::invalid_argument("Invalid base request: "s + error.value());
            }
            LoadStopData(catalogue, map);
        }
    }
}

void JsonReader::LoadStopData(TransportCatalogue& catalogue, const json::Dict& stop) const {
    geo::Coordinates coordinates = {stop.at("latitude"s).AsDouble(), stop.at("longitude"s).AsDouble()};
    catalogue.AddStop({stop.at("name"s).AsString(), coordinates});
}

void JsonReader::LoadStopsDistances(TransportCatalogue& catalogue, const json::Array& requests) const {
    for (const auto& request : requests) {
        const auto& map = request.AsMap();
        if (map.at("type"s) == "Stop"s) {
            LoadStopDistances(catalogue, map.at("name"s).AsString(), map.at("road_distances"s).AsMap());
        }
    }
}

void JsonReader::LoadStopDistances(TransportCatalogue& catalogue, const std::string& from, const json::Dict& distances) const {
    for (const auto& [to, distance] : distances) {
        catalogue.AddDistance(from, to, distance.AsInt());
    }
}

void JsonReader::LoadBusesData(TransportCatalogue& catalogue, const json::Array& requests) const {
    for (const auto& request : requests) {
        const auto& map = request.AsMap();
        if (map.at("type"s) == "Bus"s) {
            if (auto error = CheckBusData(map); error.has_value()) {
                throw std::invalid_argument("Invalid base request: "s + error.value());
            }
            LoadBusData(catalogue, map);
        }
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

json::Node JsonReader::GetBusJsonData(const request_handler::RequestHandler& handler, const Request& request) const {
    json::Builder builder;
    
    builder.StartDict().Key("request_id"s).Value(request.id);
    auto bus_stat = handler.GetBusInfo(request.name);
    if (bus_stat.has_value()) {
        builder.Key("stop_count"s).Value(bus_stat.value().stops_count)
               .Key("unique_stop_count"s).Value(bus_stat.value().unique_stops_count)
               .Key("route_length"s).Value(bus_stat.value().route_length)
               .Key("curvature"s).Value(bus_stat.value().curvature).EndDict();
    } else {
        builder.Key("error_message"s).Value("not found"s).EndDict();
    }

    return builder.Build();
}

json::Node JsonReader::GetStopJsonData(const request_handler::RequestHandler& handler, const Request& request) const {
    json::Builder builder;

    builder.StartDict().Key("request_id"s).Value(request.id);
    auto stop_stat = handler.GetStopInfo(request.name);
    if (stop_stat.has_value()) {
        builder.Key("buses"s).StartArray();
        for (auto bus : stop_stat.value()) {
            builder.Value(std::string{bus});
        }
        builder.EndArray().EndDict();
    } else {
        builder.Key("error_message"s).Value("not found"s).EndDict();
    }

    return builder.Build();
}

json::Node JsonReader::GetMapJsonData(const request_handler::RequestHandler& handler, const Request& request) const {
    json::Builder builder;

    builder.StartDict().Key("request_id"s).Value(request.id);
    auto map_svg = handler.RenderMap();
    std::stringstream strm;
    map_svg.Render(strm);
    builder.Key("map"s).Value(strm.str()).EndDict();

    return builder.Build();
}

} // json_reader
} // transport_catalogue