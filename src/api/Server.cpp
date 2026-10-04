#include "api/Server.h"

#include "crow.h"
#include "nlohmann/json.hpp"
#include "distance/DistanceMetric.h"

#include <curl/curl.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;


// ============================================================
// Helper: Add CORS headers
// ============================================================

namespace {

std::string envString(
    const char* name,
    const char* fallback
) {
    const char* value = std::getenv(name);

    if (value && std::string(value).length() > 0) {
        return std::string(value);
    }

    return std::string(fallback);
}

bool useGeminiProvider() {
    return envString("RAG_PROVIDER", "gemini") == "gemini";
}


void addCorsHeaders(crow::response& response) {
    response.set_header(
        "Access-Control-Allow-Origin",
        "*"
    );

    response.set_header(
        "Access-Control-Allow-Methods",
        "GET, POST, PUT, DELETE, OPTIONS"
    );

    response.set_header(
        "Access-Control-Allow-Headers",
        "Content-Type"
    );
}


// ============================================================
// Ollama HTTP Helpers
// ============================================================

size_t curlWriteCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp
) {
    const size_t totalSize =
        size * nmemb;

    std::string* output =
        static_cast<std::string*>(userp);

    output->append(
        static_cast<char*>(contents),
        totalSize
    );

    return totalSize;
}


std::string ollamaPost(
    const std::string& endpoint,
    const json& payload
) {
    CURL* curl =
        curl_easy_init();

    if (!curl) {
        throw std::runtime_error(
            "Failed to initialize libcurl"
        );
    }

    std::string response;

    const std::string requestBody =
        payload.dump();

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    // Optional Ollama Cloud authentication.
    // Local Ollama does not require an API key.
    const char* apiKey =
        std::getenv("OLLAMA_API_KEY");

    if (apiKey && std::string(apiKey).length() > 0) {
        const std::string authHeader =
            "Authorization: Bearer " +
            std::string(apiKey);

        headers = curl_slist_append(
            headers,
            authHeader.c_str()
        );
    }

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        endpoint.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        requestBody.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        curlWriteCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        120L
    );

    CURLcode result =
        curl_easy_perform(curl);

    long httpStatus = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpStatus
    );

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(
            std::string(
                "Ollama request failed: "
            ) +
            curl_easy_strerror(result)
        );
    }

    if (
        httpStatus < 200 ||
        httpStatus >= 300
    ) {
        throw std::runtime_error(
            "Ollama returned HTTP " +
            std::to_string(httpStatus) +
            ": " +
            response
        );
    }

    return response;
}



// ============================================================
// Gemini HTTP Helper
// ============================================================

std::string geminiPost(
    const std::string& endpoint,
    const json& payload
) {
    CURL* curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error(
            "Failed to initialize libcurl"
        );
    }

    std::string response;

    const std::string requestBody =
        payload.dump();

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    const std::string apiKey =
        envString("GEMINI_API_KEY", "");

    if (apiKey.empty()) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        throw std::runtime_error(
            "GEMINI_API_KEY is not configured"
        );
    }

    headers = curl_slist_append(
        headers,
        ("x-goog-api-key: " + apiKey).c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        endpoint.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        requestBody.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        curlWriteCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        120L
    );

    CURLcode result =
        curl_easy_perform(curl);

    long httpStatus = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpStatus
    );

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(
            std::string(
                "Gemini request failed: "
            ) +
            curl_easy_strerror(result)
        );
    }

    if (
        httpStatus < 200 ||
        httpStatus >= 300
    ) {
        throw std::runtime_error(
            "Gemini returned HTTP " +
            std::to_string(httpStatus) +
            ": " +
            response
        );
    }

    return response;
}


// ============================================================
// Generate Embedding
// ============================================================

