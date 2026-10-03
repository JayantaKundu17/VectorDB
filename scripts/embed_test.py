import json
import urllib.request

OLLAMA_URL = "http://localhost:11434/api/embed"
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


if __name__ == "__main__":
    text = "Vector databases enable semantic search."

    vector = embed(text)

    print("========================================")
    print("Ollama Embedding Test")
    print("========================================")
    print("Model      :", MODEL)
    print("Input      :", text)
    print("Dimension  :", len(vector))
    print("First 5    :", vector[:5])
    print("========================================")
