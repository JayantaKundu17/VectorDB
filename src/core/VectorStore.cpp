#include "core/VectorStore.h"

#include <cstdint>
#include <fstream>
#include <filesystem>

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

    value.resize(
        static_cast<std::size_t>(length)
    );

    if (length > 0) {
        file.read(
            &value[0],
            static_cast<std::streamsize>(length)
        );
    }

    return static_cast<bool>(file);
}

} // namespace


// ============================================================
// Constructor
// ============================================================

VectorStore::VectorStore(
    const std::string& filename
)
    : persistenceFile(filename) {
}


// ============================================================
// Insert
// ============================================================

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

    // Persist immediately.
    if (!save()) {
        // Keep the in-memory record, but report failure.
        return false;
    }

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

    if (!save()) {
        return false;
    }

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

    } else {

        it->second = record;
    }

    if (!save()) {
        return false;
    }

    return true;
}


// ============================================================
// Exists
// ============================================================

bool VectorStore::exists(
    const std::string& id
) const {
    return records.find(id) != records.end();
}


// ============================================================
// Get
// ============================================================

const VectorRecord* VectorStore::get(
    const std::string& id
) const {
    auto it = records.find(id);

    if (it == records.end()) {
        return nullptr;
    }

    return &it->second;
}


// ============================================================
// Remove
// ============================================================

bool VectorStore::remove(
    const std::string& id
) {
    auto erased = records.erase(id);

    if (erased == 0) {
        return false;
    }

    if (!save()) {
        return false;
    }

    return true;
}


// ============================================================
// Size
// ============================================================

std::size_t VectorStore::size() const {
    return records.size();
}


// ============================================================
// Clear
// ============================================================

void VectorStore::clear() {

    records.clear();

    save();
}


// ============================================================
// Get All
// ============================================================

const std::unordered_map<
    std::string,
    VectorRecord
>&
VectorStore::getAll() const {
    return records;
}


// ============================================================
// Save using default persistence file
// ============================================================

bool VectorStore::save() const {
    return save(persistenceFile);
}


// ============================================================
// Load using default persistence file
// ============================================================

bool VectorStore::load() {
    return load(persistenceFile);
}


// ============================================================
// Persistence - Save
// ============================================================

bool VectorStore::save(
    const std::string& filename
) const {

    try {

        // Make sure parent directory exists.
        std::filesystem::path path(filename);

        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(
                path.parent_path()
            );
        }

        // Write to a temporary file first.
        //
        // This prevents a crash during writing from
        // destroying the previous valid database.
        std::string tempFilename =
            filename + ".tmp";

        std::ofstream file(
            tempFilename,
            std::ios::binary |
            std::ios::trunc
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

        // ----------------------------------------------------
        // Header
        // ----------------------------------------------------

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

        // ----------------------------------------------------
        // Records
        // ----------------------------------------------------

        for (const auto& pair : records) {

            const VectorRecord& record =
                pair.second;

            // ID
            if (!writeString(
                    file,
                    record.id
                )) {
                return false;
            }

            // Text
            if (!writeString(
                    file,
                    record.text
                )) {
                return false;
            }

            // Vector
            const std::vector<float>& data =
                record.vector.data();

            const std::uint64_t dimension =
                static_cast<std::uint64_t>(
                    data.size()
                );

            file.write(
                reinterpret_cast<const char*>(
                    &dimension
                ),
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

        file.flush();

        if (!file) {
            return false;
        }

        file.close();

        // ----------------------------------------------------
        // Atomically replace old database
        // ----------------------------------------------------

        std::error_code ec;

        std::filesystem::rename(
            tempFilename,
            filename,
            ec
        );

        if (ec) {

            // On some systems rename() fails if the
            // destination already exists.

            std::filesystem::remove(
                filename,
                ec
            );

            ec.clear();

            std::filesystem::rename(
                tempFilename,
                filename,
                ec
            );

            if (ec) {
                return false;
            }
        }

        return true;

    }
    catch (...) {

        return false;
    }
}


// ============================================================
// Persistence - Load
// ============================================================

bool VectorStore::load(
    const std::string& filename
) {

    std::ifstream file(
        filename,
        std::ios::binary
    );

    // First startup: database doesn't exist yet.
    if (!file) {
        return false;
    }

    constexpr std::uint64_t MAGIC =
        0x564543444231ULL;

    constexpr std::uint32_t VERSION = 1;

    std::uint64_t storedMagic = 0;
    std::uint32_t storedVersion = 0;
    std::uint64_t recordCount = 0;

    // --------------------------------------------------------
    // Header
    // --------------------------------------------------------

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

    // Check magic number.
    if (storedMagic != MAGIC) {
        return false;
    }

    // Check database version.
    if (storedVersion != VERSION) {
        return false;
    }

    constexpr std::uint64_t MAX_RECORDS =
        100000000ULL;

    if (recordCount > MAX_RECORDS) {
        return false;
    }

    // Load into a temporary map first.
    //
    // This prevents partially loaded data from
    // corrupting the current in-memory store.
    std::unordered_map<
        std::string,
        VectorRecord
    > loadedRecords;

    loadedRecords.reserve(
        static_cast<std::size_t>(
            recordCount
        )
    );

    // --------------------------------------------------------
    // Records
    // --------------------------------------------------------

    for (
        std::uint64_t i = 0;
        i < recordCount;
        ++i
    ) {

        std::string id;
        std::string text;

        // Read ID
        if (!readString(
                file,
                id
            )) {
            return false;
        }

        // Read text
        if (!readString(
                file,
                text
            )) {
            return false;
        }

        // Read vector dimension
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

        std::vector<float> values;

        values.resize(
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

        Vector vector(values);

        VectorRecord record{
            id,
            vector,
            text
        };

        loadedRecords.emplace(
            id,
            std::move(record)
        );
    }

    // --------------------------------------------------------
    // Replace current records
    // --------------------------------------------------------

    records = std::move(
        loadedRecords
    );

    return true;
}