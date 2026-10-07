#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    return catalog_.get(id).costFor(quantity);
}
void AcquisitionManager::setTaxRates(int printPercent, int electronicPercent) {
    if (printPercent < 0 || electronicPercent < 0) {
        throw std::invalid_argument("tax rates must not be negative");
    }

    printTaxPercent_ = printPercent;
    electronicTaxPercent_ = electronicPercent;
}
Money AcquisitionManager::applyTax(const Resource& r, Money preTaxCost) const {
    const int rate = r.isDigital() ? electronicTaxPercent_ : printTaxPercent_;

    return Money::fromMinor(
        (preTaxCost.minorUnits() * (100 + rate)) / 100
    );
}

void AcquisitionManager::setDepartmentBudget(const std::string& dept, Budget budget) {
    departmentBudgets_.erase(dept);
    departmentBudgets_.emplace(dept, std::move(budget));
}

const Budget& AcquisitionManager::getBudgetFor(const std::string& dept) const {
    if (!dept.empty()) {
        auto it = departmentBudgets_.find(dept);
        if (it != departmentBudgets_.end()) return it->second;
    }
    return budget_;
}

Budget& AcquisitionManager::getBudgetFor(const std::string& dept) {
    if (!dept.empty()) {
        auto it = departmentBudgets_.find(dept);
        if (it != departmentBudgets_.end()) return it->second;
    }
    return budget_;
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason,
                                     const std::string& dept) const {
    std::string why;

    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Money preTaxCost = r->costFor(quantity);
            const Money postTaxCost = applyTax(*r, preTaxCost);
            const Budget& b = getBudgetFor(dept);
            why = b.check(r->category(), quantity, postTaxCost);
            if (why.empty())
                why = budget_.checkTitle(r->category(), r->title()); // Q7
        }
    } else {
        why = "resource not found: " + id;
    }

    if (reason) {
        *reason = why;
    }

    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(const Resource* r, const std::string& id,
                                           int qty, Money preTaxCost,
                                           Money postTaxCost, bool approved,
                                           std::string reason,
                                           const std::string& dept) {
    history_.push_back(PurchaseRecord{
    nextOrderNo_++, id, r ? r->title() : std::string("(unknown)"),
    r ? r->category() : ResourceCategory::Book, qty,
    preTaxCost, postTaxCost, approved,
    std::move(reason), false /* cancelled */, dept});
    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity,
                                               const std::string& dept) {
    const Resource& r = catalog_.get(id);        // may throw NotFoundError
    
    const Money preTaxCost = r.costFor(quantity); // may throw invalid_argument
    const Money postTaxCost = applyTax(r, preTaxCost);

    // Q7: check title quota before touching any state
    const std::string titleWhy = budget_.checkTitle(r.category(), r.title());
    if (!titleWhy.empty()) throw QuotaExceededError(titleWhy);

    Budget& b = getBudgetFor(dept);
    b.commit(r.category(), quantity, postTaxCost);  // post-tax cost
    catalog_.addHoldings(id, quantity);
    budget_.addTitle(r.category(), r.title()); // Q7: record after success

    return record(&r, id, quantity, preTaxCost, postTaxCost, true, {}, dept);
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs) {
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());
    for (const auto& req : reqs) {
        const Resource* r = catalog_.find(req.resourceId);
        Money preTaxCost;
        Money postTaxCost;
        std::string why;
        if (!r) {
            why = "resource not found: " + req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            preTaxCost = r->costFor(req.quantity);
            postTaxCost = applyTax(*r, preTaxCost);
            const Budget& b = getBudgetFor(req.department);
            why = b.check(r->category(), req.quantity, postTaxCost);
            if (why.empty())
                why = budget_.checkTitle(r->category(), r->title()); // Q7
        }

        if (why.empty()) {
            results.push_back(purchase(req.resourceId, req.quantity, req.department));
        } else {
           results.push_back(record(r, req.resourceId, req.quantity,
                         preTaxCost, postTaxCost, false, why, req.department));
        }
    }
    return results;
}

bool AcquisitionManager::cancel(int orderNo) {
    for (size_t i = 0; i < history_.size(); ++i) {
        if (history_[i].orderNo == orderNo) {
            if (!history_[i].approved || history_[i].cancelled) {
                return false;
            }
            history_[i].cancelled = true;

            const std::string resId = history_[i].resourceId;
            const int qty = history_[i].quantity;
            const Money preTax = history_[i].preTaxCost;
            const Money postTax = history_[i].cost;
            const ResourceCategory cat = history_[i].category;
            const std::string dept = history_[i].department;

            Budget& b = getBudgetFor(dept);
            b.revert(cat, qty, postTax);
            catalog_.addHoldings(resId, -qty);

            const Resource* r = catalog_.find(resId);
            record(r, resId, qty, preTax, postTax, false,
                   "Cancelled order #" + std::to_string(orderNo), dept);
            return true;
        }
    }
    return false;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& rec : history_)
        if (rec.approved && !rec.cancelled) sum += rec.cost;
    return sum;
}

Budget AcquisitionManager::rolloverBudget(const std::string& dept, double percent, Money baseBudget) const {
    const Budget& current = getBudgetFor(dept);
    return current.rollover(percent, baseBudget);
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";
    Money totalPreTax;
    for (const auto& rec : history_) {
        if (rec.approved && !rec.cancelled) {
            totalPreTax += rec.preTaxCost;
        }
        std::string statusStr = "REJECTED";
        if (rec.approved) {
            statusStr = rec.cancelled ? "CANCELLED" : "APPROVED";
        }
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << std::setw(9) << statusStr << "  " << std::setw(6)
           << rec.resourceId << " x" << std::setw(3) << rec.quantity << " "
           << "pre-tax " << rec.preTaxCost.toString()
           << "  post-tax " << rec.cost.toString() << "  "
           << rec.title;
        if (!rec.department.empty()) {
            os << "  [" << rec.department << "]";
        }
        if (!rec.reason.empty()) os << "\n        reason: " << rec.reason;
        os << "\n";
    }
    os << "Total pre-tax: " << totalPreTax << "\n";
    os << "Total post-tax: " << totalSpent() << "\n";
}

}  // namespace bookmgmt
