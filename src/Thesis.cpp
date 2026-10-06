#include "bookmgmt/Thesis.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

Thesis::Thesis(std::string id,
               std::string title,
               std::string publisher,
               int year,
               Money unitPrice,
               std::string university,
               std::string degree,
               std::string supervisor)
    : Resource(std::move(id),
               std::move(title),
               std::move(publisher),
               year,
               unitPrice),
      university_(std::move(university)),
      degree_(std::move(degree)),
      supervisor_(std::move(supervisor)) {
}

ResourceCategory Thesis::category() const {
    return ResourceCategory::Thesis;
}

Money Thesis::costFor(int copies) const {
    requirePositive(copies);
    return Money{};
}

void Thesis::printDetails(std::ostream& os) const {
    os << "  University: " << university_ << '\n';
    os << "  Degree: " << degree_ << '\n';
    os << "  Supervisor: " << supervisor_ << '\n';
}

}  // namespace bookmgmt