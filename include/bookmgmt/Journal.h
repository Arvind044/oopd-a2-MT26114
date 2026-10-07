#pragma once

#include "bookmgmt/Resource.h"

#include <string>

namespace bookmgmt {

class Journal : public Resource {
public:
    Journal(std::string id,
            std::string title,
            std::string publisher,
            int year,
            Money unitPrice,
            std::string issn,
            int issuesPerYear,
            int subscriptionYears = 1);

    ResourceCategory category() const override;

    const std::string& issn() const { return issn_; }
    int issuesPerYear() const { return issuesPerYear_; }
    int subscriptionYears() const { return subscriptionYears_; }

    Money costFor(int copies) const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string issn_;
    int issuesPerYear_;
    int subscriptionYears_;
};

}  // namespace bookmgmt