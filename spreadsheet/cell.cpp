#include "cell.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <optional>

using namespace std::literals;

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const { return {}; }
    virtual void InvalidateCache() {}
};


class Cell::EmptyImpl final : public Impl {
public:
    Value GetValue() const override {
        return ""s;
    }

    std::string GetText() const override {
        return ""s;
    }
};


class Cell::TextImpl final : public Impl {
public:
    TextImpl(std::string text) : text_(std::move(text)) {}

    Value GetValue() const override {
        if (text_.front() == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};


class Cell::FormulaImpl final : public Impl {
public:
    FormulaImpl(const SheetInterface& sheet, std::string text)
        : sheet_(sheet),
          formula_(ParseFormula(text.substr(1))) {}

    Value GetValue() const override {
        if (!cached_value_.has_value()) {
            cached_value_ = formula_->Evaluate(sheet_);
        }

        if (std::holds_alternative<double>(cached_value_.value())) {
            return std::get<double>(cached_value_.value());
        }

        return std::get<FormulaError>(cached_value_.value());
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }

    void InvalidateCache() override {
        cached_value_.reset();
    }

private:
    const SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cached_value_;
};


// Реализуйте следующие методы
Cell::Cell(SheetInterface& sheet, std::string text)
    : sheet_(sheet),
      impl_(new EmptyImpl) {
    Set(std::move(text));
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.front() == FORMULA_SIGN && text.size() > 1) {
        std::unique_ptr<Impl> new_impl = std::make_unique<FormulaImpl>(sheet_, text);
        for (const Position& ref_cell_pos : new_impl->GetReferencedCells()) {
            const CellInterface* referenced_cell = sheet_.GetCell(ref_cell_pos);
            if (!referenced_cell) {
                sheet_.SetCell(ref_cell_pos, ""s);
            }
        }
        impl_ = std::move(new_impl);
    }
    else if (!text.empty()) {
        impl_ = std::make_unique<TextImpl>(text);
    }
    else {
        impl_ = std::make_unique<EmptyImpl>();
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::InvalidateCache() {
    impl_->InvalidateCache();
}
