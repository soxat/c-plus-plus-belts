#include "svg.h"

using namespace std;

namespace Svg {
  void RenderColor(std::ostream& out, std::monostate) { out << "none"; }

  void RenderColor(std::ostream& out, const std::string& value) { out << value; }

  void RenderColor(std::ostream& out, Rgb rgb) {
    out << "rgb(" << static_cast<int>(rgb.red) << "," << static_cast<int>(rgb.green) << ","
        << static_cast<int>(rgb.blue) << ")";
  }

  void RenderColor(std::ostream& out, Rgba rgba) {
    out << "rgba(" << static_cast<int>(rgba.red) << "," << static_cast<int>(rgba.green) << ","
        << static_cast<int>(rgba.blue) << "," << rgba.opacity << ")";
  }

  void RenderColor(std::ostream& out, const Color& color) {
    visit([&out](const auto& value) { RenderColor(out, value); }, color);
  }

  Circle& Circle::SetCenter(Point point) {
    center_ = point;
    return *this;
  }
  Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
  }

  void Circle::Render(ostream& out) const {
    out << "<circle ";
    out << "cx=\"" << center_.x << "\" ";
    out << "cy=\"" << center_.y << "\" ";
    out << "r=\"" << radius_ << "\" ";
    PathProps::RenderAttrs(out);
    out << "/>";
  }

  Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
  }

  void Polyline::Render(ostream& out) const {
    out << "<polyline ";
    out << "points=\"";
    for (const Point point : points_) {
      out << point.x << "," << point.y << " ";
    }
    out << "\" ";
    PathProps::RenderAttrs(out);
    out << "/>";
  }

  Text& Text::SetPoint(Point point) {
    point_ = point;
    return *this;
  }

  Text& Text::SetOffset(Point point) {
    offset_ = point;
    return *this;
  }

  Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
  }

  Text& Text::SetFontFamily(const string& value) {
    font_family_ = value;
    return *this;
  }

  Text& Text::SetFontWeight(const string& value) {
    font_weight_ = value;
    return *this;
  }

  Text& Text::SetData(const string& data) {
    data_ = data;
    return *this;
  }

  void Text::Render(ostream& out) const {
    out << "<text ";
    out << "x=\"" << point_.x << "\" ";
    out << "y=\"" << point_.y << "\" ";
    out << "dx=\"" << offset_.x << "\" ";
    out << "dy=\"" << offset_.y << "\" ";
    out << "font-size=\"" << font_size_ << "\" ";
    if (font_family_) {
      out << "font-family=\"" << *font_family_ << "\" ";
    }
    if (font_weight_) {
      out << "font-weight=\"" << *font_weight_ << "\" ";
    }
    PathProps::RenderAttrs(out);
    out << ">";
    out << data_;
    out << "</text>";
  }

  Rect& Rect::SetPoint(Point point) {
    point_ = point;
    return *this;
  }

  Rect& Rect::SetWidth(double width) {
    width_ = width;
    return *this;
  }

  Rect& Rect::SetHeight(double height) {
    height_ = height;
    return *this;
  }

  void Rect::Render(ostream& out) const {
    out << "<rect ";
    out << "x=\"" << point_.x << "\" ";
    out << "y=\"" << point_.y << "\" ";
    out << "width=\"" << width_ << "\" ";
    out << "height=\"" << height_ << "\" ";
    PathProps::RenderAttrs(out);
    out << "/>";
  }

  void Document::Render(ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";
    for (const auto& object_ptr : objects_) {
      object_ptr->Render(out);
    }
    out << "</svg>";
  }

}  // namespace Svg
