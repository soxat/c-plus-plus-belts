#include "requests.h"

#include <vector>

#include "transport_router.h"

using namespace std;

namespace Requests {

  Json::Dict Stop::Process(const TransportCatalog& db) const {
    const auto* stop = db.GetStop(name);
    Json::Dict dict;
    if (!stop) {
      dict["error_message"] = Json::Node("not found"s);
    } else {
      Json::Array bus_nodes;
      bus_nodes.reserve(stop->bus_names.size());
      for (const auto& bus_name : stop->bus_names) {
        bus_nodes.emplace_back(bus_name);
      }
      dict["buses"] = Json::Node(move(bus_nodes));
    }
    return dict;
  }

  Json::Dict Bus::Process(const TransportCatalog& db) const {
    const auto* bus = db.GetBus(name);
    Json::Dict dict;
    if (!bus) {
      dict["error_message"] = Json::Node("not found"s);
    } else {
      dict = {
          {"stop_count", Json::Node(static_cast<int>(bus->stop_count))},
          {"unique_stop_count", Json::Node(static_cast<int>(bus->unique_stop_count))},
          {"route_length", Json::Node(static_cast<int>(bus->road_route_length))},
          {"curvature", Json::Node(bus->road_route_length / bus->geo_route_length)},
      };
    }
    return dict;
  }

  struct RouteItemResponseBuilder {
    Json::Dict operator()(const TransportRouter::RouteInfo::RideBusItem& bus_item) const {
      return Json::Dict{{"type", Json::Node("RideBus"s)},
                        {"bus", Json::Node(bus_item.bus_name)},
                        {"time", Json::Node(bus_item.time)},
                        {"span_count", Json::Node(static_cast<int>(bus_item.span_count))}};
    }
    Json::Dict operator()(const TransportRouter::RouteInfo::WaitBusItem& wait_item) const {
      return Json::Dict{
          {"type", Json::Node("WaitBus"s)},
          {"stop_name", Json::Node(wait_item.stop_name)},
          {"time", Json::Node(wait_item.time)},
      };
    }
    Json::Dict operator()(const TransportRouter::RouteInfo::WalkToCompanyItem& walk_item) const {
      return Json::Dict{
          {"type", Json::Node("WalkToCompany"s)},
          {"stop_name", Json::Node(walk_item.stop_name)},
          {"company", Json::Node(walk_item.company->cached_main_name())},
          {"time", Json::Node(walk_item.time)},
      };
    }
    Json::Dict operator()(const TransportRouter::RouteInfo::WaitCompanyItem& wait_item) const {
      return Json::Dict{
          {"type", Json::Node("WaitCompany"s)},
          {"company", Json::Node(wait_item.company->cached_main_name())},
          {"time", Json::Node(wait_item.time)},
      };
    }
  };

  Json::Dict Route::Process(const TransportCatalog& db) const {
    Json::Dict dict;
    const auto route = db.FindRoute(stop_from, stop_to);
    if (!route) {
      dict["error_message"] = Json::Node("not found"s);
    } else {
      dict["total_time"] = Json::Node(route->total_time);

      Json::Array items;
      items.reserve(route->items.size());
      for (const auto& item : route->items) {
        items.push_back(visit(RouteItemResponseBuilder{}, item));
      }

      dict["items"] = move(items);
      dict["map"] = Json::Node(db.RenderRoute(*route));
    }
    return dict;
  }

  Json::Dict Map::Process(const TransportCatalog& db) const {
    return Json::Dict{
        {"map", Json::Node(db.RenderMap())},
    };
  }

  Json::Dict FindCompanies::Process(const TransportCatalog& db) const {
    const auto companies = db.FindCompanies(filter);
    Json::Array items;
    items.reserve(companies.size());
    for (auto& company : companies) {
      items.push_back(move(company));
    }
    return Json::Dict{{"companies", move(items)}};
  }

  Json::Dict RouteToCompany::Process(const TransportCatalog& db) const {
    Json::Dict dict;
    auto route = db.FindRoute(datetime, stop_from, filter);
    if (!route) {
      dict["error_message"] = Json::Node("not found"s);
    } else {
      dict["total_time"] = Json::Node(route->total_time);
      Json::Array items;
      items.reserve(route->items.size());
      for (const auto& item : route->items) {
        items.push_back(visit(RouteItemResponseBuilder{}, item));
      }

      dict["items"] = move(items);

      if(!route->items.empty() && holds_alternative<TransportRouter::RouteInfo::WaitCompanyItem>(route->items.back())) {
        // so, get rid of this hack :troll:
        route->items.pop_back();
      }

      dict["map"] = Json::Node(db.RenderRoute(*route));
    }

    return dict;
  }

  variant<Stop, Bus, Route, Map, FindCompanies, RouteToCompany> Read(const Json::Dict& attrs) {
    const string& type = attrs.at("type").AsString();
    if (type == "Bus") {
      return Bus{attrs.at("name").AsString()};
    } else if (type == "Stop") {
      return Stop{attrs.at("name").AsString()};
    } else if (type == "Route") {
      return Route{attrs.at("from").AsString(), attrs.at("to").AsString()};
    } else if (type == "FindCompanies") {
      return FindCompanies{CompaniesFilter(attrs)};
    } else if (type == "RouteToCompany") {
      const auto& datetime = attrs.at("datetime").AsArray();
      return RouteToCompany{attrs.at("from").AsString(), CompaniesFilter(attrs.at("companies").AsMap()),
                            DateTime{datetime[0].AsInt(), datetime[1].AsInt(), datetime[2].AsInt()}};
    } else {
      return Map{};
    }
  }

  Json::Array ProcessAll(const TransportCatalog& db, const Json::Array& requests) {
    Json::Array responses;
    responses.reserve(requests.size());
    for (const Json::Node& request_node : requests) {
      Json::Dict dict =
          visit([&db](const auto& request) { return request.Process(db); }, Requests::Read(request_node.AsMap()));
      dict["request_id"] = Json::Node(request_node.AsMap().at("id").AsInt());
      responses.push_back(Json::Node(dict));
    }
    return responses;
  }
}  // namespace Requests
