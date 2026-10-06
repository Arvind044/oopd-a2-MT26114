// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}

static void testJournal() {
    Journal j("J1", "Science Monthly", "Springer", 2026,
              Money::of(500), "1234-5678", 12, 2);

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());

    // 500 per copy per year × 3 copies × 2 years
    CHECK(j.costFor(3) == Money::of(3000));

    CHECK_THROWS(j.costFor(0), std::invalid_argument);

    // Default subscription length should be 1 year
    Journal jDefault("J2", "Tech Journal", "IEEE", 2026,
                     Money::of(400), "1111-2222", 12);

    CHECK(jDefault.costFor(2) == Money::of(800));

    // Subscription length must be at least 1
    CHECK_THROWS(
        Journal("J3", "Invalid Journal", "Pub", 2026,
                Money::of(100), "3333-4444", 12, 0),
        std::invalid_argument);

    CHECK_THROWS(
        Journal("J4", "Invalid Journal", "Pub", 2026,
                Money::of(100), "5555-6666", 12, -1),
        std::invalid_argument);

    // Check Journal-specific printed details
    std::ostringstream os;
    j.print(os);

    CHECK(os.str().find("1234-5678") != std::string::npos);
    CHECK(os.str().find("12") != std::string::npos);
    CHECK(os.str().find("2") != std::string::npos);
}
static void testEBook() {
    EBook e("E1",
            "C++ Programming",
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

    CHECK(e.category() == ResourceCategory::EBook);
    CHECK(e.isDigital());

    CHECK(e.authors().size() == 1);
    CHECK(e.authors()[0] == "Bjarne Stroustrup");
    CHECK(e.isbn() == "978-0321563842");
    CHECK(e.format() == EBookFormat::PDF);
    CHECK(e.drm());

    // Pricing is inherited from ElectronicResource:
    // platform fee + price per seat × seats
    CHECK(e.costFor(5) == Money::of(550));

    // Verify parent + EBook-specific details.
    std::ostringstream os;
    e.print(os);

    CHECK(os.str().find("access url: https://example.com/cpp") != std::string::npos);
    CHECK(os.str().find("platform fee: 50.00") != std::string::npos);
    CHECK(os.str().find("978-0321563842") != std::string::npos);
    CHECK(os.str().find("PDF") != std::string::npos);
    CHECK(os.str().find("yes") != std::string::npos);
}
static void testAudioBook() {
    AudioBook a(
        "A1",
        "The C++ Programming Language - Audio",
        "Addison-Wesley",
        2013,
        Money::of(100),
        "https://example.com/audiobook",
        "John Smith",
        720,
        LicenseModel::AnnualSubscription,
        Money::of(50));

    CHECK(a.category() == ResourceCategory::AudioBook);
    CHECK(a.isDigital());

    CHECK(a.narrator() == "John Smith");
    CHECK(a.durationMinutes() == 720);

    CHECK(a.costFor(5) == Money::of(550));

    std::ostringstream os;
    a.print(os);

    CHECK(os.str().find("access url: https://example.com/audiobook") != std::string::npos);
    CHECK(os.str().find("platform fee: 50.00") != std::string::npos);
    CHECK(os.str().find("narrator: John Smith") != std::string::npos);
    CHECK(os.str().find("duration (minutes): 720") != std::string::npos);
}
static void testThesis() {
    Thesis t(
        "T1",
        "Machine Learning Research",
        "IIIT Delhi",
        2026,
        Money::of(0),
        "IIIT Delhi",
        "M.Tech",
        "Dr. Arani Bhattacharya");

    CHECK(t.category() == ResourceCategory::Thesis);
    CHECK(!t.isDigital());

    CHECK(t.university() == "IIIT Delhi");
    CHECK(t.degree() == "M.Tech");
    CHECK(t.supervisor() == "Dr. Arani Bhattacharya");

    CHECK(t.costFor(1) == Money::of(0));
    CHECK(t.costFor(10) == Money::of(0));

    std::ostringstream os;
    t.print(os);

    CHECK(os.str().find("University: IIIT Delhi") != std::string::npos);
    CHECK(os.str().find("Degree: M.Tech") != std::string::npos);
    CHECK(os.str().find("Supervisor: Dr. Arani Bhattacharya") != std::string::npos);
}
static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}

int main() {
    testMoney();
    testResourcesAndCost();
    testJournal();
    testEBook();
    testAudioBook();
    testThesis();
    testCatalog();
    testBudget();
    testAcquisition();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
