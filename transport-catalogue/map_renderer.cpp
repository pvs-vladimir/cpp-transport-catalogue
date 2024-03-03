#include "map_renderer.h"

namespace transport_catalogue {
namespace map_renderer {

using namespace std::literals;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

MapRenderer::MapRenderer(const RenderSettings& settings) : settings_(settings) {
}

svg::Document MapRenderer::RenderMap(const Buses& buses, const Stops& stops) const {
    svg::Document document;
    std::vector<geo::Coordinates> coords;
    for (const auto& stop : stops) {
        coords.push_back(stop.second->coordinates);
    }
    const SphereProjector projector(coords.begin(), coords.end(), settings_.width, settings_.height, settings_.padding);

    RenderBusLines(buses, document, projector);
    RenderBusNames(buses, document, projector);
    RenderStopCircles(stops, document, projector);
    RenderStopNames(stops, document, projector);

    return document;
}

void MapRenderer::SetSettings(const RenderSettings& settings) {
    settings_ = settings;
}

RenderSettings MapRenderer::GetSettings() const {
    return settings_;
}

void MapRenderer::RenderBusLines(const Buses& buses, svg::Document& document, const SphereProjector& projector) const {
    int color_index = 0;
    
    for (const auto& [_, bus] : buses) {
        if (!bus->stops.empty()) {
            svg::Polyline bus_line;
            for (auto stop : bus->stops) {
                bus_line.AddPoint(projector(stop->coordinates));
            }
            bus_line.SetStrokeColor(settings_.color_palette[color_index])
                    .SetFillColor(svg::NoneColor)
                    .SetStrokeWidth(settings_.line_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            document.Add(bus_line);
            ++color_index;
        }

        if (color_index >= settings_.color_palette.size()) {
            color_index = 0;
        }
    }
}

void MapRenderer::RenderBusNames(const Buses& buses, svg::Document& document, const SphereProjector& projector) const {
    svg::Text text, underlayer;
    text.SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s);
    underlayer = text;
    underlayer.SetFillColor(settings_.underlayer_color)
              .SetStrokeColor(settings_.underlayer_color)
              .SetStrokeWidth(settings_.underlayer_width)
              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    int color_index = 0;
    
    for (const auto& [busname, bus] : buses) {
        if (!bus->stops.empty()) {
            svg::Point coord_start = projector(bus->stops[0]->coordinates);
            document.Add(underlayer.SetPosition(coord_start)
                                   .SetData(std::string(busname)));
            document.Add(text.SetPosition(coord_start)
                             .SetData(std::string(busname))
                             .SetFillColor(settings_.color_palette[color_index]));
            if (!bus->is_round) {
                svg::Point coord_end = projector(bus->stops[(bus->stops.size() - 1) / 2]->coordinates);
                if (coord_end != coord_start) {
                    document.Add(underlayer.SetPosition(coord_end)
                                       .SetData(std::string(busname)));
                    document.Add(text.SetPosition(coord_end)
                                .SetData(std::string(busname))
                                .SetFillColor(settings_.color_palette[color_index]));
                }
            }
            ++color_index;
        }

        if (color_index >= settings_.color_palette.size()) {
            color_index = 0;
        }
    }
}

void MapRenderer::RenderStopCircles(const Stops& stops, svg::Document& document, const SphereProjector& projector) const {
    svg::Circle stop_circle;
    stop_circle.SetRadius(settings_.stop_radius)
               .SetFillColor("white"s);
    for (const auto& [_, stop] : stops) {
        document.Add(stop_circle.SetCenter(projector(stop->coordinates)));
    }
}

void MapRenderer::RenderStopNames(const Stops& stops, svg::Document& document, const SphereProjector& projector) const {
    svg::Text text, underlayer;
    text.SetOffset(settings_.stop_label_offset)
        .SetFontSize(settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s);
    underlayer = text;
    underlayer.SetFillColor(settings_.underlayer_color)
              .SetStrokeColor(settings_.underlayer_color)
              .SetStrokeWidth(settings_.underlayer_width)
              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    text.SetFillColor("black"s);

    for (const auto& [stopname, stop] : stops) {
        svg::Point coord = projector(stop->coordinates);
        document.Add(underlayer.SetPosition(coord)
                               .SetData(std::string(stopname)));
        document.Add(text.SetPosition(coord)
                         .SetData(std::string(stopname)));
    }
}

} // map_renderer
} // transport_catalogue