import json
import urllib.request

OLLAMA_URL = "http://localhost:11434/api/embed"
SEARCH_URL = "http://localhost:8080/search"

MODEL = "nomic-embed-text"


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


def search(vector, k=3):
    data = json.dumps({
        "vector": vector,
        "k": k,
        "metric": "cosine"
    }).encode()

    request = urllib.request.Request(
        SEARCH_URL,
        data=data,
        headers={"Content-Type": "application/json"}
    )

    with urllib.request.urlopen(request) as response:
        return json.load(response)


def main():
    query = "What is a vector database?"

    print("========================================")
    print("VectorDB Semantic Search")
    print("========================================")
    print("Query:", query)

    vector = embed(query)

    print("Embedding dimension:", len(vector))

    results = search(vector, 3)

    print("\nSearch results:")
    print(json.dumps(results, indent=2))

    print("========================================")


if __name__ == "__main__":
    main()
