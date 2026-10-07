#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "bookmgmt/AudioBook.h"
#include "bookmgmt/Book.h"
#include "bookmgmt/EBook.h"
#include "bookmgmt/ElectronicResource.h"
#include "bookmgmt/Exceptions.h"
#include "bookmgmt/Journal.h"
#include "bookmgmt/Thesis.h"

namespace bookmgmt {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) {
        ++start;
    }
    return s.substr(start);
}

std::vector<std::string> splitFields(const std::string& line, char delim) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start < line.size()) {
        size_t end = line.find(delim, start);
        if (end == std::string::npos) {
            tokens.push_back(line.substr(start));
            break;
        }
        tokens.push_back(line.substr(start, end - start));
        start = end + 1;
        if (start == line.size()) {
            tokens.push_back("");
        }
    }
    return tokens;
}
}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> r) {
    if (!r) throw std::invalid_argument("cannot add a null resource");
    const std::string id = r->id();
    if (items_.count(id)) throw DuplicateIdError(id);
    Entry& e = items_[id];
    e.resource = std::move(r);
    return *e.resource;
}

bool Catalog::contains(const std::string& id) const { return items_.count(id) > 0; }

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    if (Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    if (const Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    if (items_.erase(id) == 0) throw NotFoundError(id);
}

int Catalog::holdings(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.holdings + units < 0)
        throw std::invalid_argument("holdings cannot become negative");
    it->second.holdings += units;
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& pred) const {
    std::vector<const Resource*> out;
    for (const auto& kv : items_)
        if (pred(*kv.second.resource)) out.push_back(kv.second.resource.get());
    return out;
}

std::vector<const Resource*> Catalog::all() const {
    return where([](const Resource&) { return true; });
}

std::vector<const Resource*> Catalog::byCategory(ResourceCategory c) const {
    return where([c](const Resource& r) { return r.category() == c; });
}

std::vector<const Resource*> Catalog::searchTitle(const std::string& text) const {
    const std::string needle = lower(text);
    return where([&needle](const Resource& r) {
        return lower(r.title()).find(needle) != std::string::npos;
    });
}

std::vector<const Resource*> Catalog::searchAuthor(const std::string& text) const {
    const std::string needle = lower(text);
    return where([&needle](const Resource& r) {
        if (auto b = dynamic_cast<const Book*>(&r)) {
            for (const auto& a : b->authors()) {
                if (lower(a).find(needle) != std::string::npos) return true;
            }
        }
        if (auto eb = dynamic_cast<const EBook*>(&r)) {
            for (const auto& a : eb->authors()) {
                if (lower(a).find(needle) != std::string::npos) return true;
            }
        }
        return false;
    });
}

std::vector<const Resource*> Catalog::searchIsbnIssn(const std::string& text) const {
    const std::string needle = lower(text);
    return where([&needle](const Resource& r) {
        if (auto b = dynamic_cast<const Book*>(&r)) {
            if (lower(b->isbn()).find(needle) != std::string::npos) return true;
        }
        if (auto eb = dynamic_cast<const EBook*>(&r)) {
            if (lower(eb->isbn()).find(needle) != std::string::npos) return true;
        }
        if (auto j = dynamic_cast<const Journal*>(&r)) {
            if (lower(j->issn()).find(needle) != std::string::npos) return true;
        }
        return false;
    });
}

std::vector<const Resource*> Catalog::searchYearRange(int startYear, int endYear) const {
    if (startYear > endYear) {
        throw std::invalid_argument("startYear must be <= endYear");
    }
    return where([startYear, endYear](const Resource& r) {
        return r.year() >= startYear && r.year() <= endYear;
    });
}

