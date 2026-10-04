# VectorDB — C++ Vector Database & RAG Engine

 A vector database engine built from scratch in **C++17**, featuring **Brute Force, KD-Tree, and HNSW** similarity search, persistent vector storage, a REST API, **Gemini-powered embeddings and generation**, and an interactive web interface with **PCA vector visualization**.

---

## Live Demo

**Frontend:**  
https://vectordb-frontend.onrender.com

**Backend API:**  
https://vectordb-api-yxup.onrender.com

---

## Screenshots


<img width="1461" height="797" alt="Screenshot 2026-10-04 at 8 18 30 AM" src="https://github.com/user-attachments/assets/36b43aad-9d6a-4dfc-bbfb-866befdb9349" />


<img width="1456" height="798" alt="Screenshot 2026-10-04 at 8 18 37 AM" src="https://github.com/user-attachments/assets/95c612b1-a664-42bb-ab14-d89f54fdc4d0" />


<img width="1463" height="797" alt="Screenshot 2026-10-04 at 8 18 44 AM" src="https://github.com/user-attachments/assets/0152b7f7-328a-4304-ad0b-ade71be39c07" />



---

## Overview

VectorDB is a lightweight vector database and Retrieval-Augmented Generation (RAG) system implemented in C++17.

The project was designed to understand what happens underneath modern vector-search systems instead of relying entirely on an existing vector database.

The engine provides:

- High-dimensional vector storage
- Multiple distance metrics
- Multiple nearest-neighbor search algorithms
- HNSW approximate nearest-neighbor indexing
- File-based persistence
- REST API endpoints
- Gemini-based document embeddings
- Gemini-based answer generation
- End-to-end RAG retrieval
- PCA-based 2D vector visualization
- Search benchmarking
- Docker deployment
- Public cloud deployment using Render

---

# Architecture

```text
                        ┌─────────────────────┐
                        │     Web Browser      │
                        │ HTML / CSS / JS      │
                        └──────────┬──────────┘
                                   │
                                   ▼
                        ┌─────────────────────┐
                        │   Render Frontend    │
                        │   Static Site        │
                        └──────────┬──────────┘
                                   │ HTTP
                                   ▼
┌─────────────────────────────────────────────────────────┐
│                VectorDB REST API                        │
│                  C++ / Crow                             │
├─────────────────────────────────────────────────────────┤
│                                                         │
│   Query                                                  │
│     │                                                    │
│     ▼                                                    │
│   Gemini Embedding                                       │
│     │                                                    │
│     ▼                                                    │
│   HNSW Similarity Search                                  │
│     │                                                    │
│     ▼                                                    │
│   Top-K Relevant Chunks                                  │
│     │                                                    │
│     ▼                                                    │
│   Context Construction                                   │
│     │                                                    │
│     ▼                                                    │
│   Gemini LLM Generation                                  │
│     │                                                    │
│     ▼                                                    │
│   Grounded Answer + Sources                              │
│                                                         │
└─────────────────────────────────────────────────────────┘
                                   │
                                   ▼
                          ┌─────────────────┐
                          │ Gemini API      │
                          │ Embeddings      │
                          │ + Generation    │
                          └─────────────────┘
```

---

# Key Features

## 1. Vector Storage

The core `VectorStore` manages high-dimensional vectors together with:

- Unique identifiers
- Vector data
- Associated text / metadata
- Persistence to disk

The database can be loaded when the server starts and saved back to disk.

---

## 2. Multiple Search Algorithms

VectorDB implements three different similarity-search strategies.

### Brute Force

Compares the query vector against every stored vector.

```text
Query
  │
  ├── compare → Vector 1
  ├── compare → Vector 2
  ├── compare → Vector 3
  ├── ...
  └── compare → Vector N
```

Advantages:

- Simple
- Exact
- Good baseline for benchmarking

Complexity:

```text
O(N × D)
```

where:

- `N` = number of vectors
- `D` = vector dimensionality

---

### KD-Tree

A tree-based nearest-neighbor structure designed primarily for lower-dimensional workloads.

