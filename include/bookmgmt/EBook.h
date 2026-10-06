#pragma once

#include <string>
#include <vector>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

enum class EBookFormat {
    PDF,
    EPUB,
    HTML
};

class EBook : public ElectronicResource {
public:
    EBook(std::string id,
          std::string title,
          std::vector<std::string> authors,
          std::string isbn,
          std::string publisher,
          int year,
          Money pricePerSeat,
          std::string accessUrl,
          LicenseModel license = LicenseModel::AnnualSubscription,
          Money platformFee = Money{},
          EBookFormat format = EBookFormat::PDF,
          bool drm = false);

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    EBookFormat format() const { return format_; }
    bool drm() const { return drm_; }

    ResourceCategory category() const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    EBookFormat format_;
    bool drm_;
};

const char* ebookFormatName(EBookFormat format);

}  // namespace bookmgmt