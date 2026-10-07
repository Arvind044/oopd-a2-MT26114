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
    Book hardcover("B2", "Hardcover Test", {"Author"}, "isbn2", "P", 2020,
               Money::of(100), 1, Binding::Hardcover);

    CHECK(hardcover.costFor(3) == Money::of(360));
    Book bulkBook("B3", "Bulk Test", {"Author"}, "isbn3", "P", 2020,
              Money::of(100));

    CHECK(bulkBook.costFor(9) == Money::of(900));
    CHECK(bulkBook.costFor(10) == Money::of(900));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));
    CHECK(e.costFor(50) == Money::of(600));
    CHECK(e.costFor(51) == Money::of(605));
    CHECK(e.costFor(60) == Money::of(650));

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
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource));

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
void testTaxes() {
    Catalog c;

    c.add(std::make_unique<Book>(
        "TB1", "Tax Book", std::vector<std::string>{"Author"},
        "isbn-tax", "Publisher", 2025, Money::of(100)));

    c.add(std::make_unique<ElectronicResource>(
        "TR1", "Tax Electronic", "Publisher", 2025,
        Money::of(100), "https://example.com",
        LicenseModel::AnnualSubscription, Money::of(0)));

    Budget b(Money::of(10000));
    AcquisitionManager acq(c, b);

    acq.setTaxRates(10, 20);

    CHECK(acq.purchase("TB1", 2).preTaxCost == Money::of(200));
    CHECK(acq.history().back().cost == Money::of(220));

    CHECK(acq.purchase("TR1", 2).preTaxCost == Money::of(200));
    CHECK(acq.history().back().cost == Money::of(240));
        Catalog quotaCatalog;

    quotaCatalog.add(std::make_unique<Book>(
        "QB1", "Quota Tax Book", std::vector<std::string>{"Author"},
        "isbn-quota", "Publisher", 2025, Money::of(100)));

    Budget quotaBudget(Money::of(1000));
    quotaBudget.setQuota(
        ResourceCategory::Book,
        Quota{2, Money::of(210)}
    );

    AcquisitionManager quotaAcq(quotaCatalog, quotaBudget);
    quotaAcq.setTaxRates(10, 20);

    std::string reason;
    CHECK(!quotaAcq.canPurchase("QB1", 2, &reason));
    CHECK(!reason.empty());
    CHECK(quotaBudget.spent() == Money::of(0));
    CHECK(quotaCatalog.holdings("QB1") == 0);
        std::ostringstream report;
    acq.printReport(report);

    CHECK(report.str().find("pre-tax 200.00") != std::string::npos);
    CHECK(report.str().find("post-tax 220.00") != std::string::npos);
    CHECK(report.str().find("pre-tax 200.00") != std::string::npos);
    CHECK(report.str().find("post-tax 240.00") != std::string::npos);
    CHECK(report.str().find("Total pre-tax: 400.00") != std::string::npos);
    CHECK(report.str().find("Total post-tax: 460.00") != std::string::npos);
}

