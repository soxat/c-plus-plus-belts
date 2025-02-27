#pragma once

#include <memory>
#include <variant>
#include <optional>
#include <unordered_set>
#include <functional>

#include "common.h"
#include "formula.h"
#include "utils.h"

class Cell : public ICell {
  const ISheet* sheet_;
  std::string raw_text_;
  std::unique_ptr<IFormula> formula_;
  mutable std::optional<ICell::Value> cached_formula_value_;
  PositionSet external_deps_;

 public:
  Cell(const ISheet* sheet, std::string text);
  Value GetValue() const override;
  std::string GetText() const override;
  std::vector<Position> GetReferencedCells() const override;

  bool ContainsFormula() const;
  const IFormula* GetFormula() const;

  IFormula::HandlingResult HandleInsertedRows(int before, int count);
  IFormula::HandlingResult HandleInsertedCols(int before, int count);
  IFormula::HandlingResult HandleDeletedRows(int first, int count);
  IFormula::HandlingResult HandleDeletedCols(int first, int count);

  void AddExternalDep(Position pos);
  void RemoveExternalDep(Position pos);
  const PositionSet& ExternalDeps() const;
  void InvalidateCache();
  bool IsCached() const;

 private:
  IFormula* GetFormula();
  void RebuildText(IFormula::HandlingResult result);
  void RebuildExternalDepsWith(std::function<void(std::vector<Position>& pos)>);
};