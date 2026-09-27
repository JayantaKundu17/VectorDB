#include <cassert>
#include <cstdio>
#include <iostream>

#include "core/VectorStore.h"

int main() {

    const std::string filename =
        "test_vectordb.db";

    VectorStore store;

    // --------------------------------------------------------
    // Insert
    // --------------------------------------------------------

    assert(
        store.insert({
            "vec_1",
            Vector({1.0f, 2.0f, 3.0f}),
            "first vector"
        })
    );

    assert(store.size() == 1);

    // --------------------------------------------------------
    // Update existing record
    // --------------------------------------------------------

    assert(
        store.update({
            "vec_1",
            Vector({10.0f, 20.0f, 30.0f}),
            "updated vector"
        })
    );

    const VectorRecord* updated =
        store.get("vec_1");

    assert(updated != nullptr);

    assert(
        updated->text == "updated vector"
    );

    assert(
        updated->vector.data()[0] == 10.0f
    );

    assert(
        updated->vector.data()[1] == 20.0f
    );

    assert(
        updated->vector.data()[2] == 30.0f
    );

    // Updating a non-existing ID must fail.
    assert(
        !store.update({
            "does_not_exist",
            Vector({1.0f}),
            "invalid update"
        })
    );

    // --------------------------------------------------------
    // Upsert existing record
    // --------------------------------------------------------

    assert(
        store.upsert({
            "vec_1",
            Vector({100.0f, 200.0f}),
            "upserted vector"
        })
    );

    assert(store.size() == 1);

    const VectorRecord* upserted =
        store.get("vec_1");

    assert(upserted != nullptr);

    assert(
        upserted->text == "upserted vector"
    );

    assert(
        upserted->vector.data().size() == 2
    );

    assert(
        upserted->vector.data()[0] == 100.0f
    );

    // --------------------------------------------------------
    // Upsert new record
    // --------------------------------------------------------

    assert(
        store.upsert({
            "vec_2",
            Vector({4.0f, 5.0f, 6.0f}),
            "new vector"
        })
    );

    assert(store.size() == 2);

    assert(store.exists("vec_2"));

    // --------------------------------------------------------
    // Persistence
    // --------------------------------------------------------

    assert(store.save(filename));

    store.clear();

    assert(store.size() == 0);

    assert(store.load(filename));

    assert(store.size() == 2);

    // Verify vec_1
    const VectorRecord* loaded1 =
        store.get("vec_1");

    assert(loaded1 != nullptr);

    assert(
        loaded1->text == "upserted vector"
    );

    assert(
        loaded1->vector.data()[0] == 100.0f
    );

    // Verify vec_2
    const VectorRecord* loaded2 =
        store.get("vec_2");

    assert(loaded2 != nullptr);

    assert(
        loaded2->text == "new vector"
    );

    assert(
        loaded2->vector.data()[0] == 4.0f
    );

    // --------------------------------------------------------
    // Remove
    // --------------------------------------------------------

    assert(
        store.remove("vec_1")
    );

    assert(
        !store.exists("vec_1")
    );

    assert(store.size() == 1);

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    std::remove(filename.c_str());

    std::cout
        << "Persistence and update/upsert test passed.\n";

    return 0;
}