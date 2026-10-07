#include "bookmgmt/LendingManager.h"

#include <stdexcept>

#include "bookmgmt/Exceptions.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

bool LendingManager::canBorrow(const std::string& patronId, const std::string& resourceId,
                               std::string* reason) const {
    if (patronId.empty()) {
        if (reason) *reason = "patron ID cannot be empty";
        return false;
    }
    const Resource* r = catalog_.find(resourceId);
    if (!r) {
        if (reason) *reason = "resource not found: " + resourceId;
        return false;
    }
    if (r->isDigital()) {
        if (reason) *reason = "cannot borrow electronic resource '" + resourceId + "'; use openSession";
        return false;
    }
    if (hasBorrowed(patronId, resourceId)) {
        if (reason) *reason = "patron '" + patronId + "' has already borrowed resource '" + resourceId + "'";
        return false;
    }
    int held = catalog_.holdings(resourceId);
    int currentLoans = activeLoansCount(resourceId);
    if (currentLoans >= held) {
        if (reason) {
            *reason = "no copies available for resource '" + resourceId + "' (holdings: " +
                      std::to_string(held) + ", active loans: " + std::to_string(currentLoans) + ")";
        }
        return false;
    }
    return true;
}

void LendingManager::borrow(const std::string& patronId, const std::string& resourceId) {
    if (patronId.empty()) {
        throw std::invalid_argument("patron ID cannot be empty");
    }
    const Resource& r = catalog_.get(resourceId);
    if (r.isDigital()) {
        throw LendingError("cannot borrow electronic resource '" + resourceId + "'; use openSession");
    }
    if (hasBorrowed(patronId, resourceId)) {
        throw LendingError("patron '" + patronId + "' has already borrowed resource '" + resourceId + "'");
    }
    int held = catalog_.holdings(resourceId);
    int currentLoans = activeLoansCount(resourceId);
    if (currentLoans >= held) {
        throw UnavailableError("no copies available for resource '" + resourceId + "' (holdings: " +
                               std::to_string(held) + ", active loans: " + std::to_string(currentLoans) + ")");
    }
    activeLoans_[resourceId].insert(patronId);
}

void LendingManager::returnResource(const std::string& patronId, const std::string& resourceId) {
    if (patronId.empty()) {
        throw std::invalid_argument("patron ID cannot be empty");
    }
    (void)catalog_.get(resourceId); // verify existence (throws NotFoundError if absent)
    if (!hasBorrowed(patronId, resourceId)) {
        throw LendingError("patron '" + patronId + "' has not borrowed resource '" + resourceId + "'");
    }
    auto it = activeLoans_.find(resourceId);
    if (it != activeLoans_.end()) {
        it->second.erase(patronId);
        if (it->second.empty()) {
            activeLoans_.erase(it);
        }
    }
}

bool LendingManager::canOpenSession(const std::string& patronId, const std::string& resourceId,
                                    std::string* reason) const {
    if (patronId.empty()) {
        if (reason) *reason = "patron ID cannot be empty";
        return false;
    }
    const Resource* r = catalog_.find(resourceId);
    if (!r) {
        if (reason) *reason = "resource not found: " + resourceId;
        return false;
    }
    if (!r->isDigital()) {
        if (reason) *reason = "cannot open session on print resource '" + resourceId + "'; use borrow";
        return false;
    }
    if (hasSession(patronId, resourceId)) {
        if (reason) *reason = "patron '" + patronId + "' already has an open session on '" + resourceId + "'";
        return false;
    }
    int seats = catalog_.holdings(resourceId);
    int currentSessions = activeSessionsCount(resourceId);
    if (currentSessions >= seats) {
        if (reason) {
            *reason = "all licensed seats occupied for resource '" + resourceId + "' (seats: " +
                      std::to_string(seats) + ", active sessions: " + std::to_string(currentSessions) + ")";
        }
        return false;
    }
    return true;
}

void LendingManager::openSession(const std::string& patronId, const std::string& resourceId) {
    if (patronId.empty()) {
        throw std::invalid_argument("patron ID cannot be empty");
    }
    const Resource& r = catalog_.get(resourceId);
    if (!r.isDigital()) {
        throw LendingError("cannot open session on print resource '" + resourceId + "'; use borrow");
    }
    if (hasSession(patronId, resourceId)) {
        throw LendingError("patron '" + patronId + "' already has an open session on '" + resourceId + "'");
    }
    int seats = catalog_.holdings(resourceId);
    int currentSessions = activeSessionsCount(resourceId);
    if (currentSessions >= seats) {
        throw UnavailableError("all licensed seats occupied for resource '" + resourceId + "' (seats: " +
                               std::to_string(seats) + ", active sessions: " + std::to_string(currentSessions) + ")");
    }
    activeSessions_[resourceId].insert(patronId);
}

void LendingManager::closeSession(const std::string& patronId, const std::string& resourceId) {
    if (patronId.empty()) {
        throw std::invalid_argument("patron ID cannot be empty");
    }
    (void)catalog_.get(resourceId); // verify existence (throws NotFoundError if absent)
    if (!hasSession(patronId, resourceId)) {
        throw LendingError("patron '" + patronId + "' has no open session on resource '" + resourceId + "'");
    }
    auto it = activeSessions_.find(resourceId);
    if (it != activeSessions_.end()) {
        it->second.erase(patronId);
        if (it->second.empty()) {
            activeSessions_.erase(it);
        }
    }
}

int LendingManager::activeLoansCount(const std::string& resourceId) const {
    auto it = activeLoans_.find(resourceId);
    return it == activeLoans_.end() ? 0 : static_cast<int>(it->second.size());
}

int LendingManager::availableCopies(const std::string& resourceId) const {
    return catalog_.holdings(resourceId) - activeLoansCount(resourceId);
}

int LendingManager::activeSessionsCount(const std::string& resourceId) const {
    auto it = activeSessions_.find(resourceId);
    return it == activeSessions_.end() ? 0 : static_cast<int>(it->second.size());
}

int LendingManager::availableSeats(const std::string& resourceId) const {
    return catalog_.holdings(resourceId) - activeSessionsCount(resourceId);
}

bool LendingManager::hasBorrowed(const std::string& patronId, const std::string& resourceId) const {
    auto it = activeLoans_.find(resourceId);
    return it != activeLoans_.end() && it->second.count(patronId) > 0;
}

bool LendingManager::hasSession(const std::string& patronId, const std::string& resourceId) const {
    auto it = activeSessions_.find(resourceId);
    return it != activeSessions_.end() && it->second.count(patronId) > 0;
}

std::vector<std::string> LendingManager::activeLoansForPatron(const std::string& patronId) const {
    std::vector<std::string> res;
    for (const auto& kv : activeLoans_) {
        if (kv.second.count(patronId) > 0) {
            res.push_back(kv.first);
        }
    }
    return res;
}

std::vector<std::string> LendingManager::activeSessionsForPatron(const std::string& patronId) const {
    std::vector<std::string> res;
    for (const auto& kv : activeSessions_) {
        if (kv.second.count(patronId) > 0) {
            res.push_back(kv.first);
        }
    }
    return res;
}

}  // namespace bookmgmt
