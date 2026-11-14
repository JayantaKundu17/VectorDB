#!/usr/bin/env python3

import hashlib
import json
import os
import socket
import sys
import time
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

# Checkpoint file
CHECKPOINT_FILE = Path(
    os.getenv("RAG_CHECKPOINT", ".ingest_checkpoint.json")
)

# Network retry settings
HTTP_TIMEOUT = int(os.getenv("INGEST_HTTP_TIMEOUT", "120"))
MAX_RETRIES = int(os.getenv("INGEST_MAX_RETRIES", "5"))
RETRY_BASE_SECONDS = float(
    os.getenv("INGEST_RETRY_BASE_SECONDS", "2")
)

RETRYABLE_HTTP_STATUS = {
    408,
    429,
    500,
    502,
    503,
    504,
}


# ============================================================
# CHECKPOINT
# ============================================================

def load_checkpoint():
    if not CHECKPOINT_FILE.exists():
        return {
            "version": 1,
            "chunks": {}
        }

    try:
        with CHECKPOINT_FILE.open("r", encoding="utf-8") as f:
            data = json.load(f)

        if not isinstance(data, dict):
            raise ValueError("Checkpoint is not a JSON object.")

        if not isinstance(data.get("chunks"), dict):
            data["chunks"] = {}

        return data

    except Exception as exc:
        raise RuntimeError(
            f"Could not read checkpoint {CHECKPOINT_FILE}: {exc}"
        ) from exc


def save_checkpoint(checkpoint):
    """
    Save atomically so an interrupted write does not corrupt
    the checkpoint file.
    """
    temp_file = CHECKPOINT_FILE.with_suffix(".tmp")

    with temp_file.open("w", encoding="utf-8") as f:
        json.dump(checkpoint, f, indent=2, sort_keys=True)

    temp_file.replace(CHECKPOINT_FILE)


def chunk_signature(chunk):
    return hashlib.sha256(
        chunk.encode("utf-8")
    ).hexdigest()


def checkpoint_key(vector_id):
    return vector_id


def is_chunk_completed(
    checkpoint,
    vector_id,
    chunk_hash
):
    entry = checkpoint["chunks"].get(checkpoint_key(vector_id))

    if not isinstance(entry, dict):
        return False

    return (
        entry.get("sha256") == chunk_hash
        and entry.get("model") == GEMINI_EMBED_MODEL
        and entry.get("dimension") == EMBEDDING_DIMENSION
        and entry.get("vectordb_url") == VECTORDB_URL
    )


