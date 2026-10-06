#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {
// Thesis derives directly from Resource because it is a distinct academic
// resource and is usually free, so electronic-resource licensing and pricing
// behavior is not appropriate.
class Thesis : public Resource {
public:
    Thesis(std::string id,
           std::string title,
           std::string publisher,
           int year,
           Money unitPrice,
           std::string university,
           std::string degree,
           std::string supervisor);

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override;

    Money costFor(int copies) const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

}  // namespace bookmgmt