# VectorDB

A lightweight vector database implemented in **C++17**, featuring multiple vector-search indexes, persistent storage, interactive CLI access, automated testing, and performance benchmarking.

## Overview

VectorDB is a from-scratch implementation of core concepts used in modern vector databases and approximate nearest-neighbor (ANN) search systems.

The project supports storing high-dimensional vectors and retrieving the most similar vectors using multiple distance metrics and indexing strategies.

### Key Features

* Vector storage with unique IDs
* Insert, update, upsert, retrieve, and delete operations
* Cosine similarity
* Euclidean distance
* Manhattan distance
* Brute-force nearest-neighbor search
* KD-Tree indexing
* HNSW approximate nearest-neighbor indexing
* Top-K search
* Recall@K evaluation
* Database persistence using save/load
* Interactive command-line interface
* Automated CTest test suite
* Search performance benchmarking
* HNSW parameter tuning

---

## Architecture

```text
                    ┌──────────────────────┐
                    │    Interactive CLI   │
                    │      vectordb        │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │     VectorStore      │
                    │                      │
                    │ Insert / Update      │
                    │ Upsert / Delete      │
                    │ Get / Persistence    │
                    └──────────┬───────────┘
                               │
             ┌─────────────────┼─────────────────┐
             │                 │                 │
             ▼                 ▼                 ▼
      ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
      │ Brute Force │   │   KD-Tree   │   │    HNSW     │
      │    Index    │   │    Index    │   │    Index    │
      └─────────────┘   └─────────────┘   └─────────────┘
             │                 │                 │
             └─────────────────┼─────────────────┘
                               ▼
                    ┌──────────────────────┐
                    │   Distance Metrics   │
                    │                      │
                    │ Cosine               │
                    │ Euclidean            │
                    │ Manhattan            │
                    └──────────────────────┘
```

---

## Project Structure

```text
VectorDB/
├── src/
│   ├── core/
│   │   ├── Vector.cpp
│   │   ├── Vector.h
│   │   ├── VectorStore.cpp
│   │   └── VectorStore.h
│   │
│   ├── indexes/
│   │   ├── BruteForce.cpp
│   │   ├── BruteForce.h
│   │   ├── KDTree.cpp
│   │   ├── KDTree.h
│   │   ├── HNSW.cpp
│   │   └── HNSW.h
│   │
│   ├── distance/
│   │   └── DistanceMetric.h
│   │
│   └── main.cpp
│
├── tests/
│   ├── test_vector.cpp
│   ├── test_distance.cpp
│   ├── test_vectorstore.cpp
│   ├── test_bruteforce.cpp
│   ├── test_kdtree.cpp
│   └── test_persistence.cpp
│
├── benchmarks/
│   ├── search_benchmark.cpp
│   └── hnsw_tuning.cpp
│
├── CMakeLists.txt
├── README.md
└── .gitignore
```

---

# Core Components

## Vector

The `Vector` class represents an arbitrary-dimensional numerical vector.

Example:

```text
[1.0, 2.0, 3.0]
```

Vectors can represent embeddings produced by machine-learning models.

---

## VectorStore

`VectorStore` manages the underlying vector records.

Each record contains:

```text
ID
Vector
Text / metadata
```

Example:

```text
doc_001
[1.0, 2.0, 3.0]
"Document about machine learning."
```

Supported operations include:

```text
insert()
update()
upsert()
exists()
get()
remove()
size()
clear()
getAll()
save()
load()
```

The store uses an `unordered_map` for ID-based record management.

---

# Distance Metrics

VectorDB currently supports three distance/similarity measures.

## Cosine Similarity

Measures the angular similarity between two vectors.

A value closer to `1` indicates higher similarity.

Example:

```text
Query: [1, 2, 3]

doc_001 → 1.000000
doc_002 → 0.999859
doc_003 → 0.925820
```

---

## Euclidean Distance

Measures straight-line distance between vectors.

Smaller values indicate greater similarity.

```text
Query: [1, 2, 3]

doc_001 → 0.000000
doc_002 → 0.173205
doc_003 → larger distance
```

---

## Manhattan Distance

Measures the sum of absolute differences between vector dimensions.

Smaller values indicate greater similarity.

---

# Search Indexes

## 1. Brute Force

The brute-force index compares the query against every stored vector.

### Advantages

* Simple
* Exact
* Reliable ground truth
* Useful for benchmarking approximate indexes

### Disadvantage

Search cost increases significantly as the number and dimensionality of vectors grow.

The brute-force implementation is therefore used as the ground-truth reference when calculating recall.

---

## 2. KD-Tree

KD-Tree recursively partitions vectors according to dimensions.

It can reduce the amount of the search space examined compared with a full brute-force scan.

In the benchmark, KD-Tree produced exact results for the tested configurations.

---

## 3. HNSW

HNSW (Hierarchical Navigable Small World) is an approximate nearest-neighbor graph index.

It trades some search accuracy for significantly faster query performance at higher dimensions.

The implementation exposes the major HNSW parameters:

```text
M
efConstruction
efSearch
```

These parameters control graph connectivity, construction effort, and search effort.

---

# Benchmarking

The search benchmark evaluates:

* Brute-force search
* KD-Tree search
* HNSW search
* Query latency
* Build time
* Recall@10
* Speedup

The benchmark was run with:

```text
Vectors : 5000
Top-K   : 10
Queries : 100
```

Dimensions tested:

```text
32
128
384
768
```

## Benchmark Results

### 32 Dimensions