void Catalog::save(std::ostream& os) const {
    for (const auto& kv : items_) {
        const Resource& r = *kv.second.resource;
        const int h = kv.second.holdings;

        if (auto eb = dynamic_cast<const EBook*>(&r)) {
            std::string authorsJoined;
            for (size_t i = 0; i < eb->authors().size(); ++i) {
                if (i > 0) authorsJoined += ";";
                authorsJoined += eb->authors()[i];
            }
            std::string fmtStr = "PDF";
            if (eb->format() == EBookFormat::EPUB) fmtStr = "EPUB";
            else if (eb->format() == EBookFormat::HTML) fmtStr = "HTML";

            std::string licStr = (eb->license() == LicenseModel::Perpetual) ? "Perpetual" : "AnnualSubscription";

            os << "EBook|" << eb->id() << "|" << eb->title() << "|" << authorsJoined << "|"
               << eb->isbn() << "|" << eb->publisher() << "|" << eb->year() << "|"
               << eb->unitPrice().minorUnits() << "|" << eb->accessUrl() << "|"
               << licStr << "|" << eb->platformFee().minorUnits() << "|"
               << fmtStr << "|" << (eb->drm() ? "1" : "0") << "|" << h << "\n";
        } else if (auto ab = dynamic_cast<const AudioBook*>(&r)) {
            std::string licStr = (ab->license() == LicenseModel::Perpetual) ? "Perpetual" : "AnnualSubscription";

            os << "AudioBook|" << ab->id() << "|" << ab->title() << "|" << ab->publisher() << "|"
               << ab->year() << "|" << ab->unitPrice().minorUnits() << "|" << ab->accessUrl() << "|"
               << ab->narrator() << "|" << ab->durationMinutes() << "|"
               << licStr << "|" << ab->platformFee().minorUnits() << "|" << h << "\n";
        } else if (auto er = dynamic_cast<const ElectronicResource*>(&r)) {
            std::string licStr = (er->license() == LicenseModel::Perpetual) ? "Perpetual" : "AnnualSubscription";

            os << "ElectronicResource|" << er->id() << "|" << er->title() << "|" << er->publisher() << "|"
               << er->year() << "|" << er->unitPrice().minorUnits() << "|" << er->accessUrl() << "|"
               << licStr << "|" << er->platformFee().minorUnits() << "|" << h << "\n";
        } else if (auto b = dynamic_cast<const Book*>(&r)) {
            std::string authorsJoined;
            for (size_t i = 0; i < b->authors().size(); ++i) {
                if (i > 0) authorsJoined += ";";
                authorsJoined += b->authors()[i];
            }
            std::string bindStr = (b->binding() == Binding::Hardcover) ? "Hardcover" : "Paperback";

            os << "Book|" << b->id() << "|" << b->title() << "|" << authorsJoined << "|"
               << b->isbn() << "|" << b->publisher() << "|" << b->year() << "|"
               << b->unitPrice().minorUnits() << "|" << b->edition() << "|"
               << bindStr << "|" << h << "\n";
        } else if (auto j = dynamic_cast<const Journal*>(&r)) {
            os << "Journal|" << j->id() << "|" << j->title() << "|" << j->publisher() << "|"
               << j->year() << "|" << j->unitPrice().minorUnits() << "|" << j->issn() << "|"
               << j->issuesPerYear() << "|" << j->subscriptionYears() << "|" << h << "\n";
        } else if (auto t = dynamic_cast<const Thesis*>(&r)) {
            os << "Thesis|" << t->id() << "|" << t->title() << "|" << t->publisher() << "|"
               << t->year() << "|" << t->unitPrice().minorUnits() << "|" << t->university() << "|"
               << t->degree() << "|" << t->supervisor() << "|" << h << "\n";
        }
    }
}

void Catalog::saveToFile(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    save(ofs);
}

