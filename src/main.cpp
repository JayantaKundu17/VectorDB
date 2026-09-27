
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "core/VectorStore.h"
#include "indexes/BruteForce.h"
#include "distance/DistanceMetric.h"

namespace {

void printHelp() {
    std::cout
        << "\nAvailable commands:\n\n"
        << "  insert <id> <v1> <v2> ... <vn>\n"
        << "      Insert a vector.\n"
        << "      Example: insert doc_001 1.0 2.0 3.0\n\n"

        << "  search <v1> <v2> ... <vn> <k> <metric>\n"
        << "      Search for the top-k nearest vectors.\n"
        << "      Metrics: cosine, euclidean, manhattan\n"
        << "      Example: search 1.0 2.0 3.0 3 cosine\n\n"

        << "  get <id>\n"
        << "      Retrieve a stored vector.\n"
        << "      Example: get doc_001\n\n"

        << "  delete <id>\n"
        << "      Delete a vector.\n"
        << "      Example: delete doc_001\n\n"

        << "  list\n"
        << "      Show the number of stored vectors.\n\n"

        << "  save <filename>\n"
        << "      Save the database to disk.\n"
        << "      Example: save database.db\n\n"

        << "  load <filename>\n"
        << "      Load the database from disk.\n"
        << "      Example: load database.db\n\n"

        << "  help\n"
        << "      Show this help message.\n\n"

        << "  exit\n"
        << "      Exit VectorDB.\n\n";
}

DistanceMetric parseMetric(const std::string& metric) {

    if (metric == "cosine") {
        return DistanceMetric::COSINE;
    }

    if (metric == "euclidean") {
        return DistanceMetric::EUCLIDEAN;
    }

    if (metric == "manhattan") {
        return DistanceMetric::MANHATTAN;
    }

    throw std::invalid_argument(
        "Unknown metric. Use cosine, euclidean, or manhattan."
    );
}

void printResults(
    const std::vector<SearchResult>& results,
    const std::string& metric
) {
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }

    std::cout << "\nSearch results:\n\n";

    for (std::size_t i = 0; i < results.size(); ++i) {

        std::cout
            << i + 1
            << ". "
            << results[i].id
            << " | ";

        if (metric == "cosine") {
            std::cout << "score = ";
        } else {
            std::cout << "distance = ";
        }

        std::cout
            << std::fixed
            << std::setprecision(6)
            << results[i].score
            << "\n";
    }

    std::cout << "\n";
}

void printVector(const Vector& vector) {

    const auto& data = vector.data();

    std::cout << "[";

    for (std::size_t i = 0; i < data.size(); ++i) {

        std::cout << data[i];

        if (i + 1 < data.size()) {
            std::cout << ", ";
        }
    }

    std::cout << "]";
}

} // namespace

