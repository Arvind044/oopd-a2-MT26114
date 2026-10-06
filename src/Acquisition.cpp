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

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason) const {
    std::string why;

    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Money preTaxCost = r->costFor(quantity);
            const Money postTaxCost = applyTax(*r, preTaxCost);
            why = budget_.check(r->category(), quantity, postTaxCost);
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
                                           std::string reason) {
    history_.push_back(PurchaseRecord{
    nextOrderNo_++, id, r ? r->title() : std::string("(unknown)"),
    r ? r->category() : ResourceCategory::Book, qty,
    preTaxCost, postTaxCost, approved,
    std::move(reason)});
    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity) {
    const Resource& r = catalog_.get(id);        // may throw NotFoundError
    
    const Money preTaxCost = r.costFor(quantity); // may throw invalid_argument
    const Money postTaxCost = applyTax(r, preTaxCost);

    budget_.commit(r.category(), quantity, postTaxCost);  // post-tax cost
    catalog_.addHoldings(id, quantity);

    return record(&r, id, quantity, preTaxCost, postTaxCost, true, {});
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
            why = budget_.check(r->category(), req.quantity, postTaxCost);
        }

        if (why.empty()) {
            results.push_back(purchase(req.resourceId, req.quantity));
        } else {
           results.push_back(record(r, req.resourceId, req.quantity,
                         preTaxCost, postTaxCost, false, why));
        }
    }
    return results;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& rec : history_)
        if (rec.approved) sum += rec.cost;
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";
    Money totalPreTax;
    for (const auto& rec : history_) {
         if (rec.approved) {
            totalPreTax += rec.preTaxCost;
         }
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << (rec.approved ? "APPROVED" : "REJECTED") << "  " << std::setw(6)
           << rec.resourceId << " x" << std::setw(3) << rec.quantity << " "
           << "pre-tax " << rec.preTaxCost.toString()
           << "  post-tax " << rec.cost.toString() << "  "
            << rec.title;
        if (!rec.approved) os << "\n        reason: " << rec.reason;
        os << "\n";
    }
    os << "Total pre-tax: " << totalPreTax << "\n";
    os << "Total post-tax: " << totalSpent() << "\n";
}

}  // namespace bookmgmt