KD-Tree is useful as a comparison point but becomes less attractive as dimensionality increases.

---

### HNSW

VectorDB implements **Hierarchical Navigable Small World (HNSW)** graph search.

```text
Level 2          A -------- D
                  \        /
                   \      /
Level 1       A --- B --- D ---- F
                \   |   /
                 \  |  /
Level 0      A -- B -- C -- D -- E -- F
```

Higher layers provide sparse long-range connections while lower layers contain denser local connections.

During search:

1. Start from an entry point at a high level.
2. Move toward increasingly similar nodes.
3. Descend through the hierarchy.
4. Perform a denser search at the lowest level.
5. Return the nearest candidates.

This allows approximate nearest-neighbor search without scanning the entire database.

---

# Distance Metrics

VectorDB supports multiple distance functions:

### Cosine Similarity

Measures the angle between vectors.

Useful for semantic embeddings and text similarity.

### Euclidean Distance

Measures straight-line distance between vectors.

### Manhattan Distance

Measures the sum of absolute coordinate differences.

```text
Supported:

COSINE
EUCLIDEAN
MANHATTAN
```

---

# RAG Pipeline

The project includes a complete Retrieval-Augmented Generation pipeline.

```text
Document
   │
   ▼
Chunking
   │
   ▼
Gemini Embedding
   │
   ▼
768-dimensional vector
   │
   ▼
VectorDB
   │
   ▼
HNSW Index
   │
   ▼
User Question
   │
   ▼
Gemini Embedding
   │
   ▼
HNSW Top-K Search
   │
   ▼
Relevant Documents
   │
   ▼
Context Construction
   │
   ▼
Gemini LLM
   │
   ▼
Grounded Answer
```

The ingestion pipeline:

- Reads `.txt` documents
- Splits them into chunks
- Generates Gemini embeddings
- Stores vectors and source text
- Uses deterministic chunk IDs

The current ingestion configuration uses:

```text
Embedding model : gemini-embedding-2
Dimension       : 768
Chunk size      : 120 words
Chunk overlap   : 25 words
```

---

# Gemini Integration

VectorDB uses Gemini for both sides of the RAG pipeline.

### Embeddings

```text
gemini-embedding-2
```

The embedding pipeline produces:

```text
768-dimensional vectors
```

### Generation

```text
gemini-3.5-flash-lite
```

The LLM receives the retrieved document context and generates a grounded response.

---

# REST API

The C++ backend exposes the vector database through HTTP endpoints using **Crow**.

## Health

```http
GET /health
```

Example:

```json
{
  "service": "VectorDB",
  "status": "ok"
}
```

---

## Statistics

```http
GET /stats
```

Example:

```json
{
  "index": "HNSW",
  "index_size": 6,
  "vectors": 6
}
```

---

## Vector Visualization

```http
GET /visualization/vectors?limit=500
```

Returns vectors and associated metadata for visualization.

The frontend applies PCA in JavaScript to project high-dimensional vectors into 2D.

---

## RAG Query

```http
POST /rag
```

Example request:

```json
{
  "question": "What does HNSW stand for and how does it work?",
  "k": 3
}
```

Example response structure:

```json
{
  "question": "What does HNSW stand for and how does it work?",
  "answer": "...",
  "query_vector": [...],
  "sources": [...],
  "model": "gemini-3.5-flash-lite",
  "embedding_model": "gemini-embedding-2",
  "index": "HNSW"
}
```

---

# Web Interface

The frontend provides three main views.

## RAG Search

Ask questions against the indexed knowledge base.

Displays:

- Generated answer
- Retrieved documents
- Similarity scores
- Embedding model
- Search index
- Generation model

---

## Vector Database

Provides a live view of the database.

Displays:

- Number of vectors
- HNSW node count
- Index type
- Embedding model
- Search metric
- HNSW configuration
- RAG pipeline
- Search benchmark results
- PCA vector space

---

## System

Displays the active system architecture and configuration.

```text
Core Engine       → C++17
Vector Index      → HNSW
Embedding Model   → gemini-embedding-2
Generation Model  → gemini-3.5-flash-lite
Search Metric     → Cosine
REST API          → Crow
```

