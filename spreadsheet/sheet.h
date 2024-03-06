#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Можете дополнить ваш класс нужными полями и методами

private:
    std::vector<std::vector<std::unique_ptr<CellInterface>>> sheet_;
    Size printable_size_;
    std::unordered_map<Position, std::unordered_set<Position, PositionHasher>, PositionHasher> dependency_to_dependants_;

    void AddDependenciesToMapFor(Position pos);

    void InvalidateCache(Position pos);

    void CheckForCircularReferences(const Cell* cell, Position refered_pos);
    void CheckForCircularReferences(Position pos, Position refered_pos);

    void UpdatePrintableSize(Position pos);

    void UpdatePrintableSize();

    void ResizeSheet(Position pos);

    using CellPrinter = std::function<void(const std::unique_ptr<CellInterface>&)>;
    void PrintTable(std::ostream& output, const CellPrinter& printer) const;
};