// ── Q7 tests ─────────────────────────────────────────────────────────────────
static void testTitleQuota() {
    // --- Budget-level title checks ---
    Budget b(Money::of(5000));
    // maxTitles = 2 for Books
    b.setQuota(ResourceCategory::Book, {10, Money::of(4000), 2});

    // First title: allowed
    CHECK(b.checkTitle(ResourceCategory::Book, "Alpha").empty());
    b.addTitle(ResourceCategory::Book, "Alpha");

    // Same title again: still allowed (not a new slot)
    CHECK(b.checkTitle(ResourceCategory::Book, "Alpha").empty());

    // Second distinct title: allowed
    CHECK(b.checkTitle(ResourceCategory::Book, "Beta").empty());
    b.addTitle(ResourceCategory::Book, "Beta");

    // Third distinct title: must be rejected
    CHECK(!b.checkTitle(ResourceCategory::Book, "Gamma").empty());

    // maxTitles = 0 means unlimited
    Budget b2(Money::of(5000));
    b2.setQuota(ResourceCategory::Book, {10, Money::of(4000), 0});
    b2.addTitle(ResourceCategory::Book, "X");
    b2.addTitle(ResourceCategory::Book, "Y");
    b2.addTitle(ResourceCategory::Book, "Z");
    CHECK(b2.checkTitle(ResourceCategory::Book, "W").empty());

    // No quota set → unlimited (checkTitle returns "")
    Budget b3(Money::of(5000));
    CHECK(b3.checkTitle(ResourceCategory::Book, "AnyTitle").empty());

    // --- AcquisitionManager-level title checks ---
    Catalog cat;
    cat.emplace<Book>("TQ1", "TitleA", std::vector<std::string>{"Author"},
                      "isbn1", "Pub", 2024, Money::of(50));
    cat.emplace<Book>("TQ2", "TitleB", std::vector<std::string>{"Author"},
                      "isbn2", "Pub", 2024, Money::of(50));
    cat.emplace<Book>("TQ3", "TitleC", std::vector<std::string>{"Author"},
                      "isbn3", "Pub", 2024, Money::of(50));

    Budget bAcq(Money::of(10000));
    bAcq.setQuota(ResourceCategory::Book, {20, Money::of(9000), 2}); // max 2 titles
    AcquisitionManager acq(cat, bAcq);

    // First purchase of TitleA — OK
    CHECK(acq.purchase("TQ1", 1).approved);

    // Second purchase of same title (TitleA) — OK (existing title)
    CHECK(acq.purchase("TQ1", 1).approved);

    // Purchase of TitleB — OK (2nd distinct title)
    CHECK(acq.purchase("TQ2", 1).approved);

    // canPurchase TitleC — must be rejected
    std::string reason;
    CHECK(!acq.canPurchase("TQ3", 1, &reason));
    CHECK(reason.find("title quota") != std::string::npos);

    // purchase TitleC — must throw QuotaExceededError, state unchanged
    const Money spentBefore = bAcq.spent();
    const int holdingsBefore = cat.holdings("TQ3");
    CHECK_THROWS(acq.purchase("TQ3", 1), QuotaExceededError);
    CHECK(bAcq.spent() == spentBefore);
    CHECK(cat.holdings("TQ3") == holdingsBefore);

    // processBatch: TitleC must be rejected in batch too
    auto results = acq.processBatch({{"TQ3", 1}});
    CHECK(results.size() == 1);
    CHECK(!results[0].approved);
    CHECK(results[0].reason.find("title quota") != std::string::npos);

    // setTitleQuota convenience wrapper
    Budget b4(Money::of(5000));
    b4.setQuota(ResourceCategory::Book, {10, Money::of(4000)});
    b4.setTitleQuota(ResourceCategory::Book, 1);
    b4.addTitle(ResourceCategory::Book, "OnlyOne");
    CHECK(!b4.checkTitle(ResourceCategory::Book, "Another").empty());
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q8 tests ─────────────────────────────────────────────────────────────────
static void testCancellation() {
    Catalog cat;
    cat.emplace<Book>("CB1", "Cancel Book", std::vector<std::string>{"Author"},
                      "isbn-c", "Pub", 2024, Money::of(100));

    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(500)});

    AcquisitionManager acq(cat, b);

    // Purchase order #1: 3 copies for Money::of(300)
    const auto& rec1 = acq.purchase("CB1", 3);
    CHECK(rec1.orderNo == 1);
    CHECK(rec1.approved);
    CHECK(!rec1.cancelled);
    CHECK(cat.holdings("CB1") == 3);
    CHECK(b.spent() == Money::of(300));
    CHECK(acq.totalSpent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 2);

    // Cancel order #1: should succeed
    CHECK(acq.cancel(1));

    // Verify original record is updated to cancelled=true
    CHECK(acq.history()[0].cancelled);

    // Verify holdings, budget spent, and totalSpent are reverted
    CHECK(cat.holdings("CB1") == 0);
    CHECK(b.spent() == Money::of(0));
    CHECK(acq.totalSpent() == Money::of(0));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 5);

    // Verify history now has 2 records: original cancelled + separate cancellation record
    CHECK(acq.history().size() == 2);
    CHECK(acq.history()[1].orderNo == 2);
    CHECK(!acq.history()[1].approved);
    CHECK(acq.history()[1].reason.find("Cancelled order #1") != std::string::npos);

    // Attempting to cancel order #1 again must fail (already cancelled)
    CHECK(!acq.cancel(1));

    // Attempting to cancel non-existent order #999 must fail
    CHECK(!acq.cancel(999));

    // Attempting to cancel a rejected order must fail
    auto batchRes = acq.processBatch({{"CB1", 10}}); // exceeds unit quota (5 max)
    CHECK(batchRes.size() == 1);
    CHECK(!batchRes[0].approved);
    int rejectedOrderNo = batchRes[0].orderNo;
    CHECK(!acq.cancel(rejectedOrderNo));
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q9 tests ─────────────────────────────────────────────────────────────────
static void testDepartmentBudgets() {
    Catalog cat;
    cat.emplace<Book>("DB1", "CS Book", std::vector<std::string>{"Author"},
                      "isbn-cs", "Pub", 2024, Money::of(100));
    cat.emplace<Book>("DB2", "Physics Book", std::vector<std::string>{"Author"},
                      "isbn-ph", "Pub", 2024, Money::of(150));

    Budget defaultBudget(Money::of(1000));
    Budget csBudget(Money::of(500));
    csBudget.setQuota(ResourceCategory::Book, {3, Money::of(300)});

    Budget physicsBudget(Money::of(1000));
    physicsBudget.setQuota(ResourceCategory::Book, {5, Money::of(800)});

    AcquisitionManager acq(cat, defaultBudget);
    acq.setDepartmentBudget("Computer Science", csBudget);
    acq.setDepartmentBudget("Physics", physicsBudget);

    // CS department purchases 2 copies of DB1 (cost 200) -> OK
    const auto recCS = acq.purchase("DB1", 2, "Computer Science");
    int csOrderNo = recCS.orderNo;
    CHECK(recCS.approved);
    CHECK(recCS.department == "Computer Science");

    // Physics department purchases 3 copies of DB2 (cost 450) -> OK
    const auto recPh = acq.purchase("DB2", 3, "Physics");
    CHECK(recPh.approved);
    CHECK(recPh.department == "Physics");

    // Attempting 2 more copies for CS (total 4 copies > 3 max quota) -> fails
    std::string reason;
    CHECK(!acq.canPurchase("DB1", 2, &reason, "Computer Science"));
    CHECK(reason.find("unit quota exceeded") != std::string::npos);

    // Process batch with mixed department requests
    auto batchRes = acq.processBatch({
        {"DB1", 1, "Computer Science"}, // 1 copy OK (3/3 used)
        {"DB1", 1, "Computer Science"}, // 1 copy rejected (unit quota)
        {"DB2", 1, "Physics"}           // 1 copy OK (4/5 used)
    });

    CHECK(batchRes.size() == 3);
    CHECK(batchRes[0].approved);
    CHECK(batchRes[0].department == "Computer Science");
    CHECK(!batchRes[1].approved);
    CHECK(batchRes[1].department == "Computer Science");
    CHECK(batchRes[2].approved);
    CHECK(batchRes[2].department == "Physics");

    // Cancellation of CS order refunds CS budget
    CHECK(acq.cancel(csOrderNo));
    // CS can now purchase again
    CHECK(acq.canPurchase("DB1", 1, nullptr, "Computer Science"));
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q10 tests ────────────────────────────────────────────────────────────────
static void testRollover() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(800)});

    // Spend 400 out of 1000 -> 600 remaining
    b.commit(ResourceCategory::Book, 2, Money::of(400));
    CHECK(b.spent() == Money::of(400));
    CHECK(b.remaining() == Money::of(600));

    // Rollover 50% of remaining 600 (300) into a new base budget of 1000
    // Resulting next year total = 1000 + 300 = 1300
    Budget nextYear = b.rollover(50.0, Money::of(1000));
    CHECK(nextYear.total() == Money::of(1300));
    CHECK(nextYear.spent() == Money::of(0));
    CHECK(nextYear.remaining() == Money::of(1300));

    // Quotas are preserved in next year's budget
    auto q = nextYear.quotaFor(ResourceCategory::Book);
    CHECK(static_cast<bool>(q));
    CHECK(q->maxUnits == 10);
    CHECK(q->maxSpend == Money::of(800));

    // Next year's usage starts fresh at 0
    CHECK(nextYear.usageFor(ResourceCategory::Book).units == 0);
    CHECK(nextYear.usageFor(ResourceCategory::Book).spent == Money::of(0));

    // Original budget is unmodified
    CHECK(b.total() == Money::of(1000));
    CHECK(b.spent() == Money::of(400));

    // Invalid rollover percentages throw invalid_argument
    CHECK_THROWS(b.rollover(-5.0), std::invalid_argument);
    CHECK_THROWS(b.rollover(105.0), std::invalid_argument);
    CHECK_THROWS(b.rollover(50.0, Money::of(-100)), std::invalid_argument);

    // Test department budget rollover via AcquisitionManager
    Catalog cat;
    cat.emplace<Book>("RB1", "Rollover Book", std::vector<std::string>{"Author"},
                      "isbn-r", "Pub", 2024, Money::of(100));

    Budget defaultB(Money::of(5000));
    Budget csB(Money::of(2000));
    csB.setQuota(ResourceCategory::Book, {10, Money::of(1500)});

    AcquisitionManager acq(cat, defaultB);
    acq.setDepartmentBudget("Computer Science", csB);

    // CS spends 500 out of 2000 -> 1500 remaining
    acq.purchase("RB1", 5, "Computer Science");

    // Rollover CS budget with 20% carry-forward (20% of 1500 = 300) into 3000 base
    Budget nextCS = acq.rolloverBudget("Computer Science", 20.0, Money::of(3000));
    CHECK(nextCS.total() == Money::of(3300));
    CHECK(nextCS.spent() == Money::of(0));
}
// ─────────────────────────────────────────────────────────────────────────────

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
    testTaxes();
    testTitleQuota();
    testCancellation();
    testDepartmentBudgets();
    testRollover();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
