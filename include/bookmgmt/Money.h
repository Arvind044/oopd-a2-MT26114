#pragma once
// Money: fixed-point currency amount stored in minor units (e.g. paise/cents)
// with an ISO currency code.
// Using an integer avoids floating-point rounding errors in cost/budget sums.

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <utility>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

class Money {
public:
    Money() : minor_(0), currency_("INR") {}

    // Construct from minor units: Money::fromMinor(12550) == 125.50 INR
    static Money fromMinor(std::int64_t minor, std::string currency = "INR") {
        if (currency.empty()) throw std::invalid_argument("currency cannot be empty");
        return Money(minor, std::move(currency));
    }

    // Construct from major + minor parts: Money::of(125, 50) == 125.50 INR
    static Money of(std::int64_t major, int minor = 0, std::string currency = "INR");

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currency() const { return currency_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;  // "1234.50" / "-3.05"

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(const Money& o) {
        requireSameCurrency(o);
        minor_ += o.minor_;
        return *this;
    }

    Money& operator-=(const Money& o) {
        requireSameCurrency(o);
        minor_ -= o.minor_;
        return *this;
    }

    Money& operator*=(std::int64_t k) {
        minor_ *= k;
        return *this;
    }

    friend Money operator+(Money a, const Money& b) { return a += b; }
    friend Money operator-(Money a, const Money& b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend bool operator==(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ == b.minor_;
    }
    friend bool operator!=(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ != b.minor_;
    }
    friend bool operator<(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ < b.minor_;
    }
    friend bool operator<=(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ <= b.minor_;
    }
    friend bool operator>(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ > b.minor_;
    }
    friend bool operator>=(const Money& a, const Money& b) {
        a.requireSameCurrency(b);
        return a.minor_ >= b.minor_;
    }

private:
    explicit Money(std::int64_t minor, std::string currency = "INR")
        : minor_(minor), currency_(std::move(currency)) {}

    void requireSameCurrency(const Money& other) const {
        if (currency_ != other.currency_) {
            throw CurrencyMismatchError(currency_, other.currency_);
        }
    }

    std::int64_t minor_ = 0;
    std::string currency_ = "INR";
};

std::ostream& operator<<(std::ostream& os, Money m);

}  // namespace bookmgmt