---

# PCA Visualization

The frontend includes a 2D PCA visualization of the high-dimensional vector space.

```text
768 dimensions
      │
      ▼
     PCA
      │
      ▼
  2D projection
      │
      ▼
Interactive vector plot
```

This makes it possible to visually inspect how documents are distributed in embedding space.

Users can click points to inspect the corresponding vector/document.

---

# Benchmarking

The project includes a benchmark comparing:

- Brute Force
- KD-Tree
- HNSW

The benchmark evaluates different vector dimensions and measures:

- Search latency
- HNSW recall
- Relative speedup

Example local benchmark results from the current project:

| Dimension | Brute Force | KD-Tree | HNSW | HNSW Recall | HNSW Speedup |
|-----------|------------:|--------:|-----:|------------:|-------------:|
| 32D  | 2.8398 ms | 2.2359 ms | 4.2515 ms | 100% | 0.668× |
| 128D | 6.3158 ms | 5.8483 ms | 5.1391 ms | 90% | 1.229× |
| 384D | 16.6928 ms | 17.1806 ms | 9.3564 ms | 90% | 1.784× |
| 768D | 29.4938 ms | 29.0934 ms | 10.2078 ms | 80% | 2.889× |

The benchmark demonstrates the increasing advantage of HNSW as vector dimensionality increases.

> Benchmark results depend on hardware, dataset size, index parameters, and workload. The values above are from the project's local benchmark configuration.

---

# Technology Stack

## Backend

- C++17
- Crow 1.3.4
- CMake
- libcurl
- nlohmann/json

## Vector Search

- Brute Force
- KD-Tree
- HNSW

## AI / RAG

- Gemini Embeddings
- Gemini LLM
- Retrieval-Augmented Generation

## Frontend

- HTML5
- CSS3
- Vanilla JavaScript
- PCA visualization

## Infrastructure

- Docker
- GitHub
- Render

---

# Project Structure

```text
VectorDB/
│
├── benchmarks/
│   ├── hnsw_tuning.cpp
│   └── search_benchmark.cpp
│
├── data/
│   └── rag_docs/
│       ├── cpp_engine.txt
│       ├── embedding_models.txt
│       ├── hnsw.txt
│       ├── rag.txt
│       └── vector_database.txt
│
├── frontend/
│   ├── index.html
│   ├── app.js
│   └── style.css
│
├── scripts/
│   └── ingest.py
│
├── src/
│   ├── api/
│   │   ├── main.cpp
│   │   ├── Server.cpp
│   │   └── Server.h
│   │
│   ├── core/
│   │   ├── Vector.cpp
│   │   ├── Vector.h
│   │   ├── VectorStore.cpp
│   │   └── VectorStore.h
│   │
│   └── indexes/
│       ├── BruteForce.cpp
│       ├── BruteForce.h
│       ├── KDTree.cpp
│       ├── KDTree.h
│       ├── HNSW.cpp
│       └── HNSW.h
│
├── tests/
│   ├── test_vector.cpp
│   ├── test_distance.cpp
│   ├── test_vectorstore.cpp
│   ├── test_bruteforce.cpp
│   ├── test_kdtree.cpp
│   └── test_persistence.cpp
│
├── .dockerignore
├── .env.example
├── .gitignore
├── CMakeLists.txt
├── Dockerfile
└── README.md
```

---

# Local Setup

## Requirements

Install:

- C++17 compiler
- CMake 3.20+
- Git
- Python 3
- Docker
- Gemini API key

On macOS:

```bash
brew install cmake
brew install curl
```

---

## Clone

```bash
git clone https://github.com/JayantaKundu17/VectorDB.git
cd VectorDB
```

---

# Build

```bash
cmake -S . -B build
cmake --build build -j2
```

The REST API executable will be created at:

```text
build/vectordb_server
```

---

# Run Locally

Set the environment variables:

