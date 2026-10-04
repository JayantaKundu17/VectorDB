cmake_minimum_required(VERSION 3.20)

project(VectorDB
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Core library
# --------------------------------------------------

add_library(vectordb_core

    src/core/Vector.cpp
    src/core/VectorStore.cpp

    src/indexes/BruteForce.cpp
    src/indexes/KDTree.cpp
    src/indexes/HNSW.cpp
)

target_include_directories(vectordb_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

# --------------------------------------------------
# Main application
# --------------------------------------------------

add_executable(vectordb
    src/main.cpp
)

target_link_libraries(vectordb
    PRIVATE
        vectordb_core
)

# --------------------------------------------------
# Testing
# --------------------------------------------------

enable_testing()

add_executable(test_vector
    tests/test_vector.cpp
)

target_link_libraries(test_vector
    PRIVATE
        vectordb_core
)

add_test(
    NAME VectorTest
    COMMAND test_vector
)

add_executable(test_distance
    tests/test_distance.cpp
)

target_link_libraries(test_distance
    PRIVATE
        vectordb_core
)

add_test(
    NAME DistanceTest
    COMMAND test_distance
)

add_executable(test_vectorstore
    tests/test_vectorstore.cpp
)

target_link_libraries(test_vectorstore
    PRIVATE
        vectordb_core
)

add_test(
    NAME VectorStoreTest
    COMMAND test_vectorstore
)

add_executable(test_bruteforce
    tests/test_bruteforce.cpp
)

target_link_libraries(test_bruteforce
    PRIVATE
        vectordb_core
)

add_test(
    NAME BruteForceTest
    COMMAND test_bruteforce
)

add_executable(test_kdtree
    tests/test_kdtree.cpp
)

target_link_libraries(test_kdtree
    PRIVATE
        vectordb_core
)

add_test(
    NAME KDTreeTest
    COMMAND test_kdtree
)

add_executable(test_persistence
    tests/test_persistence.cpp
)

target_link_libraries(test_persistence
    PRIVATE
        vectordb_core
)

add_test(
    NAME PersistenceTest
    COMMAND test_persistence
)

# --------------------------------------------------
# Benchmarks
# --------------------------------------------------

add_executable(search_benchmark
    benchmarks/search_benchmark.cpp
)

target_link_libraries(search_benchmark
    PRIVATE
        vectordb_core
)

add_executable(hnsw_tuning
    benchmarks/hnsw_tuning.cpp
)

target_link_libraries(hnsw_tuning
    PRIVATE
        vectordb_core
)

# --------------------------------------------------
# REST API
# --------------------------------------------------

find_package(CURL REQUIRED)

add_executable(vectordb_server
    src/api/main.cpp
    src/api/Server.cpp
    src/api/Server.h
)

target_link_libraries(vectordb_server
    PRIVATE
        vectordb_core
        CURL::libcurl
)

target_include_directories(vectordb_server
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)