void Catalog::load(std::istream& is) {
    std::map<std::string, Entry> newItems;
    std::string line;

    while (std::getline(is, line)) {
        line = trim(line);
        if (line.empty()) continue;

        std::vector<std::string> tokens = splitFields(line, '|');
        if (tokens.empty()) continue;

        const std::string& typeTag = tokens[0];

        if (typeTag == "Book") {
            if (tokens.size() != 11) {
                throw std::runtime_error("Invalid Book record (expected 11 fields): " + line);
            }
            std::vector<std::string> authors = tokens[3].empty() ? std::vector<std::string>{} : splitFields(tokens[3], ';');
            Binding binding = (tokens[9] == "Hardcover") ? Binding::Hardcover : Binding::Paperback;
            auto book = std::make_unique<Book>(
                tokens[1], tokens[2], authors, tokens[4], tokens[5],
                std::stoi(tokens[6]), Money::fromMinor(std::stoll(tokens[7])),
                std::stoi(tokens[8]), binding);
            int h = std::stoi(tokens[10]);
            const std::string id = book->id();
            if (newItems.count(id)) throw DuplicateIdError(id);
            newItems[id] = Entry{std::move(book), h};
        } else if (typeTag == "ElectronicResource") {
            if (tokens.size() != 10) {
                throw std::runtime_error("Invalid ElectronicResource record (expected 10 fields): " + line);
            }
            LicenseModel license = (tokens[7] == "Perpetual") ? LicenseModel::Perpetual : LicenseModel::AnnualSubscription;
            auto er = std::make_unique<ElectronicResource>(
                tokens[1], tokens[2], tokens[3], std::stoi(tokens[4]),
                Money::fromMinor(std::stoll(tokens[5])), tokens[6],
                license, Money::fromMinor(std::stoll(tokens[8])));
            int h = std::stoi(tokens[9]);
            const std::string id = er->id();
            if (newItems.count(id)) throw DuplicateIdError(id);
            newItems[id] = Entry{std::move(er), h};
        } else if (typeTag == "Journal") {
            if (tokens.size() != 10) {
                throw std::runtime_error("Invalid Journal record (expected 10 fields): " + line);
            }
            auto j = std::make_unique<Journal>(
                tokens[1], tokens[2], tokens[3], std::stoi(tokens[4]),
                Money::fromMinor(std::stoll(tokens[5])), tokens[6],
                std::stoi(tokens[7]), std::stoi(tokens[8]));
            int h = std::stoi(tokens[9]);
            const std::string id = j->id();
            if (newItems.count(id)) throw DuplicateIdError(id);
            newItems[id] = Entry{std::move(j), h};
        } else if (typeTag == "EBook") {
            if (tokens.size() != 14) {
                throw std::runtime_error("Invalid EBook record (expected 14 fields): " + line);
            }
            std::vector<std::string> authors = tokens[3].empty() ? std::vector<std::string>{} : splitFields(tokens[3], ';');
            LicenseModel license = (tokens[9] == "Perpetual") ? LicenseModel::Perpetual : LicenseModel::AnnualSubscription;
            EBookFormat fmt = EBookFormat::PDF;
            if (tokens[11] == "EPUB") fmt = EBookFormat::EPUB;
            else if (tokens[11] == "HTML") fmt = EBookFormat::HTML;
            bool drm = (tokens[12] == "1" || tokens[12] == "true");

            auto eb = std::make_unique<EBook>(
                tokens[1], tokens[2], authors, tokens[4], tokens[5],
                std::stoi(tokens[6]), Money::fromMinor(std::stoll(tokens[7])),
                tokens[8], license, Money::fromMinor(std::stoll(tokens[10])),
                fmt, drm);
            int h = std::stoi(tokens[13]);
            const std::string id = eb->id();
            if (newItems.count(id)) throw DuplicateIdError(id);
            newItems[id] = Entry{std::move(eb), h};
        } else if (typeTag == "AudioBook") {
            if (tokens.size() != 12) {
                throw std::runtime_error("Invalid AudioBook record (expected 12 fields): " + line);
            }
            LicenseModel license = (tokens[9] == "Perpetual") ? LicenseModel::Perpetual : LicenseModel::AnnualSubscription;
            auto ab = std::make_unique<AudioBook>(
                tokens[1], tokens[2], tokens[3], std::stoi(tokens[4]),
                Money::fromMinor(std::stoll(tokens[5])), tokens[6],
                tokens[7], std::stoi(tokens[8]), license,
                Money::fromMinor(std::stoll(tokens[10])));
            int h = std::stoi(tokens[11]);
            const std::string id = ab->id();
            if (newItems.count(id)) throw DuplicateIdError(id);
            newItems[id] = Entry{std::move(ab), h};
        } else if (typeTag == "Thesis") {
            if (tokens.size() != 10) {
                throw std::runtime_error("Invalid Thesis record (expected 10 fields): " + line);
            }
            auto t = std::make_unique<Thesis>(
                tokens[1], tokens[2], tokens[3], std::stoi(tokens[4]),
                Money::fromMinor(std::stoll(tokens[5])), tokens[6],
                tokens[7], tokens[8]);
            int h = std::stoi(tokens[9]);
            const std::string id = t->id();
            if (newItems.count(id)) throw DuplicateIdError(id);
            newItems[id] = Entry{std::move(t), h};
        } else {
            throw std::runtime_error("Unknown resource type tag: " + typeTag);
        }
    }

    items_ = std::move(newItems);
}

void Catalog::loadFromFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not open file for reading: " + filename);
    }
    load(ifs);
}

}  // namespace bookmgmt
