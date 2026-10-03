import json
import urllib.request
import uuid
from pathlib import Path

OLLAMA_URL = "http://localhost:11434/api/embed"
VECTORDB_URL = "http://localhost:8080/vectors"

MODEL = "nomic-embed-text"

CHUNK_SIZE = 250
OVERLAP = 50


def embed(text):
    data = json.dumps({
        "model": MODEL,
        "input": text
    }).encode()

    request = urllib.request.Request(
        OLLAMA_URL,
        data=data,
        headers={"Content-Type": "application/json"}
    )

    with urllib.request.urlopen(request) as response:
        result = json.load(response)

    return result["embeddings"][0]


def add_vector(vector_id, vector, text):
    data = json.dumps({
        "id": vector_id,
        "vector": vector,
        "text": text
    }).encode()

    request = urllib.request.Request(
        VECTORDB_URL,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST"
    )

    try:
        with urllib.request.urlopen(request) as response:
            return json.load(response)
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        print(f"VectorDB error: {body}")
        return None


def chunk_text(text):
    words = text.split()

    chunks = []
    start = 0

    while start < len(words):
        end = min(start + CHUNK_SIZE, len(words))
        chunk = " ".join(words[start:end])

        if chunk.strip():
            chunks.append(chunk)

        if end == len(words):
            break

        start = end - OVERLAP

    return chunks


def main():
    path = Path("data/sample.txt")

    if not path.exists():
        print("File not found:", path)
        return

    text = path.read_text(encoding="utf-8")

    chunks = chunk_text(text)

    print("========================================")
    print("VectorDB RAG Ingestion")
    print("========================================")
    print("File       :", path)
    print("Chunks     :", len(chunks))
    print("Embedding  :", MODEL)
    print("Dimension  : 768")
    print("========================================")

    for i, chunk in enumerate(chunks):
        print(f"\nEmbedding chunk {i + 1}/{len(chunks)}...")

        vector = embed(chunk)

        vector_id = f"sample_chunk_{i}"

        result = add_vector(
            vector_id,
            vector,
            chunk
        )

        if result:
            print("Stored:", vector_id)

    print("\n========================================")
    print("Ingestion complete")
    print("========================================")


if __name__ == "__main__":
    main()
