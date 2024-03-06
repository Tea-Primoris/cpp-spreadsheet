#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <utility>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position"s);
    }

    ResizeSheet(pos);

    auto new_cell = std::make_unique<Cell>(*this, std::move(text));
    CheckForCircularReferences(new_cell.get(), pos);
    sheet_[pos.row][pos.col] = std::move(new_cell);
    UpdatePrintableSize(pos);
    InvalidateCache(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position"s);
    }

    if (static_cast<size_t>(pos.row) >= sheet_.size() || static_cast<size_t>(pos.col) >= sheet_.at(pos.row).size()) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface *>(const_cast<const Sheet *>(this)->GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (GetCell(pos)) {
        sheet_[pos.row][pos.col].reset();
        UpdatePrintableSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    auto value_printer = [&output](const std::unique_ptr<CellInterface>& cell) {
        auto visitor = [&output](const auto& value) {
            output << value;
        };
        std::visit(visitor, cell->GetValue());
    };
    PrintTable(output, value_printer);
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto text_printer = [&output](const std::unique_ptr<CellInterface>& cell) {
        output << cell->GetText();
    };
    PrintTable(output, text_printer);
}

void Sheet::AddDependenciesToMapFor(Position pos) {
    const CellInterface* const cell = GetCell(pos);
    for (const Position& referenced_cell : cell->GetReferencedCells()) {
        dependency_to_dependants_[referenced_cell].emplace(pos);
    }
}

void Sheet::InvalidateCache(Position pos) {
    for (const Position& dependant_pos : dependency_to_dependants_[pos]) {
        GetCell(dependant_pos)->InvalidateCache();
    }
}

void Sheet::CheckForCircularReferences(const Cell* cell, Position refered_pos) {
    for (const Position& referenced_pos : cell->GetReferencedCells()) {
        if (referenced_pos == refered_pos) {
            throw CircularDependencyException("Circular dependency!");
        }
        CheckForCircularReferences(referenced_pos, refered_pos);
    }
}

void Sheet::CheckForCircularReferences(const Position pos, const Position refered_pos) {
    const CellInterface* const cell = GetCell(pos);
    for (const Position& referenced_pos : cell->GetReferencedCells()) {
        if (referenced_pos == refered_pos) {
            throw CircularDependencyException("Circular dependency!");
        }
        CheckForCircularReferences(referenced_pos, refered_pos);
    }
}

void Sheet::UpdatePrintableSize(Position pos) {
    printable_size_.rows = std::max(printable_size_.rows, pos.row + 1);
    printable_size_.cols = std::max(printable_size_.cols, pos.col + 1);
}

void Sheet::UpdatePrintableSize() {
    printable_size_.cols = 0;
    printable_size_.rows = 0;
    for (size_t row = 0; row < sheet_.size(); ++row) {
        for (size_t col = 0; col < sheet_.at(row).size(); ++col) {
            if (sheet_[row][col]) {
                printable_size_.rows = std::max(printable_size_.rows, static_cast<int>(row + 1));
                printable_size_.cols = std::max(printable_size_.cols, static_cast<int>(col + 1));
            }
        }
    }
}

void Sheet::ResizeSheet(Position pos) {
    if (static_cast<size_t>(pos.row) >= sheet_.size()) {
        sheet_.resize(pos.row + 1);
    }
    if (static_cast<size_t>(pos.col) >= sheet_.at(pos.row).size()) {
        sheet_.at(pos.row).resize(pos.col + 1);
    }
}

void Sheet::PrintTable(std::ostream& output, const CellPrinter& printer) const {
    for (int row = 0; row < printable_size_.rows; ++row) {
        for (int col = 0; col < printable_size_.cols; ++col) {
            if (col != 0) {
                output << '\t';
            }
            if (static_cast<size_t>(col) < sheet_[row].size() && sheet_[row][col]) {
                printer(sheet_[row][col]);
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
