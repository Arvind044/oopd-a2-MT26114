#include "bookmgmt/Journal.h"


#include <stdexcept>
#include <ostream>

namespace bookmgmt {
    Journal::Journal(std::string id,
                 std::string title,
                 std::string publisher,
                 int year,
                 Money unitPrice,
                 std::string issn,
                 int issuesPerYear,
                 int subscriptionYears)
    : Resource(std::move(id),
               std::move(title),
               std::move(publisher),
               year,
               unitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
        if (subscriptionYears < 1) {
        throw std::invalid_argument("subscriptionYears must be at least 1");
    }
 }
 ResourceCategory Journal::category() const {
    return ResourceCategory::Journal;
 }
 Money Journal::costFor(int copies) const {
    requirePositive(copies);

    return unitPrice() * copies * subscriptionYears_;
 }
 void Journal::printDetails(std::ostream& os) const {
    os << "  ISSN: " << issn_ << '\n';
    os << "  Issues per year: " << issuesPerYear_ << '\n';
    os << "  Subscription years: " << subscriptionYears_ << '\n';
}
}  // namespace bookmgmt