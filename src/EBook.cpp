#include "bookmgmt/EBook.h"
#include "bookmgmt/Book.h"

#include <ostream>
#include <utility>
namespace bookmgmt {
    EBook::EBook(std::string id,
             std::string title,
             std::vector<std::string> authors,
             std::string isbn,
             std::string publisher,
             int year,
             Money pricePerSeat,
             std::string accessUrl,
             LicenseModel license,
             Money platformFee,
             EBookFormat format,
             bool drm)
    : ElectronicResource(std::move(id),
                         std::move(title),
                         std::move(publisher),
                         year,
                         pricePerSeat,
                         std::move(accessUrl),
                         license,
                         platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drm_(drm) {
 }
 ResourceCategory EBook::category() const {
    return ResourceCategory::EBook;
 }
 void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);

    os << "  authors: " << joinAuthors(authors_) << '\n';
    os << "  ISBN: " << isbn_ << '\n';
    os << "  format: " << ebookFormatName(format_) << '\n';
    os << "  DRM: " << (drm_ ? "yes" : "no") << '\n';
 } 
 const char* ebookFormatName(EBookFormat format) {
    switch (format) {
        case EBookFormat::PDF:
            return "PDF";
        case EBookFormat::EPUB:
            return "EPUB";
        case EBookFormat::HTML:
            return "HTML";
    }

    return "Unknown";
 }

}  // namespace bookmgmt