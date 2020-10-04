#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Json {

class Node;
using Dict = std::map<std::string, Node>;

class Node : std::variant<std::vector<Node>, Dict, bool, int, double, std::string, std::shared_ptr<const std::string>> {
 public:
  using variant::variant;
  const variant &GetBase() const { return *this; }

  const auto &AsArray() const { return std::get<std::vector<Node>>(*this); }
  const auto &AsMap() const { return std::get<Dict>(*this); }
  bool AsBool() const { return std::get<bool>(*this); }
  int AsInt() const { return std::get<int>(*this); }
  double AsDouble() const {
    return std::holds_alternative<double>(*this) ? std::get<double>(*this) : std::get<int>(*this);
  }
  const auto &AsString() const { return std::get<std::string>(*this); }
};

class Document {
 public:
  explicit Document(Node root) : root(move(root)) {}

  const Node &GetRoot() const { return root; }

 private:
  Node root;
};

Node LoadNode(std::istream &input);

Document Load(std::istream &input);

void PrintNode(const Node &node, std::ostream &output);

template <typename Value>
void PrintValue(const Value &value, std::ostream &output) {
  output << value;
}

template <>
void PrintValue<std::string>(const std::string &value, std::ostream &output);

template <>
void PrintValue<std::shared_ptr<const std::string>>(const std::shared_ptr<const std::string> &value,
                                                    std::ostream &output);

template <>
void PrintValue<bool>(const bool &value, std::ostream &output);

template <>
void PrintValue<std::vector<Node>>(const std::vector<Node> &nodes, std::ostream &output);

template <>
void PrintValue<Dict>(const Dict &dict, std::ostream &output);

void Print(const Document &document, std::ostream &output);

template <>
void PrintValue<Document>(const Document &doc, std::ostream &output);
std::ostream &operator<<(std::ostream &output, const Document &rhs);

bool operator==(const Document &lhs, const Document &rhs);
}  // namespace Json
