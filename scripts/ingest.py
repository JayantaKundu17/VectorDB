#!/usr/bin/env python3

import hashlib
import json
import os
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


# ============================================================
# CONFIGURATION
# ============================================================

VECTORDB_URL = os.getenv(
    "VECTORDB_URL",
    "http://localhost:8080/vectors"
)

GEMINI_API_KEY = os.getenv("GEMINI_API_KEY", "").strip()

GEMINI_EMBED_MODEL = os.getenv(
    "GEMINI_EMBED_MODEL",
    "gemini-embedding-2"
)

EMBEDDING_DIMENSION = 768

DATA_DIR = Path(
    os.getenv("RAG_DOCS_DIR", "data/rag_docs")
)

CHUNK_SIZE = 120
CHUNK_OVERLAP = 25


# ============================================================
# HTTP HELPERS
# ============================================================

def http_json(url, payload=None, method="GET"):
    data = None

    headers = {
        "Accept": "application/json"
    }

    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"

    request = Request(
        url,
        data=data,
        headers=headers,
        method=method
    )

    try:
        with urlopen(request, timeout=120) as response:
            raw = response.read().decode("utf-8")

            try:
                return json.loads(raw)
            except json.JSONDecodeError:
                return {
                    "raw": raw
                }

    except HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(
            f"HTTP {exc.code}: {body}"
        ) from exc

    except URLError as exc:
        raise RuntimeError(
            f"Connection failed: {exc.reason}"
        ) from exc


# ============================================================
# TEXT CHUNKING
# ============================================================

def chunk_text(text):
    words = text.split()

    if not words:
        return []

    chunks = []

    start = 0

    while start < len(words):
        end = min(start + CHUNK_SIZE, len(words))
        chunks.append(" ".join(words[start:end]))

        if end >= len(words):
            break

        start = end - CHUNK_OVERLAP

    return chunks


# ============================================================
# GEMINI EMBEDDINGS
# ============================================================

def embed(text):
    if not GEMINI_API_KEY:
        raise RuntimeError(
            "GEMINI_API_KEY is not set."
        )

    endpoint = (
        "https://generativelanguage.googleapis.com/"
        "v1beta/models/"
        f"{GEMINI_EMBED_MODEL}:embedContent"
        f"?key={GEMINI_API_KEY}"
    )

    payload = {
        "content": {
            "parts": [
                {
                    "text": text
                }
            ]
        },
        "output_dimensionality": EMBEDDING_DIMENSION
    }

    result = http_json(
        endpoint,
        payload,
        method="POST"
    )

    try:
        vector = result["embedding"]["values"]
    except (KeyError, TypeError) as exc:
        raise RuntimeError(
            "Gemini embedding response did not contain "
            "'embedding.values'."
        ) from exc

    vector = [float(x) for x in vector]

    if len(vector) != EMBEDDING_DIMENSION:
        raise RuntimeError(
            "Gemini returned "
            f"{len(vector)} dimensions; "
            f"expected {EMBEDDING_DIMENSION}."
        )

    return vector


# ============================================================
# VECTORDB INSERT / UPSERT
# ============================================================

def add_vector(vector_id, vector, text, source):
    payload = {
        "id": vector_id,
        "vector": vector,
        "text": text
    }

    result = http_json(
        VECTORDB_URL,
        payload,
        method="POST"
    )

    if "error" in result:
        raise RuntimeError(
            f"VectorDB error: {result['error']}"
        )

    return result


# ============================================================
# MAIN
# ============================================================

def main():

    print("=" * 60)
    print("VectorDB Gemini RAG Ingestion")
    print("=" * 60)

    print(f"Directory  : {DATA_DIR}")
    print(f"Model      : {GEMINI_EMBED_MODEL}")
    print(f"Dimensions : {EMBEDDING_DIMENSION}")
    print(f"VectorDB   : {VECTORDB_URL}")
    print()

    if not DATA_DIR.exists():
        raise RuntimeError(
            f"Directory does not exist: {DATA_DIR}"
        )

    files = sorted(DATA_DIR.glob("*.txt"))

    if not files:
        raise RuntimeError(
            f"No .txt files found in {DATA_DIR}"
        )

    print(f"Documents  : {len(files)}")
    print()

    total_chunks = 0

    for file_path in files:

        source = file_path.stem

        text = file_path.read_text(
            encoding="utf-8"
        ).strip()

        chunks = chunk_text(text)

        print("-" * 60)
        print(f"Document   : {file_path.name}")
        print(f"Chunks     : {len(chunks)}")

        for index, chunk in enumerate(chunks):

            # Stable ID: same document/chunk produces
            # the same VectorDB record when ingested again.
            vector_id = (
                f"rag_{source}_chunk_{index}"
            )

            print(
                f"  Embedding chunk "
                f"{index + 1}/{len(chunks)} ..."
            )

            vector = embed(chunk)

            if len(vector) != EMBEDDING_DIMENSION:
                raise RuntimeError(
                    f"Wrong dimension for {vector_id}"
                )

            result = add_vector(
                vector_id,
                vector,
                chunk,
                source
            )

            print(
                f"  Stored    : "
                f"{result.get('id', vector_id)} "
                f"dimension={result.get('dimension', len(vector))}"
            )

            total_chunks += 1

    print()
    print("=" * 60)
    print("INGESTION COMPLETE")
    print("=" * 60)
    print(f"Documents  : {len(files)}")
    print(f"New chunks : {total_chunks}")


if __name__ == "__main__":
    main()
