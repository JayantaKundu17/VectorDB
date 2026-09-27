#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/Vector.h"

struct VectorRecord {
    std::string id;
    Vector vector;
    std::string text;
};

class VectorStore {
private:
    std::unordered_map<std::string, VectorRecord> records;

public:
    bool insert(const VectorRecord& record);

    bool update(const VectorRecord& record);

    bool upsert(const VectorRecord& record);

    bool exists(const std::string& id) const;

    const VectorRecord* get(const std::string& id) const;

    bool remove(const std::string& id);

    std::size_t size() const;

    void clear();

    const std::unordered_map<std::string, VectorRecord>& getAll() const;

    // Persistence
    bool save(const std::string& filename) const;

    bool load(const std::string& filename);
};