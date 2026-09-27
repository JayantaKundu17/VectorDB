#include <cassert>
#include <iostream>

#include "core/VectorStore.h"

int main() {

    VectorStore store;

    // --------------------------------------------------
    // Insert
    // --------------------------------------------------

    bool inserted = store.insert({
        "doc_001",
        Vector({1.0f, 2.0f, 3.0f}),
        "Original document"
    });

    assert(inserted);
    assert(store.size() == 1);
    assert(store.exists("doc_001"));

    // Duplicate insert must fail
    bool duplicateInsert = store.insert({
        "doc_001",
        Vector({5.0f, 5.0f, 5.0f}),
        "Duplicate"
    });

    assert(!duplicateInsert);
    assert(store.size() == 1);

    // --------------------------------------------------
    // Get
    // --------------------------------------------------

    const VectorRecord* record = store.get("doc_001");

    assert(record != nullptr);
    assert(record->id == "doc_001");
    assert(record->text == "Original document");

    // --------------------------------------------------
    // Update
    // --------------------------------------------------

    bool updated = store.update({
        "doc_001",
        Vector({10.0f, 20.0f, 30.0f}),
        "Updated document"
    });

    assert(updated);

    record = store.get("doc_001");

    assert(record != nullptr);
    assert(record->text == "Updated document");
    assert(record->vector.data()[0] == 10.0f);
    assert(record->vector.data()[1] == 20.0f);
    assert(record->vector.data()[2] == 30.0f);

    // Updating a non-existing record must fail
    bool invalidUpdate = store.update({
        "does_not_exist",
        Vector({1.0f, 1.0f, 1.0f}),
        "Invalid"
    });

    assert(!invalidUpdate);

    // --------------------------------------------------
    // Upsert - update existing
    // --------------------------------------------------

    bool upsertExisting = store.upsert({
        "doc_001",
        Vector({100.0f, 200.0f, 300.0f}),
        "Upserted existing"
    });

    assert(upsertExisting);

    record = store.get("doc_001");

    assert(record != nullptr);
    assert(record->text == "Upserted existing");
    assert(record->vector.data()[0] == 100.0f);

    // --------------------------------------------------
    // Upsert - insert new
    // --------------------------------------------------

    bool upsertNew = store.upsert({
        "doc_002",
        Vector({4.0f, 5.0f, 6.0f}),
        "Upserted new"
    });

    assert(upsertNew);
    assert(store.size() == 2);
    assert(store.exists("doc_002"));

    // --------------------------------------------------
    // Remove
    // --------------------------------------------------

    bool removed = store.remove("doc_001");

    assert(removed);
    assert(!store.exists("doc_001"));
    assert(store.get("doc_001") == nullptr);
    assert(store.size() == 1);

    // Removing again must fail
    bool removeAgain = store.remove("doc_001");

    assert(!removeAgain);

    // --------------------------------------------------
    // Clear
    // --------------------------------------------------

    store.clear();

    assert(store.size() == 0);

    std::cout << "Mutation tests passed.\n";

    return 0;
}
