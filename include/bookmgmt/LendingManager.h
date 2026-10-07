#pragma once
// LendingManager: Manages physical book borrowing/returns and concurrent
// electronic resource session opening/closing for library patrons.

#include <map>
#include <set>
#include <string>
#include <vector>

#include "bookmgmt/Catalog.h"

namespace bookmgmt {

class LendingManager {
public:
    explicit LendingManager(Catalog& catalog) : catalog_(catalog) {}

    // Print resource lending
    bool canBorrow(const std::string& patronId, const std::string& resourceId,
                   std::string* reason = nullptr) const;
    void borrow(const std::string& patronId, const std::string& resourceId);
    void returnResource(const std::string& patronId, const std::string& resourceId);

    // Electronic resource concurrent sessions
    bool canOpenSession(const std::string& patronId, const std::string& resourceId,
                        std::string* reason = nullptr) const;
    void openSession(const std::string& patronId, const std::string& resourceId);
    void closeSession(const std::string& patronId, const std::string& resourceId);

    // Availability & active counters
    int activeLoansCount(const std::string& resourceId) const;
    int availableCopies(const std::string& resourceId) const;

    int activeSessionsCount(const std::string& resourceId) const;
    int availableSeats(const std::string& resourceId) const;

    // Direct checks
    bool hasBorrowed(const std::string& patronId, const std::string& resourceId) const;
    bool hasSession(const std::string& patronId, const std::string& resourceId) const;

    // Per-patron active lists
    std::vector<std::string> activeLoansForPatron(const std::string& patronId) const;
    std::vector<std::string> activeSessionsForPatron(const std::string& patronId) const;

private:
    Catalog& catalog_;
    // resourceId -> set of patronIds
    std::map<std::string, std::set<std::string>> activeLoans_;
    std::map<std::string, std::set<std::string>> activeSessions_;
};

}  // namespace bookmgmt
