#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;  // copies for print, seats for electronic
    std::string department = ""; // Q9: department charged (empty = default)
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money cost;
    bool approved;
    std::string reason;  // why it was rejected; empty if approved
    bool cancelled = false; // Q8: set to true if this order was cancelled
    std::string department; // Q9: department charged
    std::string vendor;     // Q12: vendor selected (empty if using default pricing)
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget);

    void setTaxRates(int printPercent, int electronicPercent);

    // Q9: Register a department-specific budget
    void setDepartmentBudget(const std::string& dept, Budget budget);

    // Q12: Register a vendor's unit price for a resource
    void addVendorPrice(const std::string& resourceId, const std::string& vendorName, Money unitPrice);

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr,
                     const std::string& dept = "") const;

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument. On success the budget
    // and holdings are updated and the record is added to history.
    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                   const std::string& dept = "");

    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // Q11: If allOrNothing is true, buys all requests if all can succeed; otherwise buys none.
    std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest>& reqs,
                                             bool allOrNothing = false);

    // Q8: Cancels an approved order: refunds budget & quota, reduces holdings,
    // marks original record as cancelled, and appends a cancellation record.
    bool cancel(int orderNo);

    // Q10: Creates next year's budget for a department (or default if dept="")
    Budget rolloverBudget(const std::string& dept, double percent, Money baseBudget = Money()) const;

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;

    void printReport(std::ostream& os) const;

private:
    struct VendorOffer {
        std::string vendorName;
        Money unitPrice;
    };

    struct SelectedVendorInfo {
        std::string vendorName;
        Money preTaxCost;
        Money postTaxCost;
    };

    SelectedVendorInfo selectVendor(const Resource& r, int quantity) const;

    Money applyTax(const Resource& r, Money preTaxCost) const;
    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                        Money preTaxCost, Money postTaxCost,
                        bool approved, std::string reason,
                        const std::string& dept = "",
                        const std::string& vendor = "");

    const Budget& getBudgetFor(const std::string& dept) const;
    Budget& getBudgetFor(const std::string& dept);

    Catalog& catalog_;
    Budget& budget_;
    std::map<std::string, Budget> departmentBudgets_;
    std::map<std::string, std::vector<VendorOffer>> vendorPrices_;
    int printTaxPercent_ = 0;
    int electronicTaxPercent_ = 0;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
