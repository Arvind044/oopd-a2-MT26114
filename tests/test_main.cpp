// Minimal self-contained test runner using standard <cassert>.

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

template <typename Exception, typename Func>
void assertThrows(Func&& fn) {
    bool thrown = false;
    try {
        fn();
    } catch (const Exception&) {
        thrown = true;
    } catch (...) {
    }
    assert(thrown);
}

static void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assertThrows<std::invalid_argument>([&] { Money::of(1, 100); });
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert(b.category() == ResourceCategory::Book);
    assert(!b.isDigital());
    assert(b.costFor(3) == Money::of(300));
    Book hardcover("B2", "Hardcover Test", {"Author"}, "isbn2", "P", 2020,
               Money::of(100), 1, Binding::Hardcover);

    assert(hardcover.costFor(3) == Money::of(360));
    Book bulkBook("B3", "Bulk Test", {"Author"}, "isbn3", "P", 2020,
              Money::of(100));

    assert(bulkBook.costFor(9) == Money::of(900));
    assert(bulkBook.costFor(10) == Money::of(900));
    assertThrows<std::invalid_argument>([&] { b.costFor(0); });
    assert(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert(e.isDigital());
    assert(e.costFor(5) == Money::of(150));
    assert(e.costFor(50) == Money::of(600));
    assert(e.costFor(51) == Money::of(605));
    assert(e.costFor(60) == Money::of(650));

    assert(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    assert(os.str().find("platform fee: 100.00") != std::string::npos);

    assertThrows<std::invalid_argument>([&] { Book("", "T", {}, "", "", 2000, Money::of(1)); });
    assertThrows<std::invalid_argument>([&] {
        Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1));
    });
}