def mark_chunk_completed(
    checkpoint,
    vector_id,
    chunk_hash,
    source,
    index
):
    checkpoint["chunks"][checkpoint_key(vector_id)] = {
        "sha256": chunk_hash,
        "source": source,
        "chunk_index": index,
        "model": GEMINI_EMBED_MODEL,
        "dimension": EMBEDDING_DIMENSION,
        "vectordb_url": VECTORDB_URL,
    }

    save_checkpoint(checkpoint)


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

    for attempt in range(MAX_RETRIES):
        try:
            with urlopen(request, timeout=HTTP_TIMEOUT) as response:
                raw = response.read().decode("utf-8")

                try:
                    return json.loads(raw)
                except json.JSONDecodeError:
                    return {
                        "raw": raw
                    }

        except HTTPError as exc:
            body = exc.read().decode(
                "utf-8",
                errors="replace"
            )

            # Retry only temporary HTTP failures.
            if (
                exc.code in RETRYABLE_HTTP_STATUS
                and attempt < MAX_RETRIES - 1
            ):
                retry_after = exc.headers.get("Retry-After")

                if retry_after:
                    try:
                        delay = float(retry_after)
                    except ValueError:
                        delay = RETRY_BASE_SECONDS * (2 ** attempt)
                else:
                    delay = RETRY_BASE_SECONDS * (2 ** attempt)

                print(
                    f"  HTTP {exc.code}; retrying in "
                    f"{delay:.1f}s "
                    f"(attempt {attempt + 2}/{MAX_RETRIES})..."
                )

                time.sleep(delay)
                continue

            raise RuntimeError(
                f"HTTP {exc.code}: {body}"
            ) from exc

        except (URLError, socket.timeout, TimeoutError) as exc:
            if attempt < MAX_RETRIES - 1:
                delay = RETRY_BASE_SECONDS * (2 ** attempt)

                print(
                    f"  Network error: {exc}; "
                    f"retrying in {delay:.1f}s "
                    f"(attempt {attempt + 2}/{MAX_RETRIES})..."
                )

                time.sleep(delay)
                continue

            reason = getattr(exc, "reason", exc)

            raise RuntimeError(
                f"Connection failed after {MAX_RETRIES} attempts: "
                f"{reason}"
            ) from exc

    raise RuntimeError("HTTP request failed.")


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

        chunks.append(
            " ".join(words[start:end])
        )

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

    print(f"Directory    : {DATA_DIR}")
    print(f"Model        : {GEMINI_EMBED_MODEL}")
    print(f"Dimensions   : {EMBEDDING_DIMENSION}")
    print(f"VectorDB     : {VECTORDB_URL}")
    print(f"Checkpoint   : {CHECKPOINT_FILE}")
    print(f"Max retries  : {MAX_RETRIES}")
    print()

    if not GEMINI_API_KEY:
        raise RuntimeError(
            "GEMINI_API_KEY is not set."
        )

    if not DATA_DIR.exists():
        raise RuntimeError(
            f"Directory does not exist: {DATA_DIR}"
        )

    files = sorted(DATA_DIR.glob("*.txt"))

    if not files:
        raise RuntimeError(
            f"No .txt files found in {DATA_DIR}"
        )

    checkpoint = load_checkpoint()

    print(f"Documents    : {len(files)}")
    print(
        f"Checkpointed : "
        f"{len(checkpoint['chunks'])} chunks"
    )
    print()

    stored_chunks = 0
    skipped_chunks = 0

    for file_number, file_path in enumerate(files, start=1):

        source = file_path.stem

        text = file_path.read_text(
            encoding="utf-8"
        ).strip()

        chunks = chunk_text(text)

        print("-" * 60)
        print(
            f"Document {file_number}/{len(files)} : "
            f"{file_path.name}"
        )
        print(f"Chunks                    : {len(chunks)}")

        for index, chunk in enumerate(chunks):

            vector_id = (
                f"rag_{source}_chunk_{index}"
            )

            chunk_hash = chunk_signature(chunk)

            if is_chunk_completed(
                checkpoint,
                vector_id,
                chunk_hash
            ):
                print(
                    f"  Chunk {index + 1}/{len(chunks)} "
                    f"SKIPPED (checkpoint)"
                )
                skipped_chunks += 1
                continue

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
                f"dimension="
                f"{result.get('dimension', len(vector))}"
            )

            # IMPORTANT:
            # checkpoint only after the VectorDB write succeeded.
            mark_chunk_completed(
                checkpoint,
                vector_id,
                chunk_hash,
                source,
                index
            )

            print("  Checkpoint: saved")

            stored_chunks += 1

    print()
    print("=" * 60)
    print("INGESTION COMPLETE")
    print("=" * 60)
    print(f"Documents       : {len(files)}")
    print(f"Stored this run : {stored_chunks}")
    print(f"Skipped chunks  : {skipped_chunks}")
    print(
        f"Checkpoint total: "
        f"{len(checkpoint['chunks'])}"
    )


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        print("Ingestion interrupted.")
        print(
            "Completed chunks remain safely recorded "
            "in the checkpoint."
        )
        sys.exit(130)
    except Exception as exc:
        print()
        print("=" * 60)
        print("INGESTION STOPPED")
        print("=" * 60)
        print(str(exc))
        print()
        print(
            f"Checkpoint preserved at: "
            f"{CHECKPOINT_FILE}"
        )
        sys.exit(1)