std::vector<float> generateEmbedding(
    const std::string& text
) {
    // --------------------------------------------------------
    // Gemini production provider
    // --------------------------------------------------------

    if (useGeminiProvider()) {

        json payload;

        payload["content"] = {
            {"parts", {
                {"text", text}
            }}
        };

        payload["output_dimensionality"] = 768;

        const std::string model =
            envString(
                "GEMINI_EMBED_MODEL",
                "gemini-embedding-2"
            );

        const std::string endpoint =
            "https://generativelanguage.googleapis.com/"
            "v1beta/models/" +
            model +
            ":embedContent";

        const std::string raw =
            geminiPost(
                endpoint,
                payload
            );

        json result =
            json::parse(raw);

        if (
            !result.contains("embedding") ||
            !result["embedding"].contains("values")
        ) {
            throw std::runtime_error(
                "Gemini embedding response does not "
                "contain embedding values"
            );
        }

        std::vector<float> embedding =
            result["embedding"]["values"]
                .get<std::vector<float>>();

        if (embedding.size() != 768) {
            throw std::runtime_error(
                "Gemini embedding dimension is " +
                std::to_string(embedding.size()) +
                ", expected 768"
            );
        }

        return embedding;
    }

    // --------------------------------------------------------
    // Existing Ollama local provider
    // --------------------------------------------------------

    json payload;

    payload["model"] =
        "nomic-embed-text";

    payload["input"] =
        text;

    const std::string ollamaBaseUrl =
        envString(
            "OLLAMA_BASE_URL",
            "http://localhost:11434"
        );

    const std::string raw =
        ollamaPost(
            ollamaBaseUrl + "/api/embed",
            payload
        );

    json result =
        json::parse(raw);

    if (
        !result.contains("embeddings") ||
        result["embeddings"].empty()
    ) {
        throw std::runtime_error(
            "Ollama embedding response "
            "does not contain embeddings"
        );
    }

    return result["embeddings"][0]
        .get<std::vector<float>>();
}


// ============================================================
// Generate LLM Answer
// ============================================================

std::string generateAnswer(
    const std::string& question,
    const std::string& context
) {
    std::ostringstream prompt;

    prompt
        << "You are answering a question using "
           "the provided context.\n\n"

        << "Use only information supported "
           "by the context.\n"

        << "Do not invent facts.\n"

        << "If the answer cannot be found in "
           "the context, say that the information "
           "is not available in the provided "
           "context.\n\n"

        << "Context:\n"
        << context
        << "\n\n"

        << "Question:\n"
        << question
        << "\n\n"

        << "Answer:\n";

    // --------------------------------------------------------
    // Gemini production provider
    // --------------------------------------------------------

    if (useGeminiProvider()) {

        json payload;

        payload["contents"] = {
            {
                {"parts", {
                    {
                        {"text", prompt.str()}
                    }
                }}
            }
        };

        const std::string model =
            envString(
                "GEMINI_MODEL",
                "gemini-3.5-flash-lite"
            );

        const std::string endpoint =
            "https://generativelanguage.googleapis.com/"
            "v1beta/models/" +
            model +
            ":generateContent";

        const std::string raw =
            geminiPost(
                endpoint,
                payload
            );

        json result =
            json::parse(raw);

        if (
            !result.contains("candidates") ||
            result["candidates"].empty() ||
            !result["candidates"][0]
                .contains("content") ||
            !result["candidates"][0]["content"]
                .contains("parts") ||
            result["candidates"][0]["content"]["parts"]
                .empty()
        ) {
            throw std::runtime_error(
                "Gemini generation response does not "
                "contain answer content"
            );
        }

        return result["candidates"][0]
            ["content"]["parts"][0]
            ["text"]
            .get<std::string>();
    }

    // --------------------------------------------------------
    // Existing Ollama provider
    // --------------------------------------------------------

    json payload;

    payload["model"] =
        envString(
            "OLLAMA_MODEL",
            "llama3.2"
        );

    payload["prompt"] =
        prompt.str();

    payload["stream"] =
        false;

    const std::string ollamaBaseUrl =
        envString(
            "OLLAMA_BASE_URL",
            "http://localhost:11434"
        );

    const std::string raw =
        ollamaPost(
            ollamaBaseUrl + "/api/generate",
            payload
        );

    json result =
        json::parse(raw);

    if (!result.contains("response")) {
        throw std::runtime_error(
            "Ollama generation response "
            "does not contain response"
        );
    }

    return result["response"]
        .get<std::string>();
}


} // namespace


