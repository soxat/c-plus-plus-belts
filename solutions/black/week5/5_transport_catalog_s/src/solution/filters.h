#pragma once

#include <string>
#include <vector>

#include "json.h"
#include "phone.pb.h"

struct CompaniesFilter {
  std::vector<std::string> names;
  std::vector<std::string> rubrics;
  std::vector<std::string> urls;
  std::vector<YellowPages::Phone> phones;

  CompaniesFilter() = default;
  CompaniesFilter(const Json::Dict& attrs);
};