```bash
export RAG_PROVIDER=gemini
export GEMINI_API_KEY="YOUR_API_KEY"
export GEMINI_EMBED_MODEL=gemini-embedding-2
export GEMINI_MODEL=gemini-3.5-flash-lite
export VECTORDB_DATA_PATH=./vectordb.db
```

Start the server:

```bash
./build/vectordb_server
```

The local API runs on:

```text
http://localhost:8080
```

---

# Ingest Documents

The RAG ingestion script reads files from:

```text
data/rag_docs/
```

Run:

```bash
python3 scripts/ingest.py
```

The script generates Gemini embeddings and stores the resulting vectors in VectorDB.

---

# Run the Frontend Locally

From the project root:

```bash
python3 -m http.server 5500 --directory frontend
```

Open:

```text
http://localhost:5500
```

---

# Docker

Build the image:

```bash
docker build -t vectordb .
```

Run:

```bash
docker run \
  -p 8080:10000 \
  -e PORT=10000 \
  -e RAG_PROVIDER=gemini \
  -e GEMINI_API_KEY="$GEMINI_API_KEY" \
  -e GEMINI_EMBED_MODEL=gemini-embedding-2 \
  -e GEMINI_MODEL=gemini-3.5-flash-lite \
  -e VECTORDB_DATA_PATH=/app/data/vectordb.db \
  vectordb
```

Check:

```bash
curl http://localhost:8080/health
```

---

# Running Tests

Configure and build:

```bash
cmake -S . -B build
cmake --build build -j2
```

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

---

# Security

Never commit API keys or secrets.

Use environment variables:

```text
GEMINI_API_KEY
```

A template is provided in:

```text
.env.example
```

The actual `.env` file is ignored by Git.

---

# Deployment

The project is deployed using two Render services.

### Backend

```text
Docker Web Service
```

The backend runs the C++ REST API and connects to Gemini.

### Frontend

```text
Render Static Site
```

The frontend communicates with the deployed C++ backend over HTTPS.

## Production Architecture

```text
GitHub
   │
   ├── Render Static Site
   │       │
   │       ▼
   │   Web Interface
   │       │
   │       ▼
   │   C++ REST API
   │       │
   │       ├── VectorStore
   │       ├── HNSW
   │       └── Gemini API
   │
   └── Dockerized Backend
```

---

# Current Deployment

### Frontend

https://vectordb-frontend.onrender.com

### API

https://vectordb-api-yxup.onrender.com

### Example API

```bash
curl https://vectordb-api-yxup.onrender.com/health
```

Expected:

```json
{
  "service": "VectorDB",
  "status": "ok"
}
```

---

# Design Goals

This project was built to explore the internal architecture of modern vector-search and RAG systems.

The implementation focuses on understanding:

- How vector databases store embeddings
- How distance metrics affect retrieval
- How nearest-neighbor algorithms work
- Why HNSW performs well for high-dimensional vectors
- How embeddings connect semantic search with machine learning
- How retrieved context is passed into an LLM
- How a C++ backend can expose vector search through a REST API
- How to package and deploy the system using Docker

---

# Future Improvements

Possible extensions include:

- Persistent cloud storage for vectors
- Authentication and API keys
- Rate limiting
- Streaming LLM responses
- Batch ingestion
- Metadata filtering
- More advanced HNSW tuning
- Larger benchmark datasets
- ANN recall/latency dashboards
- WebSocket-based live indexing
- Multi-user document collections
- Hybrid keyword + vector search
- GPU-accelerated embeddings
- Background document ingestion

---

# Why This Project?

Modern AI applications increasingly depend on semantic search, embeddings, vector databases, and RAG.

Instead of treating the vector database as a black box, this project implements the core components directly:

```text
Vector Storage
       +
Distance Functions
       +
Nearest Neighbor Search
       +
HNSW Graph
       +
Persistence
       +
REST API
       +
Embeddings
       +
RAG
       =
End-to-End Vector Search System
```

---

# Author

**Jayanta Kundu**

CSE Student | NIT Calicut

GitHub:  
https://github.com/JayantaKundu17

---

## License

This project is intended for educational, experimental, and portfolio use.