int main() {

    VectorStore store;

    std::cout
        << "\n============================================\n"
        << "              VectorDB v1.0\n"
        << "============================================\n";

    std::cout
        << "\nInteractive VectorDB shell.\n"
        << "Type 'help' for available commands.\n\n";

    std::string line;

    while (true) {

        std::cout << "vectordb> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::stringstream parser(line);

        std::string command;
        parser >> command;

        // ==================================================
        // EXIT
        // ==================================================

        if (command == "exit" || command == "quit") {

            std::cout << "Goodbye.\n";
            break;
        }

        // ==================================================
        // HELP
        // ==================================================

        if (command == "help") {

            printHelp();
            continue;
        }

        // ==================================================
        // LIST
        // ==================================================

        if (command == "list") {

            std::cout
                << "Vectors stored: "
                << store.size()
                << "\n";

            continue;
        }

        // ==================================================
        // INSERT
        // ==================================================

        if (command == "insert") {

            std::string id;
            parser >> id;

            if (id.empty()) {

                std::cout
                    << "Error: missing vector ID.\n";

                continue;
            }

            std::vector<float> values;

            float value;

            while (parser >> value) {
                values.push_back(value);
            }

            if (values.empty()) {

                std::cout
                    << "Error: vector cannot be empty.\n";

                continue;
            }

            try {

                bool inserted = store.insert({
                    id,
                    Vector(values),
                    ""
                });

                if (inserted) {

                    std::cout
                        << "Inserted "
                        << id
                        << " successfully.\n";

                } else {

                    std::cout
                        << "Insert failed: ID already exists.\n";
                }

            } catch (const std::exception& e) {

                std::cout
                    << "Insert failed: "
                    << e.what()
                    << "\n";
            }

            continue;
        }

        // ==================================================
        // SEARCH
        // ==================================================

        if (command == "search") {

            std::vector<std::string> tokens;

            std::string token;

            while (parser >> token) {
                tokens.push_back(token);
            }

            if (tokens.size() < 3) {

                std::cout
                    << "Usage:\n"
                    << "search <v1> <v2> ... <vn> <k> <metric>\n";

                continue;
            }

            // Last token = metric
            std::string metricString =
                tokens.back();

            tokens.pop_back();

            // Second-last token = k
            std::size_t k;

            try {

                k = std::stoul(tokens.back());

            } catch (...) {

                std::cout
                    << "Error: invalid Top-K value.\n";

                continue;
            }

            tokens.pop_back();

            if (k == 0) {

                std::cout
                    << "Error: Top-K must be greater than 0.\n";

                continue;
            }

            if (tokens.empty()) {

                std::cout
                    << "Error: query vector is empty.\n";

                continue;
            }

            std::vector<float> values;

            try {

                for (const auto& value : tokens) {

                    values.push_back(
                        std::stof(value)
                    );
                }

            } catch (...) {

                std::cout
                    << "Error: invalid vector value.\n";

                continue;
            }

            try {

                DistanceMetric metric =
                    parseMetric(metricString);

                Vector query(values);

                BruteForceIndex index(store);

                auto results =
                    index.search(
                        query,
                        k,
                        metric
                    );

                printResults(
                    results,
                    metricString
                );

            } catch (const std::exception& e) {

                std::cout
                    << "Search failed: "
                    << e.what()
                    << "\n";
            }

            continue;
        }

        // ==================================================
        // GET
        // ==================================================

        if (command == "get") {

            std::string id;
            parser >> id;

            if (id.empty()) {

                std::cout
                    << "Usage: get <id>\n";

                continue;
            }

            const VectorRecord* record =
                store.get(id);

            if (record == nullptr) {

                std::cout
                    << "Vector not found: "
                    << id
                    << "\n";

                continue;
            }

            std::cout
                << "\nID: "
                << record->id
                << "\n";

            std::cout
                << "Vector: ";

            printVector(record->vector);

            std::cout << "\n";

            std::cout
                << "Text: "
                << record->text
                << "\n\n";

            continue;
        }

        // ==================================================
        // DELETE
        // ==================================================

        if (command == "delete") {

            std::string id;
            parser >> id;

            if (id.empty()) {

                std::cout
                    << "Usage: delete <id>\n";

                continue;
            }

            bool removed =
                store.remove(id);

            if (removed) {

                std::cout
                    << "Deleted "
                    << id
                    << " successfully.\n";

            } else {

                std::cout
                    << "Vector not found: "
                    << id
                    << "\n";
            }

            continue;
        }

        // ==================================================
        // SAVE
        // ==================================================

        if (command == "save") {

            std::string filename;
            parser >> filename;

            if (filename.empty()) {

                std::cout
                    << "Usage: save <filename>\n";

                continue;
            }

            if (store.save(filename)) {

                std::cout
                    << "Database saved to "
                    << filename
                    << "\n";

            } else {

                std::cout
                    << "Failed to save database.\n";
            }

            continue;
        }

        // ==================================================
        // LOAD
        // ==================================================

        if (command == "load") {

            std::string filename;
            parser >> filename;

            if (filename.empty()) {

                std::cout
                    << "Usage: load <filename>\n";

                continue;
            }

            if (store.load(filename)) {

                std::cout
                    << "Database loaded from "
                    << filename
                    << "\n";

                std::cout
                    << "Vectors stored: "
                    << store.size()
                    << "\n";

            } else {

                std::cout
                    << "Failed to load database.\n";
            }

            continue;
        }

        // ==================================================
        // UNKNOWN COMMAND
        // ==================================================

        std::cout
            << "Unknown command: "
            << command
            << "\n";

        std::cout
            << "Type 'help' to see available commands.\n";
    }

    return 0;
}
