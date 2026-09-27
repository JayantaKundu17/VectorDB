#include "core/VectorStore.h"

#include <cstdint>
#include <fstream>

namespace {

bool writeString(
    std::ofstream& file,
    const std::string& value
) {
    std::uint64_t length =
        static_cast<std::uint64_t>(value.size());

    file.write(
        reinterpret_cast<const char*>(&length),
        sizeof(length)
    );

    if (!file) {
        return false;
    }

    if (length > 0) {
        file.write(
            value.data(),
            static_cast<std::streamsize>(length)
        );
    }

    return static_cast<bool>(file);
}

bool readString(
    std::ifstream& file,
    std::string& value
) {
    std::uint64_t length = 0;

    file.read(
        reinterpret_cast<char*>(&length),
        sizeof(length)
    );

    if (!file) {
        return false;
    }

    constexpr std::uint64_t MAX_STRING_SIZE =
        100ULL * 1024ULL * 1024ULL;

    if (length > MAX_STRING_SIZE) {
        return false;
    }

    value.resize(static_cast<std::size_t>(length));

    if (length > 0) {
        file.read(
            &value[0],
            static_cast<std::streamsize>(length)
        );
    }

    return static_cast<bool>(file);
}

} // namespace


bool VectorStore::insert(
    const VectorRecord& record
) {

    if (record.id.empty()) {
        return false;
    }

    if (exists(record.id)) {
        return false;
    }

    records.emplace(
        record.id,
        record
    );

    return true;
}


// ============================================================
// Update
// ============================================================

bool VectorStore::update(
    const VectorRecord& record
) {

    if (record.id.empty()) {
        return false;
    }

    auto it = records.find(record.id);

    if (it == records.end()) {
        return false;
    }

    it->second = record;

    return true;
}


// ============================================================
// Upsert
// ============================================================

bool VectorStore::upsert(
    const VectorRecord& record
) {

    if (record.id.empty()) {
        return false;
    }

    auto it = records.find(record.id);

    if (it == records.end()) {

        records.emplace(
            record.id,
            record
        );

        return true;
    }

    it->second = record;

    return true;
}


bool VectorStore::exists(
    const std::string& id
) const {

    return records.find(id) != records.end();
}


const VectorRecord* VectorStore::get(
    const std::string& id
) const {

    auto it = records.find(id);

    if (it == records.end()) {
        return nullptr;
    }

    return &it->second;
}


bool VectorStore::remove(
    const std::string& id
) {

    return records.erase(id) > 0;
}


std::size_t VectorStore::size() const {

    return records.size();
}


void VectorStore::clear() {

    records.clear();
}


const std::unordered_map<std::string, VectorRecord>&
VectorStore::getAll() const {

    return records;
}


// ============================================================
// Persistence
// ============================================================

bool VectorStore::save(
    const std::string& filename
) const {

    std::ofstream file(
        filename,
        std::ios::binary
    );

    if (!file) {
        return false;
    }

    constexpr std::uint64_t MAGIC =
        0x564543444231ULL;

    constexpr std::uint32_t VERSION = 1;

    const std::uint64_t recordCount =
        static_cast<std::uint64_t>(
            records.size()
        );

    file.write(
        reinterpret_cast<const char*>(&MAGIC),
        sizeof(MAGIC)
    );

    file.write(
        reinterpret_cast<const char*>(&VERSION),
        sizeof(VERSION)
    );

    file.write(
        reinterpret_cast<const char*>(&recordCount),
        sizeof(recordCount)
    );

    if (!file) {
        return false;
    }

    for (const auto& pair : records) {

        const VectorRecord& record =
            pair.second;

        if (!writeString(
                file,
                record.id
            )) {
            return false;
        }

        if (!writeString(
                file,
                record.text
            )) {
            return false;
        }

        const std::vector<float>& data =
            record.vector.data();

        const std::uint64_t dimension =
            static_cast<std::uint64_t>(
                data.size()
            );

        file.write(
            reinterpret_cast<const char*>(&dimension),
            sizeof(dimension)
        );

        if (!file) {
            return false;
        }

        if (dimension > 0) {

            file.write(
                reinterpret_cast<const char*>(
                    data.data()
                ),
                static_cast<std::streamsize>(
                    dimension * sizeof(float)
                )
            );

            if (!file) {
                return false;
            }
        }
    }

    return true;
}


bool VectorStore::load(
    const std::string& filename
) {

    std::ifstream file(
        filename,
        std::ios::binary
    );

    if (!file) {
        return false;
    }

    constexpr std::uint64_t MAGIC =
        0x564543444231ULL;

    constexpr std::uint32_t VERSION = 1;

    std::uint64_t storedMagic = 0;
    std::uint32_t storedVersion = 0;
    std::uint64_t recordCount = 0;

    file.read(
        reinterpret_cast<char*>(&storedMagic),
        sizeof(storedMagic)
    );

    file.read(
        reinterpret_cast<char*>(&storedVersion),
        sizeof(storedVersion)
    );

    file.read(
        reinterpret_cast<char*>(&recordCount),
        sizeof(recordCount)
    );

    if (!file) {
        return false;
    }

    if (storedMagic != MAGIC) {
        return false;
    }

    if (storedVersion != VERSION) {
        return false;
    }

    constexpr std::uint64_t MAX_RECORDS =
        100000000ULL;

    if (recordCount > MAX_RECORDS) {
        return false;
    }

    std::unordered_map<std::string, VectorRecord>
        loadedRecords;

    for (
        std::uint64_t i = 0;
        i < recordCount;
        ++i
    ) {

        VectorRecord record;

        if (!readString(
                file,
                record.id
            )) {
            return false;
        }

        if (record.id.empty()) {
            return false;
        }

        if (!readString(
                file,
                record.text
            )) {
            return false;
        }

        std::uint64_t dimension = 0;

        file.read(
            reinterpret_cast<char*>(&dimension),
            sizeof(dimension)
        );

        if (!file) {
            return false;
        }

        constexpr std::uint64_t MAX_DIMENSION =
            1000000ULL;

        if (dimension > MAX_DIMENSION) {
            return false;
        }

        std::vector<float> values(
            static_cast<std::size_t>(
                dimension
            )
        );

        if (dimension > 0) {

            file.read(
                reinterpret_cast<char*>(
                    values.data()
                ),
                static_cast<std::streamsize>(
                    dimension * sizeof(float)
                )
            );

            if (!file) {
                return false;
            }
        }

        record.vector = Vector(values);

        auto result =
            loadedRecords.emplace(
                record.id,
                std::move(record)
            );

        if (!result.second) {
            return false;
        }
    }

    records = std::move(
        loadedRecords
    );

    return true;
}