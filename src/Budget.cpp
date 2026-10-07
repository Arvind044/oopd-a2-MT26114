#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {ResourceCategory::Book,
                                           ResourceCategory::ElectronicResource, 
                                           ResourceCategory::Journal,
                                           ResourceCategory::EBook,
                                            ResourceCategory::AudioBook,
                                            ResourceCategory::Thesis
                                        };
}

Budget::Budget(Money total) : total_(total) {
    if (total_.isNegative()) throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative())
        throw std::invalid_argument("quota limits must not be negative");
    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);
    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxSpend - usageFor(c).spent;
}

Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                 std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }
    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }
    {
        auto left = unitsRemaining(c);
        if (left && units > *left) {
            why = std::string(categoryName(c)) + " unit quota exceeded: requested " +
                  std::to_string(units) + ", " + std::to_string(*left) + " remaining";
            return Failure::Quota;
        }
    }
    {
        auto left = spendRemaining(c);
        if (left && cost > *left) {
            why = std::string(categoryName(c)) + " spend quota exceeded: cost " +
                  cost.toString() + ", " + left->toString() + " remaining";
            return Failure::Quota;
        }
    }
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " + cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }
    why.clear();
    return Failure::None;
}

// ── Q7: title quota ──────────────────────────────────────────────────────────
void Budget::setTitleQuota(ResourceCategory c, int maxTitles) {
    // Creates a default quota entry if none exists, then sets the title limit.
    quotas_[c].maxTitles = maxTitles;
}

std::string Budget::checkTitle(ResourceCategory c, const std::string& title) const {
    auto q = quotaFor(c);
    if (!q || q->maxTitles == 0) return "";          // no limit set
    const Usage& u = usageFor(c);
    if (u.titles.count(title)) return "";             // already known title
    if (static_cast<int>(u.titles.size()) >= q->maxTitles) {
        return std::string(categoryName(c)) + " title quota exceeded: already have " +
               std::to_string(u.titles.size()) + " title(s), max " +
               std::to_string(q->maxTitles);
    }
    return "";
}

void Budget::addTitle(ResourceCategory c, const std::string& title) {
    usage_[c].titles.insert(title);
}
// ─────────────────────────────────────────────────────────────────────────────

std::string Budget::check(ResourceCategory c, int units, Money cost) const {
    std::string why;
    evaluate(c, units, cost, why);
    return why;
}

void Budget::commit(ResourceCategory c, int units, Money cost) {
    std::string why;
    switch (evaluate(c, units, cost, why)) {
        case Failure::None: break;
        case Failure::BadInput: throw std::invalid_argument(why);
        case Failure::Quota: throw QuotaExceededError(why);
        case Failure::Overall: throw BudgetExceededError(why);
    }
    Usage& u = usage_[c];
    u.units += units;
    u.spent += cost;
    spent_ += cost;
}

void Budget::revert(ResourceCategory c, int units, Money cost) {
    Usage& u = usage_[c];
    u.units -= units;
    if (u.units < 0) u.units = 0;
    u.spent -= cost;
    if (u.spent.isNegative()) u.spent = Money();
    spent_ -= cost;
    if (spent_.isNegative()) spent_ = Money();
}

Budget Budget::rollover(double percent, Money baseBudget) const {
    if (percent < 0.0 || percent > 100.0) {
        throw std::invalid_argument("rollover percentage must be between 0 and 100");
    }
    if (baseBudget.isNegative()) {
        throw std::invalid_argument("base budget must not be negative");
    }
    const int64_t unspentMinor = remaining().minorUnits();
    const int64_t carryMinor = static_cast<int64_t>((unspentMinor * percent) / 100.0);
    const Money carryForward = Money::fromMinor(carryMinor);
    const Money newTotal = baseBudget + carryForward;

    Budget nextYear(newTotal);
    nextYear.quotas_ = quotas_; // preserve category quotas
    return nextYear;
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
       << "Spend used/max\n";
    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);
        const std::string units =
            std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
        const std::string spend =
            u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
        os << "  " << std::setw(20) << categoryName(c) << std::setw(18) << units << spend
           << "\n";
    }
}

}  // namespace bookmgmt
