// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    catalog.emplace<EBook>(
    "E001",
    "The C++ Programming Language",
    std::vector<std::string>{"Bjarne Stroustrup"},
    "978-0321563842",
    "Addison-Wesley",
    2013,
    Money::of(100),
    "https://example.com/cpp",
    LicenseModel::AnnualSubscription,
    Money::of(50),
    EBookFormat::PDF,
    true); 

    catalog.emplace<AudioBook>(
    "A001",
    "The C++ Programming Language - Audio",
    "Addison-Wesley",
    2013,
    Money::of(100),
    "https://example.com/audiobook",
    "John Smith",
    720,
    LicenseModel::AnnualSubscription,
    Money::of(50));   
    catalog.emplace<Thesis>(
    "T001",
    "Machine Learning Research",
    "IIIT Delhi",
    2026,
    Money::of(0),
    "IIIT Delhi",
    "M.Tech",
    "Dr. Arani Bhattacharya");
    
    

    catalog.emplace<Journal>("J001", "Nature Journal", "Springer", 2026,
                          Money::of(500), "2045-2322", 12, 2);                                    

    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");
    std::cout << "\n=== Details of E001 ===\n" << catalog.get("E001");
    std::cout << "\n=== Details of A001 ===\n" << catalog.get("A001");
    std::cout << "\n=== Details of T001 ===\n" << catalog.get("T001");

    Budget budget(Money::of(20000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {10, Money::of(10000)});

    AcquisitionManager acq(catalog, budget);

    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "10 copies of B001 = " << acq.quote("B001", 10) << "  (10% bulk discount)\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "60 seats of R001  = " << acq.quote("R001", 60)  << "  (seats beyond 50 are half price)\n";
    std::cout << "5 seats of E001   = " << acq.quote("E001", 5) << "  (inherited electronic pricing)\n";
    std::cout << "5 seats of A001   = " << acq.quote("A001", 5) << "  (inherited electronic pricing)\n";
    std::cout << "5 copies of T001  = " << acq.quote("T001", 5) << "  (free thesis)\n";
// Book and EBook both store authors and ISBN. This duplication could be
// avoided by extracting shared bibliographic fields/behavior into a common
// base class or reusable component, then deriving Book and EBook from it.
    std::cout << "3 copies of J001 = " << acq.quote("J001", 3) << "  (2-year subscription)\n";

    acq.processBatch({
        {"B001", 4},   // 1800  ok
        {"B002", 5},   // 6000  ok  -> book spend 7800
        {"B001", 1},   // 450   rejected: book spend quota (200 left)
        {"R001", 20},  // 5000  ok
        {"R002", 25},  // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15},  // 6000  ok  -> e-resource spend 11000
        {"R002", 5},   // 2000  rejected: e-resource spend quota (1000 left)
        {"X999", 1},   // rejected: unknown id
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
        std::cout << "\n=== Tax demonstration ===\n";

    Budget taxBudget(Money::of(10000));

    AcquisitionManager taxAcq(catalog, taxBudget);

    taxAcq.setTaxRates(10, 20);

    taxAcq.purchase("B001", 2);
    taxAcq.purchase("R001", 2);

    taxAcq.printReport(std::cout);

    // ── Q7: Title-quota demonstration ─────────────────────────────────────────
    std::cout << "\n=== Q7: Title Quota Demo ===\n";

    Catalog titleCatalog;
    titleCatalog.emplace<Book>("D001", "Design Patterns",
        std::vector<std::string>{"GoF"}, "isbn-dp", "AW", 1994, Money::of(300));
    titleCatalog.emplace<Book>("D002", "Refactoring",
        std::vector<std::string>{"Fowler"}, "isbn-rf", "AW", 1999, Money::of(280));
    titleCatalog.emplace<Book>("D003", "Clean Code",
        std::vector<std::string>{"Martin"}, "isbn-cc", "PH", 2008, Money::of(260));

    Budget titleBudget(Money::of(50000));
    // Allow at most 2 distinct Book titles
    titleBudget.setQuota(ResourceCategory::Book, {50, Money::of(40000), 2});

    AcquisitionManager titleAcq(titleCatalog, titleBudget);

    std::cout << "Buying 3 copies of 'Design Patterns'...\n";
    titleAcq.purchase("D001", 3);

    std::cout << "Buying 2 more copies of 'Design Patterns' (same title, OK)...\n";
    titleAcq.purchase("D001", 2);

    std::cout << "Buying 1 copy of 'Refactoring' (2nd distinct title)...\n";
    titleAcq.purchase("D002", 1);

    std::cout << "Attempting to buy 'Clean Code' (3rd distinct title, limit=2)...\n";
    try {
        titleAcq.purchase("D003", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "  -> QuotaExceededError: " << e.what() << "\n";
    }

    titleAcq.printReport(std::cout);
    titleBudget.print(std::cout);
    // ─────────────────────────────────────────────────────────────────────────

    // ── Q8: Order Cancellation demonstration ──────────────────────────────────
    std::cout << "\n=== Q8: Order Cancellation Demo ===\n";

    Catalog cancelCatalog;
    cancelCatalog.emplace<Book>("C001", "Structure and Interpretation of Computer Programs",
        std::vector<std::string>{"Abelson", "Sussman"}, "978-0262510875", "MIT Press", 1996, Money::of(500));

    Budget cancelBudget(Money::of(10000));
    cancelBudget.setQuota(ResourceCategory::Book, {10, Money::of(5000)});

    AcquisitionManager cancelAcq(cancelCatalog, cancelBudget);

    std::cout << "Initial state: Holdings = " << cancelCatalog.holdings("C001")
              << ", Spent = " << cancelBudget.spent() << "\n";

    std::cout << "Purchasing 4 copies of C001...\n";
    const auto& order = cancelAcq.purchase("C001", 4);
    std::cout << "Approved Order #" << order.orderNo << ", Spent = " << cancelBudget.spent()
              << ", Holdings = " << cancelCatalog.holdings("C001") << "\n";

    std::cout << "Cancelling Order #" << order.orderNo << "...\n";
    bool success = cancelAcq.cancel(order.orderNo);
    std::cout << "Cancellation result: " << (success ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "After cancellation: Spent = " << cancelBudget.spent()
              << ", Holdings = " << cancelCatalog.holdings("C001") << "\n";

    std::cout << "\n=== Cancellation Report ===\n";
    cancelAcq.printReport(std::cout);
    // ─────────────────────────────────────────────────────────────────────────

    // ── Q9: Department Budgets demonstration ─────────────────────────────────
    std::cout << "\n=== Q9: Department Budgets Demo ===\n";

    Catalog deptCatalog;
    deptCatalog.emplace<Book>("CS1", "Introduction to Algorithms",
        std::vector<std::string>{"Cormen", "Leiserson"}, "978-0262033848", "MIT Press", 2009, Money::of(600));
    deptCatalog.emplace<Book>("PH1", "Feynman Lectures on Physics",
        std::vector<std::string>{"Feynman"}, "978-0465023820", "Basic Books", 2011, Money::of(800));

    Budget defaultDeptBudget(Money::of(10000));

    Budget csDeptBudget(Money::of(5000));
    csDeptBudget.setQuota(ResourceCategory::Book, {5, Money::of(3000)});

    Budget physDeptBudget(Money::of(8000));
    physDeptBudget.setQuota(ResourceCategory::Book, {10, Money::of(5000)});

    AcquisitionManager deptAcq(deptCatalog, defaultDeptBudget);
    deptAcq.setDepartmentBudget("Computer Science", csDeptBudget);
    deptAcq.setDepartmentBudget("Physics", physDeptBudget);

    std::cout << "Processing department batch requests...\n";
    deptAcq.processBatch({
        {"CS1", 3, "Computer Science"}, // 1800 OK
        {"PH1", 2, "Physics"},          // 1600 OK
        {"CS1", 4, "Computer Science"}, // 2400 Rejected (exceeds CS unit quota 5 max)
    });

    std::cout << "\n=== Department Acquisition Report ===\n";
    deptAcq.printReport(std::cout);
    // ─────────────────────────────────────────────────────────────────────────

    // ── Q10: Year-End Rollover demonstration ──────────────────────────────────
    std::cout << "\n=== Q10: Year-End Rollover Demo ===\n";

    Budget thisYearBudget(Money::of(10000));
    thisYearBudget.setQuota(ResourceCategory::Book, {10, Money::of(6000)});
    thisYearBudget.commit(ResourceCategory::Book, 4, Money::of(4000));

    std::cout << "This Year Budget:\n";
    thisYearBudget.print(std::cout);

    std::cout << "\nRolling over with 50% carry-forward into next year's 12000 base budget...\n";
    Budget nextYearBudget = thisYearBudget.rollover(50.0, Money::of(12000));

    std::cout << "Next Year Budget (Total = 12000 base + 3000 carry-forward = 15000):\n";
    nextYearBudget.print(std::cout);
    // ─────────────────────────────────────────────────────────────────────────

    // ── Q12: Vendor Selection demonstration ──────────────────────────────────
    std::cout << "\n=== Q12: Vendor Selection Demo ===\n";

    Catalog vendorCatalog;
    vendorCatalog.emplace<Book>("V001", "Introduction to Algorithms",
        std::vector<std::string>{"Cormen"}, "978-0262033848", "MIT Press", 2009, Money::of(800));

    Budget vendorBudget(Money::of(10000));
    AcquisitionManager vendorAcq(vendorCatalog, vendorBudget);

    std::cout << "Default quote for V001 (no vendors registered): " << vendorAcq.quote("V001", 1) << "\n";

    vendorAcq.addVendorPrice("V001", "VendorAlpha", Money::of(750));
    vendorAcq.addVendorPrice("V001", "VendorBeta", Money::of(650));
    vendorAcq.addVendorPrice("V001", "VendorGamma", Money::of(700));

    std::cout << "Quote for V001 after adding 3 vendors (650, 700, 750): "
              << vendorAcq.quote("V001", 1) << " (cheapest vendor selected)\n";

    const auto& vendorRec = vendorAcq.purchase("V001", 2);
    std::cout << "Purchased 2 copies. Order #" << vendorRec.orderNo
              << ", cost = " << vendorRec.cost
              << ", vendor = " << vendorRec.vendor << "\n";

    std::cout << "\n=== Vendor Acquisition Report ===\n";
    vendorAcq.printReport(std::cout);
    // ─────────────────────────────────────────────────────────────────────────

    return 0;
}