static void testJournal() {
    Journal j("J1", "Science Monthly", "Springer", 2026,
              Money::of(500), "1234-5678", 12, 2);

    assert(j.category() == ResourceCategory::Journal);
    assert(!j.isDigital());

    // 500 per copy per year × 3 copies × 2 years
    assert(j.costFor(3) == Money::of(3000));

    assertThrows<std::invalid_argument>([&] { j.costFor(0); });

    // Default subscription length should be 1 year
    Journal jDefault("J2", "Tech Journal", "IEEE", 2026,
                     Money::of(400), "1111-2222", 12);

    assert(jDefault.costFor(2) == Money::of(800));

    // Subscription length must be at least 1
    assertThrows<std::invalid_argument>([&] {
        Journal("J3", "Invalid Journal", "Pub", 2026,
                Money::of(100), "3333-4444", 12, 0);
    });

    assertThrows<std::invalid_argument>([&] {
        Journal("J4", "Invalid Journal", "Pub", 2026,
                Money::of(100), "5555-6666", 12, -1);
    });

    // Check Journal-specific printed details
    std::ostringstream os;
    j.print(os);

    assert(os.str().find("1234-5678") != std::string::npos);
    assert(os.str().find("12") != std::string::npos);
    assert(os.str().find("2") != std::string::npos);
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

    assert(e.category() == ResourceCategory::EBook);
    assert(e.isDigital());

    assert(e.authors().size() == 1);
    assert(e.authors()[0] == "Bjarne Stroustrup");
    assert(e.isbn() == "978-0321563842");
    assert(e.format() == EBookFormat::PDF);
    assert(e.drm());

    // Pricing is inherited from ElectronicResource:
    // platform fee + price per seat × seats
    assert(e.costFor(5) == Money::of(550));

    // Verify parent + EBook-specific details.
    std::ostringstream os;
    e.print(os);

    assert(os.str().find("access url: https://example.com/cpp") != std::string::npos);
    assert(os.str().find("platform fee: 50.00") != std::string::npos);
    assert(os.str().find("978-0321563842") != std::string::npos);
    assert(os.str().find("PDF") != std::string::npos);
    assert(os.str().find("yes") != std::string::npos);
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

    assert(a.category() == ResourceCategory::AudioBook);
    assert(a.isDigital());

    assert(a.narrator() == "John Smith");
    assert(a.durationMinutes() == 720);

    assert(a.costFor(5) == Money::of(550));

    std::ostringstream os;
    a.print(os);

    assert(os.str().find("access url: https://example.com/audiobook") != std::string::npos);
    assert(os.str().find("platform fee: 50.00") != std::string::npos);
    assert(os.str().find("narrator: John Smith") != std::string::npos);
    assert(os.str().find("duration (minutes): 720") != std::string::npos);
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

    assert(t.category() == ResourceCategory::Thesis);
    assert(!t.isDigital());

    assert(t.university() == "IIIT Delhi");
    assert(t.degree() == "M.Tech");
    assert(t.supervisor() == "Dr. Arani Bhattacharya");

    assert(t.costFor(1) == Money::of(0));
    assert(t.costFor(10) == Money::of(0));

    std::ostringstream os;
    t.print(os);

    assert(os.str().find("University: IIIT Delhi") != std::string::npos);
    assert(os.str().find("Degree: M.Tech") != std::string::npos);
    assert(os.str().find("Supervisor: Dr. Arani Bhattacharya") != std::string::npos);
}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    assert(c.size() == 3);
    assert(c.contains("B1"));
    assert(c.find("nope") == nullptr);
    assertThrows<NotFoundError>([&] { c.get("nope"); });
    assertThrows<DuplicateIdError>([&] {
        c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1, Money::of(1));
    });

    assert(c.searchTitle("clean").size() == 2);
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    assert(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    assert(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    assert(c.holdings("B1") == 3);
    assertThrows<std::invalid_argument>([&] { c.addHoldings("B1", -5); });

    c.remove("R1");
    assert(c.size() == 2);
    assertThrows<NotFoundError>([&] { c.remove("R1"); });
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    assert(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    assert(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    assert(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    assert(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    assert(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert(b.spent() == Money::of(300));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    assert(!b.unitsRemaining(ResourceCategory::ElectronicResource));

    assertThrows<QuotaExceededError>([&] {
        b.commit(ResourceCategory::Book, 2, Money::of(10));
    });
    assertThrows<BudgetExceededError>([&] {
        b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800));
    });
    assertThrows<std::invalid_argument>([&] {
        b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1));
    });
    assert(b.spent() == Money::of(300));  // failed commits changed nothing
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

    assert(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    assert(acq.canPurchase("B1", 3, &why) && why.empty());
    assert(!acq.canPurchase("B1", 4, &why) && !why.empty());
    assert(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    assert(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    assert(c.holdings("B1") == 2);

    assertThrows<QuotaExceededError>([&] { acq.purchase("B1", 2); });
    assertThrows<NotFoundError>([&] { acq.purchase("nope", 1); });
    assert(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert(res.size() == 5);
    assert(res[0].approved && res[0].cost == Money::of(150));
    assert(!res[1].approved);  // 1050 > remaining 150
    assert(res[2].approved);
    assert(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    assert(!res[4].approved);
    assert(acq.totalSpent() == Money::of(450));
    assert(b.spent() == acq.totalSpent());
    assert(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    assert(acq.history().size() == 6);
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

    assert(acq.purchase("TB1", 2).preTaxCost == Money::of(200));
    assert(acq.history().back().cost == Money::of(220));

    assert(acq.purchase("TR1", 2).preTaxCost == Money::of(200));
    assert(acq.history().back().cost == Money::of(240));
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
    assert(!quotaAcq.canPurchase("QB1", 2, &reason));
    assert(!reason.empty());
    assert(quotaBudget.spent() == Money::of(0));
    assert(quotaCatalog.holdings("QB1") == 0);
    std::ostringstream report;
    acq.printReport(report);

    assert(report.str().find("pre-tax 200.00") != std::string::npos);
    assert(report.str().find("post-tax 220.00") != std::string::npos);
    assert(report.str().find("pre-tax 200.00") != std::string::npos);
    assert(report.str().find("post-tax 240.00") != std::string::npos);
    assert(report.str().find("Total pre-tax: 400.00") != std::string::npos);
    assert(report.str().find("Total post-tax: 460.00") != std::string::npos);
}

// ── Q7 tests ─────────────────────────────────────────────────────────────────
static void testTitleQuota() {
    // --- Budget-level title checks ---
    Budget b(Money::of(5000));
    // maxTitles = 2 for Books
    b.setQuota(ResourceCategory::Book, {10, Money::of(4000), 2});

    // First title: allowed
    assert(b.checkTitle(ResourceCategory::Book, "Alpha").empty());
    b.addTitle(ResourceCategory::Book, "Alpha");

    // Same title again: still allowed (not a new slot)
    assert(b.checkTitle(ResourceCategory::Book, "Alpha").empty());

    // Second distinct title: allowed
    assert(b.checkTitle(ResourceCategory::Book, "Beta").empty());
    b.addTitle(ResourceCategory::Book, "Beta");

    // Third distinct title: must be rejected
    assert(!b.checkTitle(ResourceCategory::Book, "Gamma").empty());

    // maxTitles = 0 means unlimited
    Budget b2(Money::of(5000));
    b2.setQuota(ResourceCategory::Book, {10, Money::of(4000), 0});
    b2.addTitle(ResourceCategory::Book, "X");
    b2.addTitle(ResourceCategory::Book, "Y");
    b2.addTitle(ResourceCategory::Book, "Z");
    assert(b2.checkTitle(ResourceCategory::Book, "W").empty());

    // No quota set → unlimited (checkTitle returns "")
    Budget b3(Money::of(5000));
    assert(b3.checkTitle(ResourceCategory::Book, "AnyTitle").empty());

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
    assert(acq.purchase("TQ1", 1).approved);

    // Second purchase of same title (TitleA) — OK (existing title)
    assert(acq.purchase("TQ1", 1).approved);

    // Purchase of TitleB — OK (2nd distinct title)
    assert(acq.purchase("TQ2", 1).approved);

    // canPurchase TitleC — must be rejected
    std::string reason;
    assert(!acq.canPurchase("TQ3", 1, &reason));
    assert(reason.find("title quota") != std::string::npos);

    // purchase TitleC — must throw QuotaExceededError, state unchanged
    const Money spentBefore = bAcq.spent();
    const int holdingsBefore = cat.holdings("TQ3");
    assertThrows<QuotaExceededError>([&] { acq.purchase("TQ3", 1); });
    assert(bAcq.spent() == spentBefore);
    assert(cat.holdings("TQ3") == holdingsBefore);

    // processBatch: TitleC must be rejected in batch too
    auto results = acq.processBatch({{"TQ3", 1}});
    assert(results.size() == 1);
    assert(!results[0].approved);
    assert(results[0].reason.find("title quota") != std::string::npos);

    // setTitleQuota convenience wrapper
    Budget b4(Money::of(5000));
    b4.setQuota(ResourceCategory::Book, {10, Money::of(4000)});
    b4.setTitleQuota(ResourceCategory::Book, 1);
    b4.addTitle(ResourceCategory::Book, "OnlyOne");
    assert(!b4.checkTitle(ResourceCategory::Book, "Another").empty());
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
    assert(rec1.orderNo == 1);
    assert(rec1.approved);
    assert(!rec1.cancelled);
    assert(cat.holdings("CB1") == 3);
    assert(b.spent() == Money::of(300));
    assert(acq.totalSpent() == Money::of(300));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 2);

    // Cancel order #1: should succeed
    assert(acq.cancel(1));

    // Verify original record is updated to cancelled=true
    assert(acq.history()[0].cancelled);

    // Verify holdings, budget spent, and totalSpent are reverted
    assert(cat.holdings("CB1") == 0);
    assert(b.spent() == Money::of(0));
    assert(acq.totalSpent() == Money::of(0));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 5);

    // Verify history now has 2 records: original cancelled + separate cancellation record
    assert(acq.history().size() == 2);
    assert(acq.history()[1].orderNo == 2);
    assert(!acq.history()[1].approved);
    assert(acq.history()[1].reason.find("Cancelled order #1") != std::string::npos);

    // Attempting to cancel order #1 again must fail (already cancelled)
    assert(!acq.cancel(1));

    // Attempting to cancel non-existent order #999 must fail
    assert(!acq.cancel(999));

    // Attempting to cancel a rejected order must fail
    auto batchRes = acq.processBatch({{"CB1", 10}}); // exceeds unit quota (5 max)
    assert(batchRes.size() == 1);
    assert(!batchRes[0].approved);
    int rejectedOrderNo = batchRes[0].orderNo;
    assert(!acq.cancel(rejectedOrderNo));
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
    assert(recCS.approved);
    assert(recCS.department == "Computer Science");

    // Physics department purchases 3 copies of DB2 (cost 450) -> OK
    const auto recPh = acq.purchase("DB2", 3, "Physics");
    assert(recPh.approved);
    assert(recPh.department == "Physics");

    // Attempting 2 more copies for CS (total 4 copies > 3 max quota) -> fails
    std::string reason;
    assert(!acq.canPurchase("DB1", 2, &reason, "Computer Science"));
    assert(reason.find("unit quota exceeded") != std::string::npos);

    // Process batch with mixed department requests
    auto batchRes = acq.processBatch({
        {"DB1", 1, "Computer Science"}, // 1 copy OK (3/3 used)
        {"DB1", 1, "Computer Science"}, // 1 copy rejected (unit quota)
        {"DB2", 1, "Physics"}           // 1 copy OK (4/5 used)
    });

    assert(batchRes.size() == 3);
    assert(batchRes[0].approved);
    assert(batchRes[0].department == "Computer Science");
    assert(!batchRes[1].approved);
    assert(batchRes[1].department == "Computer Science");
    assert(batchRes[2].approved);
    assert(batchRes[2].department == "Physics");

    // Cancellation of CS order refunds CS budget
    assert(acq.cancel(csOrderNo));
    // CS can now purchase again
    assert(acq.canPurchase("DB1", 1, nullptr, "Computer Science"));
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q10 tests ────────────────────────────────────────────────────────────────
static void testRollover() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(800)});

    // Spend 400 out of 1000 -> 600 remaining
    b.commit(ResourceCategory::Book, 2, Money::of(400));
    assert(b.spent() == Money::of(400));
    assert(b.remaining() == Money::of(600));

    // Rollover 50% of remaining 600 (300) into a new base budget of 1000
    // Resulting next year total = 1000 + 300 = 1300
    Budget nextYear = b.rollover(50.0, Money::of(1000));
    assert(nextYear.total() == Money::of(1300));
    assert(nextYear.spent() == Money::of(0));
    assert(nextYear.remaining() == Money::of(1300));

    // Quotas are preserved in next year's budget
    auto q = nextYear.quotaFor(ResourceCategory::Book);
    assert(static_cast<bool>(q));
    assert(q->maxUnits == 10);
    assert(q->maxSpend == Money::of(800));

    // Next year's usage starts fresh at 0
    assert(nextYear.usageFor(ResourceCategory::Book).units == 0);
    assert(nextYear.usageFor(ResourceCategory::Book).spent == Money::of(0));

    // Original budget is unmodified
    assert(b.total() == Money::of(1000));
    assert(b.spent() == Money::of(400));

    // Invalid rollover percentages throw invalid_argument
    assertThrows<std::invalid_argument>([&] { b.rollover(-5.0); });
    assertThrows<std::invalid_argument>([&] { b.rollover(105.0); });
    assertThrows<std::invalid_argument>([&] { b.rollover(50.0, Money::of(-100)); });

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
    assert(nextCS.total() == Money::of(3300));
    assert(nextCS.spent() == Money::of(0));
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q11 tests ────────────────────────────────────────────────────────────────
static void testAllOrNothingBatch() {
    Catalog cat;
    cat.emplace<Book>("AON1", "Book 1", std::vector<std::string>{"Author"},
                      "isbn-1", "Pub", 2024, Money::of(100));
    cat.emplace<Book>("AON2", "Book 2", std::vector<std::string>{"Author"},
                      "isbn-2", "Pub", 2024, Money::of(150));
    cat.emplace<Book>("AON3", "Book 3", std::vector<std::string>{"Author"},
                      "isbn-3", "Pub", 2024, Money::of(200));

    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(600)});

    AcquisitionManager acq(cat, b);

    // 1. Successful all-or-nothing batch
    auto resSuccess = acq.processBatch({{"AON1", 2}, {"AON2", 1}}, true);
    assert(resSuccess.size() == 2);
    assert(resSuccess[0].approved);
    assert(resSuccess[1].approved);
    assert(b.spent() == Money::of(350));
    assert(cat.holdings("AON1") == 2);
    assert(cat.holdings("AON2") == 1);
    assert(acq.history().size() == 2);

    // 2. Failure on the first request (unknown resource)
    const size_t histBefore1 = acq.history().size();
    const Money spentBefore1 = b.spent();
    auto resFailFirst = acq.processBatch({{"INVALID", 1}, {"AON1", 1}}, true);
    assert(resFailFirst.size() == 2);
    assert(!resFailFirst[0].approved);
    assert(b.spent() == spentBefore1);
    assert(acq.history().size() == histBefore1); // state & history unchanged

    // 3. Failure on a later request (unit quota exceeded)
    const size_t histBefore2 = acq.history().size();
    const Money spentBefore2 = b.spent();
    // AON1 already has 2 holdings. Unit quota is 5 max.
    // Batch: AON1 x2 (ok, 4 total) then AON1 x2 (exceeds 5 max -> fails)
    auto resFailLater = acq.processBatch({{"AON1", 2}, {"AON1", 2}}, true);
    assert(resFailLater.size() == 2);
    assert(resFailLater[0].approved);  // simulation approved 1st
    assert(!resFailLater[1].approved); // simulation rejected 2nd
    assert(b.spent() == spentBefore2);
    assert(cat.holdings("AON1") == 2);  // holdings unchanged
    assert(acq.history().size() == histBefore2); // history unchanged

    // 4. Cumulative budget failure
    // Current spent = 350, max spend quota = 600. Remaining spend = 250.
    // Request 1: AON1 x1 (100) -> sim spent 450 (ok)
    // Request 2: AON3 x1 (200) -> sim spent 650 > 600 (fails!)
    auto resFailCumulative = acq.processBatch({{"AON1", 1}, {"AON3", 1}}, true);
    assert(resFailCumulative.size() == 2);
    assert(resFailCumulative[0].approved);
    assert(!resFailCumulative[1].approved);
    assert(b.spent() == Money::of(350)); // state unchanged

    // 5. Existing non-all-or-nothing behavior still works (default allOrNothing = false)
    auto resStandard = acq.processBatch({{"AON1", 1}, {"AON3", 1}});
    assert(resStandard.size() == 2);
    assert(resStandard[0].approved);   // 100 spent -> OK (total spent 450)
    assert(!resStandard[1].approved);  // 200 spent -> rejected (450+200=650 > 600)
    assert(b.spent() == Money::of(450)); // first request committed, second failed
    assert(acq.history().size() == histBefore2 + 2); // history updated for non-all-or-nothing
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q12 tests ────────────────────────────────────────────────────────────────
static void testVendorPricing() {
    Catalog cat;
    cat.emplace<Book>("V1", "Vendor Book 1", std::vector<std::string>{"Author"},
                      "isbn-v1", "Pub", 2024, Money::of(100)); // Default price 100
    cat.emplace<Book>("V2", "Vendor Book 2", std::vector<std::string>{"Author"},
                      "isbn-v2", "Pub", 2024, Money::of(200)); // Default price 200

    Budget b(Money::of(2000));
    AcquisitionManager acq(cat, b);

    // 1. No vendors registered -> preserves old behavior
    const auto& recNoVendor = acq.purchase("V1", 1);
    assert(recNoVendor.cost == Money::of(100));
    assert(recNoVendor.vendor.empty());

    // 2. One vendor registered
    acq.addVendorPrice("V1", "BookDepot", Money::of(80));
    assert(acq.quote("V1", 1) == Money::of(80));
    const auto& recOneVendor = acq.purchase("V1", 1);
    assert(recOneVendor.cost == Money::of(80));
    assert(recOneVendor.vendor == "BookDepot");

    // 3. Multiple vendors registered (cheapest selected, expensive ignored)
    acq.addVendorPrice("V2", "ExpensiveVendor", Money::of(250));
    acq.addVendorPrice("V2", "CheapestVendor", Money::of(120));
    acq.addVendorPrice("V2", "MidVendor", Money::of(180));

    assert(acq.quote("V2", 1) == Money::of(120));
    const auto& recMulti = acq.purchase("V2", 2); // 2 x 120 = 240
    assert(recMulti.cost == Money::of(240));
    assert(recMulti.vendor == "CheapestVendor");

    // 4. Equal-price deterministic tie-breaking (lexicographical order)
    cat.emplace<Book>("V3", "Equal Price Book", std::vector<std::string>{"Author"},
                      "isbn-v3", "Pub", 2024, Money::of(500));
    acq.addVendorPrice("V3", "ZebraBooks", Money::of(300));
    acq.addVendorPrice("V3", "AlphaBooks", Money::of(300));

    const auto& recTie = acq.purchase("V3", 1);
    assert(recTie.cost == Money::of(300));
    assert(recTie.vendor == "AlphaBooks"); // "AlphaBooks" < "ZebraBooks"

    // 5. Q11 all-or-nothing batch uses vendor pricing correctly
    Catalog catAON;
    catAON.emplace<Book>("VA1", "AON Vendor Book", std::vector<std::string>{"Author"},
                         "isbn-va1", "Pub", 2024, Money::of(500));
    Budget bAON(Money::of(300)); // Budget = 300
    AcquisitionManager acqAON(catAON, bAON);

    // Without vendor price, cost is 500 > 300 (fails)
    std::string reason;
    assert(!acqAON.canPurchase("VA1", 1, &reason));

    // Register vendor offering price of 250 (fits within budget of 300)
    acqAON.addVendorPrice("VA1", "DiscountVendor", Money::of(250));
    assert(acqAON.canPurchase("VA1", 1, &reason));

    // All-or-nothing batch succeeds using vendor price
    auto aonRes = acqAON.processBatch({{"VA1", 1}}, true);
    assert(aonRes.size() == 1);
    assert(aonRes[0].approved);
    assert(aonRes[0].cost == Money::of(250));
    assert(aonRes[0].vendor == "DiscountVendor");
    assert(bAON.spent() == Money::of(250));

    // Error handling on invalid addVendorPrice calls
    assertThrows<std::invalid_argument>([&] {
        acq.addVendorPrice("V1", "", Money::of(10));
    });
    assertThrows<std::invalid_argument>([&] {
        acq.addVendorPrice("V1", "Vendor", Money::of(-10));
    });
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q13 tests ────────────────────────────────────────────────────────────────
static void testCatalogSearchesAndPersistence() {
    Catalog cat;
    cat.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                      "978-0132350884", "Prentice Hall", 2008, Money::of(450), 1, Binding::Paperback);
    cat.emplace<Book>("B2", "Design Patterns", std::vector<std::string>{"Erich Gamma", "Richard Helm", "Ralph Johnson", "John Vlissides"},
                      "978-0201633610", "Addison-Wesley", 1994, Money::of(550), 1, Binding::Hardcover);
    cat.emplace<EBook>("E1", "The C++ Programming Language", std::vector<std::string>{"Bjarne Stroustrup"},
                       "978-0321563842", "Addison-Wesley", 2013, Money::of(100),
                       "https://example.com/cpp", LicenseModel::AnnualSubscription, Money::of(50),
                       EBookFormat::PDF, true);
    cat.emplace<AudioBook>("A1", "Clean Architecture Audio", "Addison-Wesley", 2017,
                           Money::of(120), "https://example.com/audio", "Uncle Bob", 600,
                           LicenseModel::Perpetual, Money::of(0));
    cat.emplace<Journal>("J1", "Nature Communications", "Nature Publishing", 2020,
                         Money::of(500), "2041-1723", 12, 2);
    cat.emplace<Thesis>("T1", "Quantum Computing Foundations", "IIIT Delhi", 2022,
                        Money::of(0), "IIIT Delhi", "Ph.D", "Dr. Bhattacharya");
    cat.emplace<ElectronicResource>("ER1", "ACM Digital Library", "ACM", 2019,
                                    Money::of(200), "https://acm.org",
                                    LicenseModel::AnnualSubscription, Money::of(1000));

    cat.addHoldings("B1", 5);
    cat.addHoldings("E1", 10);
    cat.addHoldings("J1", 2);

    // 1. Author Search (Book and EBook)
    assert(cat.searchAuthor("martin").size() == 1);
    assert(cat.searchAuthor("martin")[0]->id() == "B1");
    assert(cat.searchAuthor("STROUSTRUP").size() == 1);
    assert(cat.searchAuthor("STROUSTRUP")[0]->id() == "E1");
    assert(cat.searchAuthor("Johnson").size() == 1);
    assert(cat.searchAuthor("Johnson")[0]->id() == "B2");
    assert(cat.searchAuthor("Unknown Author").empty());

    // 2. ISBN / ISSN Search
    assert(cat.searchIsbnIssn("0132350884").size() == 1);
    assert(cat.searchIsbnIssn("0132350884")[0]->id() == "B1");
    assert(cat.searchIsbnIssn("0321563842").size() == 1);
    assert(cat.searchIsbnIssn("0321563842")[0]->id() == "E1");
    assert(cat.searchIsbnIssn("2041-1723").size() == 1);
    assert(cat.searchIsbnIssn("2041-1723")[0]->id() == "J1");
    assert(cat.searchIsbnIssn("9999999").empty());

    // 3. Publication Year Range Search
    auto res90s = cat.searchYearRange(1990, 2000);
    assert(res90s.size() == 1);
    assert(res90s[0]->id() == "B2"); // 1994

    auto res2010to2020 = cat.searchYearRange(2010, 2020);
    assert(res2010to2020.size() == 4); // E1 (2013), A1 (2017), ER1 (2019), J1 (2020)

    auto resExact2008 = cat.searchYearRange(2008, 2008);
    assert(resExact2008.size() == 1);
    assert(resExact2008[0]->id() == "B1");

    assert(cat.searchYearRange(1900, 1950).empty());
    assertThrows<std::invalid_argument>([&] { cat.searchYearRange(2025, 2020); });

    // 4. In-Memory Persistence (save -> load round-trip)
    std::stringstream ss;
    cat.save(ss);

    Catalog loadedCat;
    loadedCat.load(ss);

    assert(loadedCat.size() == cat.size());
    assert(loadedCat.holdings("B1") == 5);
    assert(loadedCat.holdings("E1") == 10);
    assert(loadedCat.holdings("J1") == 2);
    assert(loadedCat.holdings("B2") == 0);

    // Verify exact derived types and properties
    auto b1 = dynamic_cast<const Book*>(loadedCat.find("B1"));
    assert(b1 != nullptr);
    assert(b1->title() == "Clean Code");
    assert(b1->authors().size() == 1);
    assert(b1->authors()[0] == "Robert C. Martin");
    assert(b1->isbn() == "978-0132350884");
    assert(b1->publisher() == "Prentice Hall");
    assert(b1->year() == 2008);
    assert(b1->unitPrice() == Money::of(450));
    assert(b1->binding() == Binding::Paperback);

    auto b2 = dynamic_cast<const Book*>(loadedCat.find("B2"));
    assert(b2 != nullptr);
    assert(b2->binding() == Binding::Hardcover);
    assert(b2->authors().size() == 4);

    auto eb1 = dynamic_cast<const EBook*>(loadedCat.find("E1"));
    assert(eb1 != nullptr);
    assert(eb1->format() == EBookFormat::PDF);
    assert(eb1->drm() == true);
    assert(eb1->license() == LicenseModel::AnnualSubscription);
    assert(eb1->platformFee() == Money::of(50));
    assert(eb1->accessUrl() == "https://example.com/cpp");

    auto ab1 = dynamic_cast<const AudioBook*>(loadedCat.find("A1"));
    assert(ab1 != nullptr);
    assert(ab1->narrator() == "Uncle Bob");
    assert(ab1->durationMinutes() == 600);
    assert(ab1->license() == LicenseModel::Perpetual);

    auto j1 = dynamic_cast<const Journal*>(loadedCat.find("J1"));
    assert(j1 != nullptr);
    assert(j1->issn() == "2041-1723");
    assert(j1->issuesPerYear() == 12);
    assert(j1->subscriptionYears() == 2);

    auto t1 = dynamic_cast<const Thesis*>(loadedCat.find("T1"));
    assert(t1 != nullptr);
    assert(t1->university() == "IIIT Delhi");
    assert(t1->degree() == "Ph.D");
    assert(t1->supervisor() == "Dr. Bhattacharya");

    auto er1 = dynamic_cast<const ElectronicResource*>(loadedCat.find("ER1"));
    assert(er1 != nullptr);
    assert(dynamic_cast<const EBook*>(er1) == nullptr);
    assert(dynamic_cast<const AudioBook*>(er1) == nullptr);

    // Verify searches still work on restored catalog
    assert(loadedCat.searchAuthor("martin").size() == 1);
    assert(loadedCat.searchIsbnIssn("2041-1723").size() == 1);
    assert(loadedCat.searchYearRange(1990, 2000).size() == 1);

    // 5. File Persistence (saveToFile -> loadFromFile)
    const std::string tempFile = "temp_catalog_test.txt";
    cat.saveToFile(tempFile);

    Catalog fileLoadedCat;
    fileLoadedCat.loadFromFile(tempFile);
    assert(fileLoadedCat.size() == cat.size());
    assert(fileLoadedCat.find("B1") != nullptr);
    std::remove(tempFile.c_str());

    // 6. Atomic loading error preservation
    Catalog intactCat;
    intactCat.emplace<Book>("KEEP1", "Original Book", std::vector<std::string>{"Orig"},
                            "isbn-orig", "Pub", 2020, Money::of(50));
    std::stringstream corruptedStream;
    corruptedStream << "Book|CORRUPT|MissingFields\n";
    assertThrows<std::runtime_error>([&] { intactCat.load(corruptedStream); });
    assert(intactCat.size() == 1);
    assert(intactCat.find("KEEP1") != nullptr);

    // Unknown type tag error
    std::stringstream unknownTypeStream;
    unknownTypeStream << "UnknownType|ID|Title\n";
    assertThrows<std::runtime_error>([&] { intactCat.load(unknownTypeStream); });
    assert(intactCat.size() == 1);

    // Non-existent file error
    assertThrows<std::runtime_error>([&] {
        fileLoadedCat.loadFromFile("non_existent_file_12345.txt");
    });
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q14 tests ────────────────────────────────────────────────────────────────
static void testLending() {
    Catalog cat;
    cat.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                      "isbn-b1", "Pub", 2008, Money::of(450));
    cat.emplace<Journal>("J1", "Nature", "Springer", 2024, Money::of(500), "2045-2322", 12, 1);
    cat.emplace<Thesis>("T1", "ML Research", "IIITD", 2026, Money::of(0), "IIITD", "M.Tech", "Supervisor");
    cat.emplace<ElectronicResource>("ER1", "IEEE Xplore", "IEEE", 2026, Money::of(100), "https://ieee.org");
    cat.emplace<EBook>("EB1", "C++ Programming", std::vector<std::string>{"Bjarne"}, "isbn-eb1", "AW", 2013,
                       Money::of(100), "https://cpp.org");
    cat.emplace<AudioBook>("AB1", "Audio Book 1", "AW", 2020, Money::of(80), "https://audio.org", "Narrator", 300);

    // Initial holdings
    cat.addHoldings("B1", 2);   // 2 print copies
    cat.addHoldings("J1", 1);   // 1 print copy
    cat.addHoldings("T1", 0);   // 0 print copies
    cat.addHoldings("ER1", 2);  // 2 licensed seats
    cat.addHoldings("EB1", 1);  // 1 licensed seat

    LendingManager lending(cat);

    // 1. Initial Availability checks
    assert(lending.availableCopies("B1") == 2);
    assert(lending.activeLoansCount("B1") == 0);
    assert(lending.availableSeats("ER1") == 2);
    assert(lending.activeSessionsCount("ER1") == 0);
    assert(lending.availableCopies("T1") == 0);

    // 2. Successful borrow and return
    std::string reason;
    assert(lending.canBorrow("P1", "B1", &reason));
    lending.borrow("P1", "B1");
    assert(lending.hasBorrowed("P1", "B1"));
    assert(lending.activeLoansCount("B1") == 1);
    assert(lending.availableCopies("B1") == 1);
    assert(cat.holdings("B1") == 2); // Catalog holdings must NEVER be decremented!

    lending.returnResource("P1", "B1");
    assert(!lending.hasBorrowed("P1", "B1"));
    assert(lending.activeLoansCount("B1") == 0);
    assert(lending.availableCopies("B1") == 2);
    assert(cat.holdings("B1") == 2); // Catalog holdings untouched

    // 3. Capacity limit on print copies
    lending.borrow("P1", "B1");
    lending.borrow("P2", "B1");
    assert(lending.availableCopies("B1") == 0);
    assert(lending.activeLoansCount("B1") == 2);

    // Borrowing when 0 copies available (capacity limit exceeded)
    assert(!lending.canBorrow("P3", "B1", &reason));
    assert(reason.find("no copies available") != std::string::npos);
    assertThrows<UnavailableError>([&] { lending.borrow("P3", "B1"); });
    assert(lending.activeLoansCount("B1") == 2); // state unchanged

    // 4. Borrowing with zero initial holdings (T1 has 0 holdings)
    assert(!lending.canBorrow("P1", "T1", &reason));
    assertThrows<UnavailableError>([&] { lending.borrow("P1", "T1"); });
    assert(lending.activeLoansCount("T1") == 0);

    // 5. Invalid return: patron didn't borrow the resource
    assertThrows<LendingError>([&] { lending.returnResource("P3", "B1"); });
    assertThrows<LendingError>([&] { lending.returnResource("P1", "J1"); });
    assert(lending.activeLoansCount("B1") == 2); // state unchanged

    // 6. Duplicate borrow by same patron for same resource
    assert(!lending.canBorrow("P1", "B1", &reason));
    assert(reason.find("already borrowed") != std::string::npos);
    assertThrows<LendingError>([&] { lending.borrow("P1", "B1"); });
    assert(lending.activeLoansCount("B1") == 2); // state unchanged

    // 7. Successful electronic session open/close
    assert(lending.canOpenSession("P1", "ER1", &reason));
    lending.openSession("P1", "ER1");
    assert(lending.hasSession("P1", "ER1"));
    assert(lending.activeSessionsCount("ER1") == 1);
    assert(lending.availableSeats("ER1") == 1);
    assert(cat.holdings("ER1") == 2); // Catalog holdings must NEVER be decremented!

    lending.closeSession("P1", "ER1");
    assert(!lending.hasSession("P1", "ER1"));
    assert(lending.activeSessionsCount("ER1") == 0);
    assert(lending.availableSeats("ER1") == 2);
    assert(cat.holdings("ER1") == 2);

    // 8. Electronic seat limit
    lending.openSession("P1", "ER1");
    lending.openSession("P2", "ER1");
    assert(lending.availableSeats("ER1") == 0);
    assert(lending.activeSessionsCount("ER1") == 2);

    // Opening session when all seats occupied
    assert(!lending.canOpenSession("P3", "ER1", &reason));
    assert(reason.find("all licensed seats occupied") != std::string::npos);
    assertThrows<UnavailableError>([&] { lending.openSession("P3", "ER1"); });
    assert(lending.activeSessionsCount("ER1") == 2); // state unchanged

    // 9. Invalid close: closing session not open
    assertThrows<LendingError>([&] { lending.closeSession("P3", "ER1"); });
    assertThrows<LendingError>([&] { lending.closeSession("P1", "EB1"); });
    assert(lending.activeSessionsCount("ER1") == 2); // state unchanged

    // 10. Duplicate session by same patron on same resource
    assert(!lending.canOpenSession("P1", "ER1", &reason));
    assert(reason.find("already has an open session") != std::string::npos);
    assertThrows<LendingError>([&] { lending.openSession("P1", "ER1"); });
    assert(lending.activeSessionsCount("ER1") == 2); // state unchanged

    // 11. Type mismatches:
    // borrow() on electronic resource (ER1, EB1, AB1) -> must fail
    assert(!lending.canBorrow("P1", "ER1", &reason));
    assert(reason.find("cannot borrow electronic resource") != std::string::npos);
    assertThrows<LendingError>([&] { lending.borrow("P1", "ER1"); });
    assertThrows<LendingError>([&] { lending.borrow("P1", "EB1"); });
    assertThrows<LendingError>([&] { lending.borrow("P1", "AB1"); });

    // openSession() on print resource (B1, J1, T1) -> must fail
    assert(!lending.canOpenSession("P1", "J1", &reason));
    assert(reason.find("cannot open session on print resource") != std::string::npos);
    assertThrows<LendingError>([&] { lending.openSession("P1", "J1"); });
    assertThrows<LendingError>([&] { lending.openSession("P1", "B1"); });
    assertThrows<LendingError>([&] { lending.openSession("P1", "T1"); });

    // 12. Non-existent resource
    assert(!lending.canBorrow("P1", "NONEXISTENT", &reason));
    assertThrows<NotFoundError>([&] { lending.borrow("P1", "NONEXISTENT"); });
    assertThrows<NotFoundError>([&] { lending.returnResource("P1", "NONEXISTENT"); });
    assert(!lending.canOpenSession("P1", "NONEXISTENT", &reason));
    assertThrows<NotFoundError>([&] { lending.openSession("P1", "NONEXISTENT"); });
    assertThrows<NotFoundError>([&] { lending.closeSession("P1", "NONEXISTENT"); });

    // Invalid patron ID (empty)
    assertThrows<std::invalid_argument>([&] { lending.borrow("", "B1"); });
    assertThrows<std::invalid_argument>([&] { lending.returnResource("", "B1"); });
    assertThrows<std::invalid_argument>([&] { lending.openSession("", "ER1"); });
    assertThrows<std::invalid_argument>([&] { lending.closeSession("", "ER1"); });

    // 13. Patron-specific active queries
    // P1 currently has: loan on B1, session on ER1
    // P2 currently has: loan on B1, session on ER1
    lending.borrow("P1", "J1"); // P1 borrows J1 as well
    lending.openSession("P1", "EB1"); // P1 opens session on EB1

    auto p1Loans = lending.activeLoansForPatron("P1");
    assert(p1Loans.size() == 2);
    auto p1Sessions = lending.activeSessionsForPatron("P1");
    assert(p1Sessions.size() == 2);

    auto p2Loans = lending.activeLoansForPatron("P2");
    assert(p2Loans.size() == 1);
    assert(p2Loans[0] == "B1");

    auto p3Loans = lending.activeLoansForPatron("P3");
    assert(p3Loans.empty());
    auto p3Sessions = lending.activeSessionsForPatron("P3");
    assert(p3Sessions.empty());
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Q15 tests ────────────────────────────────────────────────────────────────
static void testCurrency() {
    // 1. Default currency (INR) & zero construction
    Money mDefault;
    assert(mDefault.currency() == "INR");
    assert(mDefault.minorUnits() == 0);
    assert(mDefault.isZero());

    Money mOfDefault = Money::of(50, 25);
    assert(mOfDefault.currency() == "INR");
    assert(mOfDefault.minorUnits() == 5025);

    Money mMinorDefault = Money::fromMinor(1200);
    assert(mMinorDefault.currency() == "INR");
    assert(mMinorDefault.minorUnits() == 1200);

    // 2. Explicit currencies (USD, EUR, GBP)
    Money mUsd = Money::of(100, 50, "USD");
    assert(mUsd.currency() == "USD");
    assert(mUsd.minorUnits() == 10050);

    Money mEur = Money::fromMinor(2000, "EUR");
    assert(mEur.currency() == "EUR");
    assert(mEur.minorUnits() == 2000);

    Money mGbp = Money::of(45, 0, "GBP");
    assert(mGbp.currency() == "GBP");
    assert(mGbp.minorUnits() == 4500);

    // 3. Same-currency arithmetic & comparisons
    Money usd1 = Money::of(10, 50, "USD");
    Money usd2 = Money::of(5, 25, "USD");
    Money usdSum = usd1 + usd2;
    assert(usdSum.currency() == "USD");
    assert(usdSum == Money::of(15, 75, "USD"));

    Money usdDiff = usd1 - usd2;
    assert(usdDiff.currency() == "USD");
    assert(usdDiff == Money::of(5, 25, "USD"));

    Money usdCompound = usd1;
    usdCompound += usd2;
    assert(usdCompound == Money::of(15, 75, "USD"));
    usdCompound -= usd2;
    assert(usdCompound == usd1);

    assert(usd2 < usd1);
    assert(usd2 <= usd1);
    assert(usd1 > usd2);
    assert(usd1 >= usd2);
    assert(usd1 == usd1);
    assert(usd1 != usd2);

    // 4. Scalar multiplication preserving currency
    Money eur = Money::of(25, 50, "EUR");
    Money scaled1 = eur * 3;
    assert(scaled1.currency() == "EUR");
    assert(scaled1 == Money::of(76, 50, "EUR"));

    Money scaled2 = 2 * eur;
    assert(scaled2.currency() == "EUR");
    assert(scaled2 == Money::of(51, 0, "EUR"));

    Money eurCompound = eur;
    eurCompound *= 4;
    assert(eurCompound.currency() == "EUR");
    assert(eurCompound == Money::of(102, 0, "EUR"));

    // 5. Mismatched currency arithmetic (throws CurrencyMismatchError)
    Money inr = Money::of(100, 0, "INR");
    Money usd = Money::of(100, 0, "USD");

    assertThrows<CurrencyMismatchError>([&] { (void)(inr + usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(usd + inr); });
    assertThrows<CurrencyMismatchError>([&] { (void)(inr - usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(usd - inr); });

    Money inrMut = inr;
    assertThrows<CurrencyMismatchError>([&] { inrMut += usd; });
    assertThrows<CurrencyMismatchError>([&] { inrMut -= usd; });
    assert(inrMut == inr); // State unchanged on failure

    // 6. Mismatched currency comparisons (throws CurrencyMismatchError for ALL comparisons)
    assertThrows<CurrencyMismatchError>([&] { (void)(inr == usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(inr != usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(inr < usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(inr <= usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(inr > usd); });
    assertThrows<CurrencyMismatchError>([&] { (void)(inr >= usd); });

    // 7. Empty currency rejection
    assertThrows<std::invalid_argument>([&] { Money::of(10, 0, ""); });
    assertThrows<std::invalid_argument>([&] { Money::fromMinor(100, ""); });
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
    testAllOrNothingBatch();
    testVendorPricing();
    testCatalogSearchesAndPersistence();
    testLending();
    testCurrency();
    std::cout << "All tests passed!\n";
    return 0;
}
