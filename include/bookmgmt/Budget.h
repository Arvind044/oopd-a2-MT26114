#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats) and how much money may be spent on
// one category. Categories with no quota are limited only by the total budget.

#if __has_include(<optional>)
#  include <optional>
#elif __has_include(<experimental/optional>)
#  include <experimental/optional>
namespace std {
    using experimental::optional;
    using experimental::nullopt;
}
#endif
#include <iosfwd>
#include <map>
#include <string>
#include <unordered_set>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct Quota {
    int maxUnits;    // maximum copies/seats that may be bought
    Money maxSpend;  // maximum money that may be spent
    int maxTitles = 0; // maximum distinct titles per category (0 = no limit)
};

struct Usage {
    int units = 0;
    Money spent;
    std::unordered_set<std::string> titles; // distinct titles purchased
};

class Budget {
public:
    explicit Budget(Money total);

    Money total() const { return total_; }
    Money spent() const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory c, Quota q);
    // Q7: convenience wrapper to set only the title limit on an existing quota
    void setTitleQuota(ResourceCategory c, int maxTitles);
    // Q7: returns empty string if title is allowed, or a reason if the limit is hit
    std::string checkTitle(ResourceCategory c, const std::string& title) const;
    // Q7: record a distinct title after a successful purchase
    void addTitle(ResourceCategory c, const std::string& title);
    void removeQuota(ResourceCategory c);
    std::optional<Quota> quotaFor(ResourceCategory c) const;
    Usage usageFor(ResourceCategory c) const;

    // Remaining allowance in a category; nullopt means "no quota set".
    std::optional<int> unitsRemaining(ResourceCategory c) const;
    std::optional<Money> spendRemaining(ResourceCategory c) const;

    // Returns an empty string if the purchase fits, otherwise the reason it
    // does not. Does not change state.
    std::string check(ResourceCategory c, int units, Money cost) const;

    // Records a purchase. Throws QuotaExceededError / BudgetExceededError
    // (and changes nothing) if it would not fit.
    void commit(ResourceCategory c, int units, Money cost);

    // Q8: Refunds category units, category spending, and overall budget spending
    void revert(ResourceCategory c, int units, Money cost);

    // Q10: Creates next year's Budget carrying forward a percentage of unspent money.
    Budget rollover(double percent, Money baseBudget = Money()) const;

    void print(std::ostream& os) const;

private:
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory c, int units, Money cost, std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota> quotas_;
    std::map<ResourceCategory, Usage> usage_;
};

}  // namespace bookmgmt
