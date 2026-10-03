import json
import urllib.request
import urllib.error

# ============================================================
# Configuration
# ============================================================

OLLAMA_URL = "http://localhost:11434"

EMBED_MODEL = "nomic-embed-text"
LLM_MODEL = "llama3.2"

SEARCH_URL = "http://localhost:8080/search"
VECTOR_URL = "http://localhost:8080/vectors"

TOP_K = 3


# ============================================================
# Ollama Embedding
# ============================================================

def embed(text):
    data = json.dumps({
        "model": EMBED_MODEL,
        "input": text
    }).encode()

    request = urllib.request.Request(
        f"{OLLAMA_URL}/api/embed",
        data=data,
        headers={
            "Content-Type": "application/json"
        },
        method="POST"
    )

    with urllib.request.urlopen(request) as response:
        result = json.load(response)

    embeddings = result.get("embeddings")

    if not embeddings:
        raise RuntimeError(
            "Ollama did not return an embedding."
        )

    return embeddings[0]


# ============================================================
# VectorDB Search
# ============================================================

def search(vector, k=TOP_K):
    data = json.dumps({
        "vector": vector,
        "k": k,
        "metric": "cosine"
    }).encode()

    request = urllib.request.Request(
        SEARCH_URL,
        data=data,
        headers={
            "Content-Type": "application/json"
        },
        method="POST"
    )

    with urllib.request.urlopen(request) as response:
        result = json.load(response)

    return result.get("results", [])


# ============================================================
# Retrieve Vector Record
# ============================================================

def get_vector(vector_id):
    request = urllib.request.Request(
        f"{VECTOR_URL}/{vector_id}",
        method="GET"
    )

    try:
        with urllib.request.urlopen(request) as response:
            return json.load(response)

    except urllib.error.HTTPError as e:
        print(
            f"Failed to retrieve vector {vector_id}: "
            f"{e.read().decode()}"
        )
        return None


# ============================================================
# Build RAG Context
# ============================================================

def build_context(results):
    context_parts = []

    for rank, result in enumerate(results, start=1):

        vector_id = result["id"]
        score = result["score"]

        record = get_vector(vector_id)

        if not record:
            continue

        text = record.get("text", "")

        if not text:
            continue

        context_parts.append(
            f"[Document {rank} | "
            f"ID: {vector_id} | "
            f"Score: {score:.4f}]\n"
            f"{text}"
        )

    return "\n\n".join(context_parts)


# ============================================================
# LLM Generation
# ============================================================

def generate_answer(question, context):

    prompt = f"""You are a Retrieval-Augmented Generation assistant.

Answer the user's question using ONLY the retrieved context below.

Rules:
1. Do not invent information.
2. Do not use outside knowledge.
3. If the context does not contain enough information, say:
   "The information is not available in the retrieved context."
4. Give a concise and direct answer.
5. Do not mention the internal retrieval process unless necessary.

Retrieved Context:
------------------
{context}
------------------

Question:
{question}

Answer:
"""

    data = json.dumps({
        "model": LLM_MODEL,
        "prompt": prompt,
        "stream": False
    }).encode()

    request = urllib.request.Request(
        f"{OLLAMA_URL}/api/generate",
        data=data,
        headers={
            "Content-Type": "application/json"
        },
        method="POST"
    )

    with urllib.request.urlopen(request) as response:
        result = json.load(response)

    answer = result.get("response")

    if not answer:
        raise RuntimeError(
            "Ollama did not return an answer."
        )

    return answer.strip()


# ============================================================
# Main RAG Pipeline
# ============================================================

def main():

    question = input(
        "Ask a question: "
    ).strip()

    if not question:
        print("Question cannot be empty.")
        return

    print()
    print("========================================")
    print("VectorDB RAG Pipeline")
    print("========================================")

    print()
    print("Question:")
    print(question)

    # --------------------------------------------------------
    # Step 1: Embed question
    # --------------------------------------------------------

    print()
    print("1. Generating query embedding...")

    query_vector = embed(question)

    print(
        f"   Embedding dimension: "
        f"{len(query_vector)}"
    )

    # --------------------------------------------------------
    # Step 2: HNSW semantic search
    # --------------------------------------------------------

    print()
    print("2. Searching VectorDB HNSW index...")

    results = search(
        query_vector,
        TOP_K
    )

    if not results:
        print()
        print("No relevant documents found.")
        return

    print(
        f"   Retrieved {len(results)} documents."
    )

    # --------------------------------------------------------
    # Step 3: Retrieve actual chunks
    # --------------------------------------------------------

    print()
    print("3. Retrieving document chunks...")

    context = build_context(results)

    if not context:
        print("No document text could be retrieved.")
        return

    for rank, result in enumerate(
        results,
        start=1
    ):
        print(
            f"   {rank}. "
            f"{result['id']} "
            f"(score: {result['score']:.4f})"
        )

    # --------------------------------------------------------
    # Step 4: Generate answer
    # --------------------------------------------------------

    print()
    print(
        f"4. Generating answer with "
        f"{LLM_MODEL}..."
    )

    answer = generate_answer(
        question,
        context
    )

    # --------------------------------------------------------
    # Step 5: Display answer
    # --------------------------------------------------------

    print()
    print("========================================")
    print("Answer")
    print("========================================")
    print(answer)

    print()
    print("========================================")
    print("RAG complete")
    print("========================================")


if __name__ == "__main__":
    main()
