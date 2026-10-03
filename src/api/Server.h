#pragma once

#include <string>

#include "core/VectorStore.h"
#include "indexes/HNSW.h"

class Server {
private:
    std::string databasePath;

    VectorStore store;

    HNSW index;

public:
    Server();

    void run(int port);
};