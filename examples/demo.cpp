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
    return 0;
}