// ============================================================
// Server Constructor
// ============================================================

Server::Server()
    : databasePath(
        std::getenv("VECTORDB_DATA_PATH")
            ? std::getenv("VECTORDB_DATA_PATH")
            : "vectordb.db"
      ),
      store(databasePath),
      index(store) {


    // --------------------------------------------------------
    // Create persistence directory if necessary
    // --------------------------------------------------------

    std::filesystem::path path(
        databasePath
    );


    if (!path.parent_path().empty()) {

        std::error_code ec;

        std::filesystem::create_directories(
            path.parent_path(),
            ec
        );

        if (ec) {

            std::cerr
                << "Warning: could not create "
                   "persistence directory: "
                << ec.message()
                << "\n";
        }
    }


    // --------------------------------------------------------
    // Load existing database
    // --------------------------------------------------------

    if (
        std::filesystem::exists(
            databasePath
        )
    ) {

        if (!store.load()) {

            std::cerr
                << "Warning: failed to load "
                   "database from "
                << databasePath
                << "\n";
        }
        else {

            std::cout
                << "VectorDB loaded "
                << store.size()
                << " vectors from "
                << databasePath
                << "\n";
        }

    }
    else {

        std::cout
            << "No existing database found. "
               "Starting empty.\n";
    }


    // --------------------------------------------------------
    // Build HNSW index
    // --------------------------------------------------------

    index.build();

    std::cout
        << "HNSW index built with "
        << index.size()
        << " vectors\n";
}


// ============================================================
// Server
// ============================================================

