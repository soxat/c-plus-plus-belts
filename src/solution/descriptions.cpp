#include "descriptions.h"

using namespace std;

namespace Descriptions {

  Stop Stop::ParseFrom(const Json::Dict& attrs) {
    Stop stop = {.name = attrs.at("name").AsString(),
                 .position =
                     {
                         .latitude = attrs.at("latitude").AsDouble(),
                         .longitude = attrs.at("longitude").AsDouble(),
                     },
                 .distances = {}};
    if (attrs.count("road_distances") > 0) {
      for (const auto& [neighbour_stop, distance_node] : attrs.at("road_distances").AsMap()) {
        stop.distances[neighbour_stop] = distance_node.AsInt();
      }
    }
    return stop;
  }

  Messages::Descriptions::Bus Bus::Serialize() const {
    Messages::Descriptions::Bus message;
    message.set_name(name);
    for (const auto& stop : stops) {
      *message.add_stops() = stop;
    }
    for (const auto& endpoint : endpoints) {
      *message.add_endpoints() = endpoint;
    }
    return message;
  }
  
  Bus Bus::ParseFrom(Messages::Descriptions::Bus message) {
    Bus bus;
    bus.name = move(*message.mutable_name());

    bus.stops.reserve(message.stops_size());
    for (auto& stop : *message.mutable_stops()) {
      bus.stops.push_back(move(stop));
    }

    bus.endpoints.reserve(2);
    for (auto& endpoint : *message.mutable_endpoints()) {
      bus.stops.push_back(move(endpoint));
    }
    return bus;
  }

  static vector<string> ParseStops(const Json::Array& stop_nodes, bool is_roundtrip) {
    vector<string> stops;
    stops.reserve(stop_nodes.size());
    for (const Json::Node& stop_node : stop_nodes) {
      stops.push_back(stop_node.AsString());
    }
    if (is_roundtrip || stops.size() <= 1) {
      return stops;
    }
    stops.reserve(stops.size() * 2 - 1);  // end stop is not repeated
    for (size_t stop_idx = stops.size() - 1; stop_idx > 0; --stop_idx) {
      stops.push_back(stops[stop_idx - 1]);
    }
    return stops;
  }

  int ComputeStopsDistance(const Stop& lhs, const Stop& rhs) {
    if (auto it = lhs.distances.find(rhs.name); it != lhs.distances.end()) {
      return it->second;
    } else {
      return rhs.distances.at(lhs.name);
    }
  }

  Bus Bus::ParseFrom(const Json::Dict& attrs) {
    const auto& name = attrs.at("name").AsString();
    const auto& stops = attrs.at("stops").AsArray();
    if (stops.empty()) {
      return Bus{.name = name, .stops = {}, .endpoints = {}};
    } else {
      Bus bus{.name = name,
              .stops = ParseStops(stops, attrs.at("is_roundtrip").AsBool()),
              .endpoints = {stops.front().AsString(), stops.back().AsString()}};
      if (bus.endpoints.back() == bus.endpoints.front()) {
        bus.endpoints.pop_back();
      }
      return bus;
    }
  }

  vector<InputQuery> ReadDescriptions(const Json::Array& nodes) {
    vector<InputQuery> result;
    result.reserve(nodes.size());

    for (const Json::Node& node : nodes) {
      const auto& node_dict = node.AsMap();
      if (node_dict.at("type").AsString() == "Bus") {
        result.push_back(Bus::ParseFrom(node_dict));
      } else {
        result.push_back(Stop::ParseFrom(node_dict));
      }
    }

    return result;
  }

}  // namespace Descriptions