| Index       | Query Latency | Recall@10 | Speedup |
| ----------- | ------------: | --------: | ------: |
| Brute Force |     2.8252 ms |      100% |   1.00x |
| KD-Tree     |     2.2371 ms |      100% |   1.26x |
| HNSW        |     4.2307 ms |      100% |   0.67x |

At 32 dimensions, HNSW does not outperform brute force because the dataset is relatively small and low-dimensional.

### 128 Dimensions

| Index       | Query Latency | Recall@10 | Speedup |
| ----------- | ------------: | --------: | ------: |
| Brute Force |     6.2927 ms |      100% |   1.00x |
| KD-Tree     |     5.8607 ms |      100% |   1.07x |
| HNSW        |     5.1366 ms |       90% |   1.23x |

HNSW begins to provide a measurable query-speed advantage.

### 384 Dimensions

| Index       | Query Latency | Recall@10 | Speedup |
| ----------- | ------------: | --------: | ------: |
| Brute Force |    15.4624 ms |      100% |   1.00x |
| KD-Tree     |    15.1755 ms |      100% |   1.02x |
| HNSW        |     7.1915 ms |       90% |   2.15x |

At 384 dimensions, HNSW provides approximately **2.15× query speedup** over brute force.

### 768 Dimensions

| Index       | Query Latency | Recall@10 | Speedup |
| ----------- | ------------: | --------: | ------: |
| Brute Force |    29.3314 ms |      100% |   1.00x |
| KD-Tree     |    28.9573 ms |      100% |   1.01x |
| HNSW        |    10.1153 ms |       80% |   2.90x |

At 768 dimensions, HNSW provides approximately **2.9× query speedup** over brute force, at the cost of reduced recall.

---

# HNSW Parameter Tuning

A separate benchmark evaluates different HNSW configurations.

Dataset:

```text
Vectors   : 5000
Dimension : 384
Top-K     : 10
Queries   : 50
```

Parameters tested:

```text
M:
8, 16, 32

efConstruction:
100, 200

efSearch:
50, 100, 200
```

One strong configuration observed during testing was:

```text
M = 32
efConstruction = 100
efSearch = 100
```

Results:

```text
Recall@10 : 100%
Query     : ~8.26 ms
Speedup   : ~2.22x
```

Another configuration:

```text
M = 32
efConstruction = 100
efSearch = 50
```

produced:

```text
Recall@10 : 80%
Query     : ~6.93 ms
Speedup   : ~2.72x
```

This demonstrates the fundamental HNSW trade-off:

```text
Higher efSearch
       ↓
Higher recall
       ↓
Higher query latency
```

---

# Persistence

VectorDB supports saving and loading the database.

Example:

```text
vectordb> save database.db
Database saved to database.db

vectordb> delete doc_001
Deleted doc_001 successfully.

vectordb> load database.db
Database loaded from database.db

vectordb> get doc_001
ID: doc_001
Vector: [1.000000, 2.000000, 3.000000]
```

Persistence is covered by an automated test.

---

# Interactive CLI

Start VectorDB with:

```bash
./build/vectordb
```

The shell supports operations such as:

```text
insert
get
delete
list
search
save
load
help
```

Example:

```text
vectordb> insert doc_001 1 2 3
Inserted doc_001 successfully.

vectordb> insert doc_002 1.1 2.1 3.1
Inserted doc_002 successfully.

vectordb> search 1 2 3 3 cosine

Search results:

1. doc_001 | score = 1.000000
2. doc_002 | score = 0.999859
```

---

# Building

Requirements:

* C++17 compiler
* CMake 3.20+
* macOS, Linux, or another C++17-compatible environment

Clone the repository and build:

```bash
cmake -S . -B build
cmake --build build
```

Run the application:

```bash
./build/vectordb
```

---

# Testing

Run the complete test suite:

```bash
ctest --test-dir build --output-on-failure
```

Current test suite:

```text
VectorTest
DistanceTest
VectorStoreTest
BruteForceTest
KDTreeTest
PersistenceTest
```

Current result:

```text
100% tests passed
6/6 tests passed
```

---

# Running Benchmarks

Search benchmark:

```bash
./build/search_benchmark
```

HNSW parameter tuning:

```bash
./build/hnsw_tuning
```

---

# Design Goals

The project focuses on understanding the engineering behind vector databases rather than relying on an external vector database implementation.

The main goals are:

1. Implement vector storage from scratch.
2. Implement multiple nearest-neighbor search strategies.
3. Compare exact and approximate search.
4. Measure query performance and recall.
5. Understand HNSW parameter trade-offs.
6. Add persistent storage.
7. Provide a usable command-line interface.
8. Maintain correctness through automated tests.

---

# Future Improvements

Potential next steps include:

* HNSW-specific unit tests
* Larger-scale benchmarks
* Real embedding datasets
* Batch insertion
* Batch search
* Metadata filtering
* Concurrent queries
* Multithreaded indexing
* Memory-usage benchmarking
* More persistence formats
* REST API
* Python bindings
* Docker deployment
* Improved CLI command parsing
* Production-oriented error handling

---

# Technology Stack

```text
Language       : C++17
Build System   : CMake
Testing        : CTest
Storage        : In-memory unordered_map + file persistence
Indexes        : Brute Force, KD-Tree, HNSW
Distance       : Cosine, Euclidean, Manhattan
Benchmarking   : Custom C++ benchmarks
```

---

## Project Status

**VectorDB v1.0**

The current implementation provides a functional vector-storage and vector-search engine with exact and approximate nearest-neighbor indexes, persistence, automated tests, benchmarking, and an interactive CLI.