void Server::run(int port) {

    crow::SimpleApp app;


    // ========================================================
    // CORS Preflight
    // ========================================================
    //
    // IMPORTANT:
    // "/<path>" passes a string path parameter.
    // Therefore this lambda MUST accept:
    //
    // request
    // response
    // path
    //
    // ========================================================

    CROW_ROUTE(app, "/<path>")
        .methods(crow::HTTPMethod::OPTIONS)

    ([](
        const crow::request&,
        crow::response& response,
        const std::string&
    ) {

        addCorsHeaders(response);

        response.code = 204;

        response.end();
    });


    // ========================================================
    // Health
    // GET /health
    // ========================================================

    CROW_ROUTE(app, "/health")

    ([] {

        json result;

        result["service"] =
            "VectorDB";

        result["status"] =
            "ok";


        crow::response response(
            200,
            result.dump()
        );


        response.set_header(
            "Content-Type",
            "application/json"
        );

        addCorsHeaders(response);


        return response;
    });


    // ========================================================
    // Stats
    // GET /stats
    // ========================================================

    CROW_ROUTE(app, "/stats")

    ([this] {

        json result;

        result["vectors"] =
            store.size();

        result["index"] =
            "HNSW";

        result["index_size"] =
            index.size();


        crow::response response(
            200,
            result.dump()
        );


        response.set_header(
            "Content-Type",
            "application/json"
        );

        addCorsHeaders(response);


        return response;
    });


    // ========================================================
    // Vector Visualization
    // GET /visualization/vectors?limit=500
    // ========================================================

    CROW_ROUTE(app, "/visualization/vectors")
        .methods(crow::HTTPMethod::GET)
    ([this](const crow::request& request) {

        try {
            std::size_t limit = 500;

            const char* limitParam =
                request.url_params.get("limit");

            if (limitParam != nullptr) {
                try {
                    limit = static_cast<std::size_t>(
                        std::stoul(limitParam)
                    );
                } catch (...) {
                    limit = 500;
                }
            }

            if (limit < 2) limit = 2;
            if (limit > 750) limit = 750;

            json output;
            output["count"] = store.size();
            output["sample_size"] = 0;
            output["dimension"] = 0;
            output["vectors"] = json::array();

            std::size_t dimension = 0;
            std::size_t count = 0;

            for (const auto& entry : store.getAll()) {
                const auto& record = entry.second;

                if (record.vector.dimension() == 0) {
                    continue;
                }

                if (dimension == 0) {
                    dimension = record.vector.dimension();
                }

                if (record.vector.dimension() != dimension) {
                    continue;
                }

                json item;
                item["id"] = record.id;
                item["dimension"] = record.vector.dimension();
                item["vector"] = record.vector.data();

                if (!record.text.empty()) {
                    item["text"] = record.text;
                }

                output["vectors"].push_back(item);
                ++count;

                if (count >= limit) {
                    break;
                }
            }

            output["sample_size"] = count;
            output["dimension"] = dimension;

            crow::response response(
                200,
                output.dump()
            );

            response.set_header(
                "Content-Type",
                "application/json"
            );

            addCorsHeaders(response);
            return response;
        }
        catch (const std::exception& e) {
            json error;
            error["error"] = e.what();

            crow::response response(
                500,
                error.dump()
            );

            addCorsHeaders(response);
            return response;
        }
    });


    // ========================================================
    // Insert Vector
    // POST /vectors
    // ========================================================

    CROW_ROUTE(app, "/vectors")
        .methods(crow::HTTPMethod::POST)

    ([this](const crow::request& request) {

        try {

            json body =
                json::parse(request.body);


            // ------------------------------------------------
            // Validate
            // ------------------------------------------------

            if (
                !body.contains("id") ||
                !body.contains("vector")
            ) {

                crow::response response(
                    400,
                    R"({"error":"id and vector are required"})"
                );

                addCorsHeaders(response);

                return response;
            }


            std::string id =
                body["id"]
                    .get<std::string>();


            std::vector<float> values =
                body["vector"]
                    .get<std::vector<float>>();


            if (id.empty()) {

                crow::response response(
                    400,
                    R"({"error":"id cannot be empty"})"
                );

                addCorsHeaders(response);

                return response;
            }


            if (values.empty()) {

                crow::response response(
                    400,
                    R"({"error":"vector cannot be empty"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // Optional text metadata
            // ------------------------------------------------

            std::string text;


            if (
                body.contains("text") &&
                !body["text"].is_null()
            ) {

                text =
                    body["text"]
                        .get<std::string>();
            }


            // ------------------------------------------------
            // Create VectorRecord
            // ------------------------------------------------

            VectorRecord record;

            record.id =
                id;

            record.vector =
                Vector(
                    std::move(values)
                );

            record.text =
                text;


            // ------------------------------------------------
            // Store vector
            // ------------------------------------------------

            if (!store.upsert(record)) {

                crow::response response(
                    500,
                    R"({"error":"failed to store vector"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // Persist
            // ------------------------------------------------

            if (!store.save()) {

                std::cerr
                    << "Warning: failed to save "
                       "database\n";
            }


            // ------------------------------------------------
            // Rebuild HNSW
            // ------------------------------------------------

            index.build();


            // ------------------------------------------------
            // Response
            // ------------------------------------------------

            json result;

            result["status"] =
                "ok";

            result["id"] =
                id;

            result["dimension"] =
                record.vector.dimension();


            crow::response response(
                200,
                result.dump()
            );


            response.set_header(
                "Content-Type",
                "application/json"
            );

            addCorsHeaders(response);


            return response;
        }


        catch (const std::exception& e) {

            json error;

            error["error"] =
                e.what();


            crow::response response(
                400,
                error.dump()
            );

            addCorsHeaders(response);

            return response;
        }
    });


    // ========================================================
    // Get Vector
    // GET /vectors/<id>
    // ========================================================

    CROW_ROUTE(app, "/vectors/<string>")

    ([this](const std::string& id) {

        const VectorRecord* record =
            store.get(id);


        if (!record) {

            crow::response response(
                404,
                R"({"error":"vector not found"})"
            );

            addCorsHeaders(response);

            return response;
        }


        json result;

        result["id"] =
            record->id;

        result["vector"] =
            record->vector.data();

        result["text"] =
            record->text;


        crow::response response(
            200,
            result.dump()
        );


        response.set_header(
            "Content-Type",
            "application/json"
        );

        addCorsHeaders(response);


        return response;
    });


    // ========================================================
    // Search
    // POST /search
    // ========================================================

    CROW_ROUTE(app, "/search")
        .methods(crow::HTTPMethod::POST)

    ([this](const crow::request& request) {

        try {

            json body =
                json::parse(request.body);


            // ------------------------------------------------
            // Validate vector
            // ------------------------------------------------

            if (!body.contains("vector")) {

                crow::response response(
                    400,
                    R"({"error":"vector is required"})"
                );

                addCorsHeaders(response);

                return response;
            }


            std::vector<float> values =
                body["vector"]
                    .get<std::vector<float>>();


            if (values.empty()) {

                crow::response response(
                    400,
                    R"({"error":"vector cannot be empty"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // K
            // ------------------------------------------------

            std::size_t k =
                body.value(
                    "k",
                    static_cast<std::size_t>(5)
                );


            if (k == 0) {

                crow::response response(
                    400,
                    R"({"error":"k must be greater than 0"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // Distance metric
            // ------------------------------------------------

            std::string metricName =
                body.value(
                    "metric",
                    "cosine"
                );


            DistanceMetric metric;


            if (metricName == "cosine") {

                metric =
                    DistanceMetric::COSINE;
            }

            else if (
                metricName == "euclidean"
            ) {

                metric =
                    DistanceMetric::EUCLIDEAN;
            }

            else if (
                metricName == "manhattan"
            ) {

                metric =
                    DistanceMetric::MANHATTAN;
            }

            else {

                crow::response response(
                    400,
                    R"({"error":"unsupported metric"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // Query vector
            // ------------------------------------------------

            Vector query(
                std::move(values)
            );


            // ------------------------------------------------
            // HNSW search
            // ------------------------------------------------

            auto results =
                index.search(
                    query,
                    k,
                    metric
                );


            // ------------------------------------------------
            // Build response
            // ------------------------------------------------

            json output;

            output["results"] =
                json::array();


            for (
                const auto& result :
                results
            ) {

                json item;

                item["id"] =
                    result.id;

                item["score"] =
                    result.score;


                // Include stored text when available.
                const VectorRecord* record =
                    store.get(result.id);

                if (
                    record &&
                    !record->text.empty()
                ) {

                    item["text"] =
                        record->text;
                }


                output["results"]
                    .push_back(item);
            }


            crow::response response(
                200,
                output.dump()
            );


            response.set_header(
                "Content-Type",
                "application/json"
            );

            addCorsHeaders(response);


            return response;
        }


        catch (const std::exception& e) {

            json error;

            error["error"] =
                e.what();


            crow::response response(
                400,
                error.dump()
            );

            addCorsHeaders(response);

            return response;
        }
    });


    // ========================================================
    // RAG
    // POST /rag
    //
    // Request:
    //
    // {
    //     "question": "What is a vector database?",
    //     "k": 3
    // }
    //
    // ========================================================

    CROW_ROUTE(app, "/rag")
        .methods(crow::HTTPMethod::POST)

    ([this](const crow::request& request) {

        try {

            // ------------------------------------------------
            // Parse request
            // ------------------------------------------------

            json body =
                json::parse(request.body);


            // ------------------------------------------------
            // Validate question
            // ------------------------------------------------

            if (!body.contains("question")) {

                crow::response response(
                    400,
                    R"({"error":"question is required"})"
                );

                addCorsHeaders(response);

                return response;
            }


            std::string question =
                body["question"]
                    .get<std::string>();


            if (question.empty()) {

                crow::response response(
                    400,
                    R"({"error":"question cannot be empty"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // Top-K
            // ------------------------------------------------

            std::size_t k =
                body.value(
                    "k",
                    static_cast<std::size_t>(3)
                );


            if (k == 0) {

                crow::response response(
                    400,
                    R"({"error":"k must be greater than 0"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // Check database
            // ------------------------------------------------

            if (store.size() == 0) {

                crow::response response(
                    404,
                    R"({"error":"database is empty"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // 1. Generate query embedding
            // ------------------------------------------------

            std::cout
                << "RAG: generating query embedding...\n";


            std::vector<float> queryValues =
                generateEmbedding(
                    question
                );


            if (queryValues.empty()) {

                crow::response response(
                    500,
                    R"({"error":"embedding generation returned an empty vector"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // 2. Validate dimension
            // ------------------------------------------------

            std::size_t databaseDimension =
                0;


            for (
                const auto& entry :
                store.getAll()
            ) {

                databaseDimension =
                    entry.second
                        .vector
                        .dimension();

                break;
            }


            if (
                databaseDimension !=
                queryValues.size()
            ) {

                json error;

                error["error"] =
                    "query embedding dimension does not match database";

                error["query_dimension"] =
                    queryValues.size();

                error["database_dimension"] =
                    databaseDimension;


                crow::response response(
                    400,
                    error.dump()
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // 3. Search HNSW
            // ------------------------------------------------

            std::cout
                << "RAG: searching HNSW...\n";


            Vector query(
                std::move(queryValues)
            );


            auto results =
                index.search(
                    query,
                    k,
                    DistanceMetric::COSINE
                );


            if (results.empty()) {

                crow::response response(
                    404,
                    R"({"error":"no relevant documents found"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // 4. Retrieve document chunks
            // ------------------------------------------------

            std::ostringstream context;

            json sources =
                json::array();


            std::size_t sourceNumber =
                1;


            for (
                const auto& result :
                results
            ) {

                const VectorRecord* record =
                    store.get(result.id);


                if (record == nullptr) {
                    continue;
                }


                if (record->text.empty()) {
                    continue;
                }


                // Add retrieved text to LLM context.

                context
                    << "[Source "
                    << sourceNumber
                    << "]\n"

                    << record->text
                    << "\n\n";


                // Add source metadata.

                json source;

                source["id"] =
                    record->id;

                source["score"] =
                    result.score;

                source["text"] =
                    record->text;


                sources.push_back(
                    source
                );


                sourceNumber++;
            }


            if (sources.empty()) {

                crow::response response(
                    404,
                    R"({"error":"search results found, but no document text was available"})"
                );

                addCorsHeaders(response);

                return response;
            }


            // ------------------------------------------------
            // 5. Generate answer with llama3.2
            // ------------------------------------------------

            std::cout
                << "RAG: generating answer...\n";


            std::string answer =
                generateAnswer(
                    question,
                    context.str()
                );


            // ------------------------------------------------
            // 6. Build final response
            // ------------------------------------------------

            json output;


            output["question"] =
                question;


            // Return the query embedding so the frontend can plot
            // every question as a separate blue point.
            output["query_vector"] =
                query.data();


            output["answer"] =
                answer;


            output["sources"] =
                sources;


            if (useGeminiProvider()) {
    output["model"] =
        envString(
            "GEMINI_MODEL",
            "gemini-3.5-flash-lite"
        );

    output["embedding_model"] =
        envString(
            "GEMINI_EMBED_MODEL",
            "gemini-embedding-2"
        );
} else {
    output["model"] =
        envString(
            "OLLAMA_MODEL",
            "llama3.2"
        );

    output["embedding_model"] =
        "nomic-embed-text";
}


            output["index"] =
                "HNSW";


            crow::response response(
                200,
                output.dump()
            );


            response.set_header(
                "Content-Type",
                "application/json"
            );

            addCorsHeaders(response);


            return response;
        }


        catch (const std::exception& e) {

            std::cerr
                << "RAG error: "
                << e.what()
                << "\n";


            json error;

            error["error"] =
                e.what();


            crow::response response(
                500,
                error.dump()
            );

            addCorsHeaders(response);

            return response;
        }
    });


    // ========================================================
    // Start Server
    // ========================================================

    std::cout
        << "VectorDB API listening on port "
        << port
        << "\n";


    app
        .port(port)
        .bindaddr("0.0.0.0")
        .multithreaded()
        .run();
}