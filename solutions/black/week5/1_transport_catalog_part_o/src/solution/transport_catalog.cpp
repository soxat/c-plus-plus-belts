#include "transport_catalog.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <unordered_map>

#include "utils.h"

using namespace std;

TransportCatalog::TransportCatalog(Messages::TransportCatalog message) {
  for (auto& bus_message : *message.mutable_buses_responses()) {
    buses_[move(*bus_message.mutable_name())] = Bus{
        .stop_count = bus_message.stop_count(),
        .unique_stop_count = bus_message.unique_stop_count(),
        .road_route_length = bus_message.road_route_length(),
        .geo_route_length = bus_message.geo_route_length(),
    };
  }

  for (auto& stop_message : *message.mutable_stops_responses()) {
    stops_[move(*stop_message.mutable_name())] =
        Stop{.bus_names = set<string>(move_iterator(begin(*stop_message.mutable_bus_names())),
                                      move_iterator(end(*stop_message.mutable_bus_names())))};
  }
}

TransportCatalog::TransportCatalog(vector<Descriptions::InputQuery> data, const Json::Dict& routing_settings_json,
                                   const Json::Dict& render_settings_json) {
  auto stops_end =
      partition(begin(data), end(data), [](const auto& item) { return holds_alternative<Descriptions::Stop>(item); });

  Descriptions::StopsDict stops_dict;
  for (const auto& item : Range{begin(data), stops_end}) {
    const auto& stop = get<Descriptions::Stop>(item);
    stops_dict[stop.name] = &stop;
    stops_.insert({stop.name, {}});
  }

  Descriptions::BusesDict buses_dict;
  for (const auto& item : Range{stops_end, end(data)}) {
    const auto& bus = get<Descriptions::Bus>(item);

    buses_dict[bus.name] = &bus;
    buses_[bus.name] =
        Bus{bus.stops.size(), ComputeUniqueItemsCount(AsRange(bus.stops)),
            ComputeRoadRouteLength(bus.stops, stops_dict), ComputeGeoRouteDistance(bus.stops, stops_dict)};

    for (const string& stop_name : bus.stops) {
      stops_.at(stop_name).bus_names.insert(bus.name);
    }
  }

  router_ = make_unique<TransportRouter>(stops_dict, buses_dict, routing_settings_json);

  map_renderer_ = make_unique<MapRenderer>(stops_dict, buses_dict, render_settings_json);
  map_ = map_renderer_->Render();
}

const TransportCatalog::Stop* TransportCatalog::GetStop(const string& name) const {
  return GetValuePointer(stops_, name);
}

const TransportCatalog::Bus* TransportCatalog::GetBus(const string& name) const {
  return GetValuePointer(buses_, name);
}

optional<TransportRouter::RouteInfo> TransportCatalog::FindRoute(const string& stop_from, const string& stop_to) const {
  return router_->FindRoute(stop_from, stop_to);
}

string TransportCatalog::RenderMap() const {
  ostringstream out;
  map_.Render(out);
  return out.str();
}

string TransportCatalog::RenderRoute(const TransportRouter::RouteInfo& route) const {
  ostringstream out;
  BuildRouteMap(route).Render(out);
  return out.str();
}

int TransportCatalog::ComputeRoadRouteLength(const vector<string>& stops, const Descriptions::StopsDict& stops_dict) {
  int result = 0;
  for (size_t i = 1; i < stops.size(); ++i) {
    result += Descriptions::ComputeStopsDistance(*stops_dict.at(stops[i - 1]), *stops_dict.at(stops[i]));
  }
  return result;
}

double TransportCatalog::ComputeGeoRouteDistance(const vector<string>& stops,
                                                 const Descriptions::StopsDict& stops_dict) {
  double result = 0;
  for (size_t i = 1; i < stops.size(); ++i) {
    result += Sphere::Distance(stops_dict.at(stops[i - 1])->position, stops_dict.at(stops[i])->position);
  }
  return result;
}

Svg::Document TransportCatalog::BuildRouteMap(const TransportRouter::RouteInfo& route) const {
  return map_renderer_->RenderRoute(map_, route);
}

Messages::TransportCatalog TransportCatalog::Serialize() const {
  Messages::TransportCatalog message;

  for (auto& [bus_name, bus] : buses_) {
    Messages::Response_Bus bus_msg;
    bus_msg.set_name(bus_name);
    bus_msg.set_stop_count(bus.stop_count);
    bus_msg.set_unique_stop_count(bus.unique_stop_count);
    bus_msg.set_road_route_length(bus.road_route_length);
    bus_msg.set_geo_route_length(bus.geo_route_length);
    *message.add_buses_responses() = move(bus_msg);
  }

  for (auto& [stop_name, stop] : stops_) {
    Messages::Response_Stop stop_msg;
    stop_msg.set_name(stop_name);
    for_each(stop.bus_names.begin(), stop.bus_names.end(),
             [&stop_msg](const string& name) { stop_msg.add_bus_names(name); });
    *message.add_stops_responses() = move(stop_msg);
  }

  return message;
}

void SerializeTransportCatalog(const TransportCatalog& db, const Json::Dict& serialization_settings) {
  const string& filename = serialization_settings.at("file").AsString();
  ofstream output(filename, ios::binary);
  auto message = db.Serialize();
  message.SerializeToOstream(&output);
}

// namespace {
//   string ReadFileData(const string& file_name) {
//     ifstream file(file_name, ios::binary | ios::ate);
//     const ifstream::pos_type end_pos = file.tellg();
//     file.seekg(0, ios::beg);

//     string data(end_pos, '\0');
//     file.read(&data[0], end_pos);
//     return data;
//   }
// }  // namespace

TransportCatalog ParseTransportCatalog(const Json::Dict& serialization_settings) {
  const string& filename = serialization_settings.at("file").AsString();
  Messages::TransportCatalog message;
  ifstream file(filename, ios::binary);
  message.ParseFromIstream(&file);
  return TransportCatalog(move(message));
}