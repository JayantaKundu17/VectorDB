#include <cassert>
#include <iostream>

#include "core/VectorStore.h"

int main() {

    VectorStore store;


    // --------------------------------------------------
    // Empty store
    // --------------------------------------------------

    assert(store.size() == 0);


    // --------------------------------------------------
    // Insert first vector
    // --------------------------------------------------

    VectorRecord record1{
        "doc_001",
        Vector({1.0f, 2.0f, 3.0f}),
        "First document"
    };

    bool inserted = store.insert(record1);

    assert(inserted);
    assert(store.size() == 1);


    // --------------------------------------------------
    // Check existence
    // --------------------------------------------------

    assert(store.exists("doc_001"));
    assert(!store.exists("doc_999"));


    // --------------------------------------------------
    // Retrieve vector
    // --------------------------------------------------

    const VectorRecord* retrieved =
        store.get("doc_001");

    assert(retrieved != nullptr);

    assert(retrieved->id == "doc_001");

    assert(retrieved->text == "First document");

    assert(retrieved->vector.dimension() == 3);

    assert(retrieved->vector[0] == 1.0f);
    assert(retrieved->vector[1] == 2.0f);
    assert(retrieved->vector[2] == 3.0f);


    // --------------------------------------------------
    // Duplicate ID should fail
    // --------------------------------------------------

    VectorRecord duplicate{
        "doc_001",
        Vector({4.0f, 5.0f, 6.0f}),
        "Duplicate document"
    };

    bool duplicateInserted =
        store.insert(duplicate);

    assert(!duplicateInserted);

    assert(store.size() == 1);


    // --------------------------------------------------
    // Insert second vector
    // --------------------------------------------------

    VectorRecord record2{
        "doc_002",
        Vector({4.0f, 5.0f, 6.0f}),
        "Second document"
    };

    assert(store.insert(record2));

    assert(store.size() == 2);


    // --------------------------------------------------
    // Remove vector
    // --------------------------------------------------

    bool removed =
        store.remove("doc_001");

    assert(removed);

    assert(!store.exists("doc_001"));

    assert(store.get("doc_001") == nullptr);

    assert(store.size() == 1);


    // --------------------------------------------------
    // Removing non-existent vector
    // --------------------------------------------------

    bool removedMissing =
        store.remove("does_not_exist");

    assert(!removedMissing);

    assert(store.size() == 1);


    // --------------------------------------------------
    // Clear database
    // --------------------------------------------------

    store.clear();

    assert(store.size() == 0);

    assert(!store.exists("doc_002"));


    std::cout << "VectorStore tests passed.\n";

    return 0;
}