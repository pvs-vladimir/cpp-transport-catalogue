#include "svg.h"

#include <utility>
#include <sstream>

namespace svg {

using namespace std::literals;

Point::Point(double x, double y) : x(x), y(y) {
}

bool operator==(const Point& lhs, const Point& rhs) {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

bool operator!=(const Point& lhs, const Point& rhs) {
    return !(lhs == rhs);
}

RenderContext::RenderContext(std::ostream& out) : out(out) {
}

RenderContext::RenderContext(std::ostream& out, int indent_step, int indent)
    : out(out), indent_step(indent_step), indent(indent) {
}

RenderContext RenderContext::Indented() const {
    return {out, indent_step, indent + indent_step};
}

void RenderContext::RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
        out.put(' ');
    }
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << std::endl;
}

Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {
}

Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : red(r), green(g), blue(b), opacity(o) {
}
    
std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap) {
    using namespace std::literals;
    switch(line_cap) {
        case(StrokeLineCap::BUTT) :
            out << "butt"sv;
            break;
        case(StrokeLineCap::ROUND) :
            out << "round"sv;
            break;
        case(StrokeLineCap::SQUARE) :
            out << "square"sv;
            break;
    }
    return out;
}
    
std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join) {
    using namespace std::literals;
    switch(line_join) {
        case(StrokeLineJoin::ARCS) :
            out << "arcs"sv;
            break;
        case(StrokeLineJoin::BEVEL) :
            out << "bevel"sv;
            break;
        case(StrokeLineJoin::MITER) :
            out << "miter"sv;
            break;
        case(StrokeLineJoin::MITER_CLIP) :
            out << "miter-clip"sv;
            break;
        case(StrokeLineJoin::ROUND) :
            out << "round"sv;
            break;
    }
    return out;
}

void RenderColor::operator()(std::monostate) const {
    using namespace std::literals;
    out << "none"sv;
}

void RenderColor::operator()(std::string color) const {
    out << color;
}

void RenderColor::operator()(Rgb color) const {
    using namespace std::literals;
    out << "rgb("sv << static_cast<int>(color.red) << ","sv;
    out << static_cast<int>(color.green) << ","sv;
    out << static_cast<int>(color.blue) << ")"sv;
}

void RenderColor::operator()(Rgba color) const {
    using namespace std::literals;
    out << "rgba("sv << static_cast<int>(color.red) << ","sv;
    out << static_cast<int>(color.green) << ","sv;
    out << static_cast<int>(color.blue) << ","sv;
    out << color.opacity << ")"sv;
}
    
std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(RenderColor{out}, color);
    return out;
}

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << " />"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}
    
void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    for (size_t i = 0; i < points_.size(); ++i) {
        if (i > 0) {
            out << " "sv;
        }
        out << points_[i].x << ","sv << points_[i].y;
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}
    
Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = data;
    return *this;
}
    
void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << font_size_ << "\""sv;
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    RenderAttrs(out);
    out << ">"sv;
    for (const char& c : data_) {
        switch(c) {
            case ('"'):
                out << "&quot;"sv;
                break;
            case('\''):
                out << "&apos;"sv;
                break;
            case('<'):
                out << "&lt;"sv;
                break;
            case('>'):
                out << "&gt;"sv;
                break;
            case('&'):
                out << "&amp;"sv;
                break;
            default:
                out << c;
        }
    }
    out << "</text>"sv;
}
    
void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}
    
void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    for (size_t i = 0; i < objects_.size(); ++i) {
        objects_[i].get()->Render(out);
    }
    out << "</svg>"sv;
}
    
}  // namespace svg