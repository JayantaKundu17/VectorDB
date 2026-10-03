const API_BASE = "http://localhost:8080";

const questionInput = document.getElementById("questionInput");
const sendButton = document.getElementById("sendButton");
const chatMessages = document.getElementById("chatMessages");
const sourcesContainer = document.getElementById("sourcesContainer");
const sourceCount = document.getElementById("sourceCount");
const refreshButton = document.getElementById("refreshButton");
const systemStatus = document.getElementById("systemStatus");

const navItems = document.querySelectorAll(".nav-item");
const sections = document.querySelectorAll(".section");
const pageTitle = document.getElementById("pageTitle");
const pageSubtitle = document.getElementById("pageSubtitle");

navItems.forEach(button => {
    button.addEventListener("click", async () => {
        navItems.forEach(item => item.classList.remove("active"));
        sections.forEach(section => section.classList.remove("active"));
        button.classList.add("active");

        const sectionId = button.dataset.section;
        document.getElementById(sectionId).classList.add("active");
        updatePageHeader(sectionId);

        if (sectionId === "databaseSection") {
            await loadStats();
            await loadPCA(
                lastRagSources.map(source => source?.id).filter(Boolean),
                queryHistory
            );
        }
    });
});

function updatePageHeader(sectionId) {
    if (sectionId === "ragSection") {
        pageTitle.textContent = "RAG Search";
        pageSubtitle.textContent =
            "Ask questions about your indexed knowledge base.";
    } else if (sectionId === "databaseSection") {
        pageTitle.textContent = "Vector Database";
        pageSubtitle.textContent =
            "Live database metrics and search performance.";
    } else if (sectionId === "systemSection") {
        pageTitle.textContent = "System";
        pageSubtitle.textContent =
            "Configuration of the current VectorDB deployment.";
    }
}

async function askQuestion() {
    const question = questionInput.value.trim();

    if (!question) {
        return;
    }

    sendButton.disabled = true;

    removeWelcome();

    addUserMessage(question);

    questionInput.value = "";
    questionInput.style.height = "auto";

    addLoadingMessage();

    try {
        // text/plain intentionally avoids the CORS preflight.
        // The request body is still JSON and Server.cpp parses it as JSON.
        const response = await fetch(`${API_BASE}/rag`, {
            method: "POST",

            headers: {
                "Content-Type": "text/plain"
            },

            body: JSON.stringify({
                question,
                k: 3
            })
        });

        if (!response.ok) {
            let message = `HTTP ${response.status}`;

            try {
                const errorData = await response.json();

                if (errorData?.error) {
                    message = errorData.error;
                }
            } catch (_) {}

            throw new Error(message);
        }

        const data = await response.json();

        removeLoadingMessage();

        addAnswerMessage(data);

        displaySources(data.sources || []);

        lastRagSources = data.sources || [];

        updatePCAHighlights(lastRagSources);

        // Store every query embedding for the current browser session.
        if (
            Array.isArray(data.query_vector) &&
            data.query_vector.length > 1
        ) {
            queryHistory.push({
                id: `query_${nextQueryNumber++}`,
                question,
                vector: data.query_vector,
                isQuery: true
            });
        } else {
            console.warn(
                "RAG response did not contain query_vector; query point cannot be plotted."
            );
        }

        await loadPCA(
            lastRagSources
                .map(source => source?.id)
                .filter(Boolean),
            queryHistory
        );

    } catch (error) {

        console.error(
            "VectorDB request failed:",
            error
        );

        removeLoadingMessage();

        addErrorMessage(
            `Unable to contact VectorDB API: ${error.message}`
        );

        displaySources([]);

        lastRagSources = [];

        updatePCAHighlights([]);

    } finally {

        sendButton.disabled = false;

        questionInput.focus();
    }
}

function removeWelcome() {
    document.querySelector(".welcome")?.remove();
}

function addUserMessage(question) {
    const message =
        document.createElement("div");

    message.className =
        "message message-user";

    message.innerHTML =
        `<div class="user-bubble">${escapeHTML(question)}</div>`;

    chatMessages.appendChild(message);

    scrollChat();
}

function addAnswerMessage(data) {

    const message =
        document.createElement("div");

    message.className =
        "message";

    message.innerHTML = `
        <div class="answer-block">

            <div class="answer-label">
                VectorDB ·
                ${escapeHTML(data.index || "HNSW")}
                ·
                ${escapeHTML(data.model || "llama3.2")}
            </div>

            <div class="answer-text">
                ${escapeHTML(
                    data.answer ||
                    "No answer returned."
                )}
            </div>

        </div>
    `;

    chatMessages.appendChild(message);

    scrollChat();
}

function addErrorMessage(text) {

    const message =
        document.createElement("div");

    message.className =
        "message";

    message.innerHTML = `
        <div class="answer-block">

            <div class="answer-label">
                Error
            </div>

            <div class="answer-text">
                ${escapeHTML(text)}
            </div>

        </div>
    `;

    chatMessages.appendChild(message);

    scrollChat();
}

function addLoadingMessage() {

    const message =
        document.createElement("div");

    message.className =
        "message loading-message";

    message.innerHTML = `
        <div class="loading">

            <div class="loading-dot"></div>

            <div class="loading-dot"></div>

            <div class="loading-dot"></div>

            <span>
                Searching HNSW and generating answer...
            </span>

        </div>
    `;

    chatMessages.appendChild(message);

    scrollChat();
}

function removeLoadingMessage() {
    document
        .querySelector(".loading-message")
        ?.remove();
}

function scrollChat() {
    chatMessages.scrollTop =
        chatMessages.scrollHeight;
}


// ============================================================
// RETRIEVED SOURCES
// ============================================================

function displaySources(sources) {

    sourcesContainer.innerHTML = "";

    sourceCount.textContent =
        `${sources.length} source${
            sources.length === 1 ? "" : "s"
        }`;

    if (!sources.length) {

        sourcesContainer.innerHTML = `
            <div class="empty-state">

                <div class="empty-icon">
                    ⌕
                </div>

                <p>
                    No source documents were returned.
                </p>

            </div>
        `;

        return;
    }

    sources.forEach((source, index) => {

        const card =
            document.createElement("div");

        card.className =
            "source-card";

        const score =
            Number(source.score || 0);

        card.innerHTML = `
            <div class="source-top">

                <div class="source-id">
                    ${index + 1}.
                    ${escapeHTML(
                        source.id || "Unknown"
                    )}
                </div>

                <div class="source-score">
                    ${score.toFixed(4)}
                </div>

            </div>

            <div class="source-text">
                ${escapeHTML(
                    source.text ||
                    "No text available."
                )}
            </div>
        `;

        sourcesContainer.appendChild(card);
    });
}


// ============================================================
// STATS
// ============================================================

async function loadStats() {

    try {

        const response =
            await fetch(`${API_BASE}/stats`);

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const data =
            await response.json();

        document.getElementById(
            "statVectors"
        ).textContent =
            formatNumber(data.vectors);

        document.getElementById(
            "statIndexSize"
        ).textContent =
            formatNumber(data.index_size);

        document.getElementById(
            "statIndex"
        ).textContent =
            data.index || "HNSW";

        setSystemOnline();

    } catch (error) {

        console.error(
            "Stats request failed:",
            error
        );

        document.getElementById(
            "statVectors"
        ).textContent =
            "Offline";

        document.getElementById(
            "statIndexSize"
        ).textContent =
            "—";

        document.getElementById(
            "statIndex"
        ).textContent =
            "—";

        setSystemOffline();
    }
}


// ============================================================
// HEALTH
// ============================================================

async function checkHealth() {

    try {

        const response =
            await fetch(`${API_BASE}/health`);

        return response.ok;

    } catch (_) {

        return false;
    }
}

async function updateHealth() {

    const healthy =
        await checkHealth();

    healthy
        ? setSystemOnline()
        : setSystemOffline();
}

function setSystemOnline() {

    if (systemStatus) {
        systemStatus.textContent =
            "System Online";
    }
}

function setSystemOffline() {

    if (systemStatus) {
        systemStatus.textContent =
            "System Offline";
    }
}


// ============================================================
// REFRESH
// ============================================================

refreshButton.addEventListener(
    "click",
    async () => {

        const original =
            refreshButton.textContent;

        refreshButton.textContent =
            "↻ Loading...";

        await loadStats();

        await updateHealth();

        if (
            document
                .getElementById(
                    "databaseSection"
                )
                .classList
                .contains("active")
        ) {
            await loadPCA(
                lastRagSources
                    .map(source => source?.id)
                    .filter(Boolean),
                queryHistory
            );
        }

        refreshButton.textContent =
            original;
    }
);


// ============================================================
// EXAMPLE QUESTIONS
// ============================================================

document
    .querySelectorAll(".example-question")
    .forEach(button => {

        button.addEventListener(
            "click",
            () => {

                questionInput.value =
                    button.textContent.trim();

                questionInput.focus();
            }
        );
    });


// ============================================================
// SEND
// ============================================================

sendButton.addEventListener(
    "click",
    askQuestion
);


// ============================================================
// ENTER KEY
// ============================================================

questionInput.addEventListener(
    "keydown",
    event => {

        if (
            event.key === "Enter" &&
            !event.shiftKey
        ) {

            event.preventDefault();

            askQuestion();
        }
    }
);


// ============================================================
// TEXTAREA AUTO RESIZE
// ============================================================

questionInput.addEventListener(
    "input",
    () => {

        questionInput.style.height =
            "auto";

        questionInput.style.height =
            Math.min(
                questionInput.scrollHeight,
                130
            ) + "px";
    }
);


// ============================================================
// PCA VECTOR VISUALIZATION
// ============================================================

const pcaCanvas =
    document.getElementById("pcaCanvas");

const pcaTooltip =
    document.getElementById("pcaTooltip");

const pcaSelected =
    document.getElementById("pcaSelected");

let pcaPoints = [];

let pcaDimension = 0;

let highlightedPcaIds =
    new Set();

let highlightedPcaTexts =
    new Set();

let lastRagSources =
    [];

let queryHistory =
    [];

let nextQueryNumber =
    1;


// ============================================================
// LOAD PCA
// ============================================================

async function loadPCA(
    extraIds = [],
    extraQueryVectors = []
) {

    if (!pcaCanvas) {
        return;
    }

    const status =
        document.getElementById(
            "pcaStatus"
        );

    const sampleSize =
        document.getElementById(
            "pcaSampleSize"
        );

    const dimension =
        document.getElementById(
            "pcaDimension"
        );

    const variance =
        document.getElementById(
            "pcaVariance"
        );

    status.textContent =
        "Loading...";

    try {

        const response =
            await fetch(
                `${API_BASE}/visualization/vectors?limit=500`
            );

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const data =
            await response.json();

        const records =
            [...(data.vectors || [])];


        // The visualization endpoint returns only a sample.
        // A retrieved source can therefore be outside that sample.
        const existingIds =
            new Set(
                records.map(
                    record =>
                        normalizePcaId(
                            record?.id
                        )
                )
            );


        // Fetch retrieved vectors explicitly.
        for (
            const id of extraIds || []
        ) {

            const normalizedId =
                normalizePcaId(id);

            if (
                !normalizedId ||
                existingIds.has(
                    normalizedId
                )
            ) {
                continue;
            }

            try {

                const vectorResponse =
                    await fetch(
                        `${API_BASE}/vectors/${encodeURIComponent(id)}`
                    );

                if (!vectorResponse.ok) {
                    continue;
                }

                const vectorRecord =
                    await vectorResponse.json();

                if (
                    Array.isArray(
                        vectorRecord?.vector
                    )
                ) {

                    records.push(
                        vectorRecord
                    );

                    existingIds.add(
                        normalizePcaId(
                            vectorRecord.id
                        )
                    );
                }

            } catch (fetchError) {

                console.warn(
                    `Could not load highlighted vector ${id}:`,
                    fetchError
                );
            }
        }


        // Query embeddings are visualized separately.
        // They are not persisted in VectorDB.
        for (
            const queryRecord of
            extraQueryVectors || []
        ) {

            if (
                !queryRecord ||
                !Array.isArray(
                    queryRecord.vector
                ) ||
                queryRecord.vector.length < 2
            ) {
                continue;
            }

            records.push({

                id:
                    queryRecord.id,

                text:
                    queryRecord.question || "",

                vector:
                    queryRecord.vector,

                dimension:
                    queryRecord.vector.length,

                isQuery:
                    true,

                question:
                    queryRecord.question || ""
            });
        }


        if (records.length < 2) {

            throw new Error(
                "Not enough vectors for PCA."
            );
        }


        const projection =
            calculatePCA(records);

        pcaPoints =
            projection.points;

        pcaDimension =
            projection.dimension;

        sampleSize.textContent =
            projection.points
                .length
                .toLocaleString();

        dimension.textContent =
            projection.dimension
                .toLocaleString();

        variance.textContent =
            `${projection.explainedVariance.toFixed(1)}%`;

        status.textContent =
            "Ready";

        drawPCA();

    } catch (error) {

        console.error(
            "PCA visualization error:",
            error
        );

        pcaPoints = [];

        sampleSize.textContent =
            "—";

        dimension.textContent =
            "—";

        variance.textContent =
            "—";

        status.textContent =
            "Unavailable";

        drawPCAError(
            error.message
        );
    }
}


// ============================================================
// PCA CALCULATION
// ============================================================

function calculatePCA(records) {

    const dimension =
        Number(
            records[0]?.dimension ||
            records[0]?.vector?.length ||
            0
        );

    const validRecords =
        records.filter(
            record =>
                Array.isArray(
                    record.vector
                ) &&
                record.vector.length ===
                    dimension
        );

    if (
        validRecords.length < 2 ||
        dimension < 2
    ) {

        throw new Error(
            "Not enough consistent-dimension vectors."
        );
    }

    const n =
        validRecords.length;

    const means =
        new Float64Array(
            dimension
        );


    for (
        const record of validRecords
    ) {

        for (
            let j = 0;
            j < dimension;
            j++
        ) {

            means[j] +=
                Number(
                    record.vector[j]
                );
        }
    }


    for (
        let j = 0;
        j < dimension;
        j++
    ) {

        means[j] /=
            n;
    }


    const matrix =
        new Array(n);


    for (
        let i = 0;
        i < n;
        i++
    ) {

        const row =
            new Float64Array(
                dimension
            );

        for (
            let j = 0;
            j < dimension;
            j++
        ) {

            row[j] =
                Number(
                    validRecords[i]
                        .vector[j]
                ) -
                means[j];
        }

        matrix[i] =
            row;
    }


    const pc1 =
        powerIteration(
            matrix,
            createInitialVector(
                dimension,
                1
            ),
            null,
            45
        );

    const pc2 =
        powerIteration(
            matrix,
            createInitialVector(
                dimension,
                2
            ),
            pc1,
            55
        );


    let totalVariance =
        0;

    let pc1Variance =
        0;

    let pc2Variance =
        0;

    const points =
        [];


    for (
        let i = 0;
        i < n;
        i++
    ) {

        const row =
            matrix[i];

        const x =
            dotProduct(
                row,
                pc1
            );

        const y =
            dotProduct(
                row,
                pc2
            );


        points.push({

            id:
                validRecords[i].id,

            text:
                validRecords[i].text ||
                "",

            x,

            y,

            isQuery:
                Boolean(
                    validRecords[i]
                        .isQuery
                ),

            question:
                validRecords[i]
                    .question ||
                ""
        });


        totalVariance +=
            dotProduct(
                row,
                row
            );

        pc1Variance +=
            x * x;

        pc2Variance +=
            y * y;
    }


    const explainedVariance =
        totalVariance > 0
            ? (
                (
                    pc1Variance +
                    pc2Variance
                ) /
                totalVariance
            ) *
                100
            : 0;


    return {
        points,
        dimension,
        explainedVariance
    };
}


// ============================================================
// PCA INITIAL VECTOR
// ============================================================

function createInitialVector(
    dimension,
    seed
) {

    const vector =
        new Float64Array(
            dimension
        );

    for (
        let i = 0;
        i < dimension;
        i++
    ) {

        vector[i] =
            Math.sin(
                (i + 1) *
                (seed * 12.9898)
            );
    }

    normalize(vector);

    return vector;
}


// ============================================================
// POWER ITERATION
// ============================================================

function powerIteration(
    matrix,
    initialVector,
    orthogonalTo,
    iterations
) {

    const dimension =
        initialVector.length;

    let vector =
        new Float64Array(
            initialVector
        );


    for (
        let iteration = 0;
        iteration < iterations;
        iteration++
    ) {

        const projection =
            new Float64Array(
                dimension
            );


        for (
            const row of matrix
        ) {

            const scalar =
                dotProduct(
                    row,
                    vector
                );


            for (
                let j = 0;
                j < dimension;
                j++
            ) {

                projection[j] +=
                    row[j] *
                    scalar;
            }
        }


        if (orthogonalTo) {

            const component =
                dotProduct(
                    projection,
                    orthogonalTo
                );


            for (
                let j = 0;
                j < dimension;
                j++
            ) {

                projection[j] -=
                    component *
                    orthogonalTo[j];
            }
        }


        normalize(
            projection
        );

        vector =
            projection;
    }


    return vector;
}


// ============================================================
// DOT PRODUCT
// ============================================================

function dotProduct(a, b) {

    let sum =
        0;

    for (
        let i = 0;
        i < a.length;
        i++
    ) {

        sum +=
            a[i] *
            b[i];
    }

    return sum;
}


// ============================================================
// NORMALIZE
// ============================================================

function normalize(vector) {

    let length =
        0;

    for (
        let i = 0;
        i < vector.length;
        i++
    ) {

        length +=
            vector[i] *
            vector[i];
    }


    length =
        Math.sqrt(
            length
        );

    if (!length) {
        return;
    }


    for (
        let i = 0;
        i < vector.length;
        i++
    ) {

        vector[i] /=
            length;
    }
}


// ============================================================
// PCA ID NORMALIZATION
// ============================================================

function normalizePcaId(value) {

    return String(
        value ?? ""
    )
        .trim()
        .toLowerCase()
        .replace(
            /^\"|\"$/g,
            ""
        );
}


// ============================================================
// PCA TEXT NORMALIZATION
// ============================================================

function normalizePcaText(value) {

    return String(
        value ?? ""
    )
        .trim()
        .replace(
            /\s+/g,
            " "
        )
        .toLowerCase();
}


// ============================================================
// UPDATE PCA HIGHLIGHTS
// ============================================================

function updatePCAHighlights(
    sources
) {

    highlightedPcaIds =
        new Set();

    highlightedPcaTexts =
        new Set();


    for (
        const source of sources || []
    ) {

        if (
            source?.id !== undefined &&
            source?.id !== null
        ) {

            highlightedPcaIds.add(
                normalizePcaId(
                    source.id
                )
            );
        }


        if (source?.text) {

            highlightedPcaTexts.add(
                normalizePcaText(
                    source.text
                )
            );
        }
    }


    drawPCA();
}


// ============================================================
// CHECK HIGHLIGHT
// ============================================================

function isPcaPointHighlighted(
    point
) {

    const pointId =
        normalizePcaId(
            point?.id
        );

    const pointText =
        normalizePcaText(
            point?.text
        );


    if (
        pointId &&
        highlightedPcaIds.has(
            pointId
        )
    ) {

        return true;
    }


    if (
        pointText &&
        highlightedPcaTexts.has(
            pointText
        )
    ) {

        return true;
    }


    // Support variants such as:
    // sample_chunk_0
    // vec_0
    // 0
    if (pointId) {

        const variants =
            new Set([
                pointId,

                pointId.replace(
                    /^sample_chunk_/,
                    ""
                ),

                pointId.replace(
                    /^vec_/,
                    ""
                ),

                `sample_chunk_${
                    pointId.replace(
                        /^vec_/,
                        ""
                    )
                }`,

                `vec_${
                    pointId.replace(
                        /^sample_chunk_/,
                        ""
                    )
                }`
            ]);


        for (
            const variant of variants
        ) {

            if (
                highlightedPcaIds.has(
                    variant
                )
            ) {

                return true;
            }
        }
    }


    return false;
}


// ============================================================
// DRAW PCA
// ============================================================

function drawPCA() {

    if (
        !pcaCanvas?.parentElement ||
        !pcaPoints.length
    ) {

        drawPCAError(
            "No vector data available."
        );

        return;
    }


    const rect =
        pcaCanvas.parentElement
            .getBoundingClientRect();

    const dpr =
        window.devicePixelRatio || 1;

    const width =
        rect.width;

    const height =
        rect.height;


    pcaCanvas.width =
        Math.floor(
            width * dpr
        );

    pcaCanvas.height =
        Math.floor(
            height * dpr
        );


    const ctx =
        pcaCanvas.getContext(
            "2d"
        );


    ctx.setTransform(
        dpr,
        0,
        0,
        dpr,
        0,
        0
    );


    ctx.clearRect(
        0,
        0,
        width,
        height
    );


    const padding =
        45;


    let minX =
        Infinity;

    let maxX =
        -Infinity;

    let minY =
        Infinity;

    let maxY =
        -Infinity;


    for (
        const point of pcaPoints
    ) {

        minX =
            Math.min(
                minX,
                point.x
            );

        maxX =
            Math.max(
                maxX,
                point.x
            );

        minY =
            Math.min(
                minY,
                point.y
            );

        maxY =
            Math.max(
                maxY,
                point.y
            );
    }


    const rangeX =
        maxX -
        minX ||
        1;

    const rangeY =
        maxY -
        minY ||
        1;


    const screenX =
        x =>
            padding +
            (
                (x - minX) /
                rangeX
            ) *
            (
                width -
                padding * 2
            );


    const screenY =
        y =>
            height -
            padding -
            (
                (y - minY) /
                rangeY
            ) *
            (
                height -
                padding * 2
            );


    // --------------------------------------------------------
    // GRID
    // --------------------------------------------------------

    ctx.lineWidth =
        1;

    ctx.strokeStyle =
        "rgba(255,255,255,0.055)";


    for (
        let i = 1;
        i <= 4;
        i++
    ) {

        const gx =
            padding +
            (
                width -
                padding * 2
            ) *
            (i / 5);

        const gy =
            padding +
            (
                height -
                padding * 2
            ) *
            (i / 5);


        ctx.beginPath();

        ctx.moveTo(
            gx,
            padding
        );

        ctx.lineTo(
            gx,
            height - padding
        );

        ctx.stroke();


        ctx.beginPath();

        ctx.moveTo(
            padding,
            gy
        );

        ctx.lineTo(
            width - padding,
            gy
        );

        ctx.stroke();
    }


    // --------------------------------------------------------
    // GRAPH BORDER
    // --------------------------------------------------------

    ctx.strokeStyle =
        "rgba(255,255,255,0.12)";

    ctx.strokeRect(
        padding,
        padding,
        width - padding * 2,
        height - padding * 2
    );


    // ========================================================
    // POINT DRAWING
    //
    // Query points retain their true PCA coordinates.
    // When multiple query points overlap visually, only the
    // marker is offset so each one remains visible.
    // ========================================================

    const drawnQueryPoints =
        [];


    for (
        const point of pcaPoints
    ) {

        const trueX =
            screenX(
                point.x
            );

        const trueY =
            screenY(
                point.y
            );


        const highlighted =
            isPcaPointHighlighted(
                point
            );

        const isQuery =
            Boolean(
                point.isQuery
            );


        let drawX =
            trueX;

        let drawY =
            trueY;


        // ----------------------------------------------------
        // SEPARATE OVERLAPPING QUERY POINTS
        // ----------------------------------------------------

        if (isQuery) {

            let nearbyQueryCount =
                0;


            for (
                const previous of
                drawnQueryPoints
            ) {

                const dx =
                    trueX -
                    previous.x;

                const dy =
                    trueY -
                    previous.y;

                const distance =
                    Math.sqrt(
                        dx * dx +
                        dy * dy
                    );


                if (
                    distance < 18
                ) {

                    nearbyQueryCount++;
                }
            }


            /*
             * Fan nearby query markers around the true PCA
             * position.
             *
             * The PCA coordinates stored in point.x / point.y
             * never change.
             */
            if (
                nearbyQueryCount > 0
            ) {

                const goldenAngle =
                    Math.PI *
                    (
                        3 -
                        Math.sqrt(5)
                    );


                const angle =
                    -Math.PI / 2 +
                    nearbyQueryCount *
                    goldenAngle;


                const radius =
                    10 +
                    Math.floor(
                        nearbyQueryCount /
                        8
                    ) *
                    5;


                drawX =
                    trueX +
                    Math.cos(
                        angle
                    ) *
                    radius;


                drawY =
                    trueY +
                    Math.sin(
                        angle
                    ) *
                    radius;

            } else {

                drawX =
                    trueX;

                drawY =
                    trueY;
            }


            drawnQueryPoints.push({
                x: trueX,
                y: trueY
            });
        }


        // Save displayed position for hover/click.
        point.screenX =
            drawX;

        point.screenY =
            drawY;


        const radius =
            isQuery
                ? 7
                : (
                    highlighted
                        ? 5.5
                        : 3.2
                );


        ctx.beginPath();

        ctx.arc(
            drawX,
            drawY,
            radius,
            0,
            Math.PI * 2
        );


        ctx.fillStyle =
            (
                isQuery ||
                highlighted
            )
                ? "#2563eb"
                : "rgba(20,20,20,0.92)";


        ctx.fill();


        // ----------------------------------------------------
        // QUERY POINT RING
        // ----------------------------------------------------

        if (isQuery) {

            ctx.beginPath();

            ctx.arc(
                drawX,
                drawY,
                11,
                0,
                Math.PI * 2
            );

            ctx.strokeStyle =
                "rgba(37,99,235,0.32)";

            ctx.lineWidth =
                2;

            ctx.stroke();


        } else if (highlighted) {

            // ------------------------------------------------
            // RETRIEVED VECTOR RING
            // ------------------------------------------------

            ctx.beginPath();

            ctx.arc(
                drawX,
                drawY,
                8,
                0,
                Math.PI * 2
            );

            ctx.strokeStyle =
                "rgba(37,99,235,0.25)";

            ctx.lineWidth =
                2;

            ctx.stroke();
        }
    }


    // --------------------------------------------------------
    // AXIS LABELS
    // --------------------------------------------------------

    ctx.fillStyle =
        "rgba(180,180,190,0.75)";

    ctx.font =
        "10px -apple-system, BlinkMacSystemFont, Segoe UI, sans-serif";


    ctx.fillText(
        "PC1",
        width -
            padding +
            8,
        height -
            padding +
            4
    );


    ctx.fillText(
        "PC2",
        padding -
            18,
        padding -
            10
    );
}


// ============================================================
// PCA ERROR
// ============================================================

function drawPCAError(
    message
) {

    if (
        !pcaCanvas?.parentElement
    ) {

        return;
    }


    const rect =
        pcaCanvas.parentElement
            .getBoundingClientRect();

    const dpr =
        window.devicePixelRatio || 1;


    pcaCanvas.width =
        Math.floor(
            rect.width *
            dpr
        );

    pcaCanvas.height =
        Math.floor(
            rect.height *
            dpr
        );


    const ctx =
        pcaCanvas.getContext(
            "2d"
        );


    ctx.setTransform(
        dpr,
        0,
        0,
        dpr,
        0,
        0
    );


    ctx.clearRect(
        0,
        0,
        rect.width,
        rect.height
    );


    ctx.fillStyle =
        "#707070";

    ctx.font =
        "12px -apple-system, BlinkMacSystemFont, Segoe UI, sans-serif";

    ctx.textAlign =
        "center";


    ctx.fillText(
        message,
        rect.width / 2,
        rect.height / 2
    );


    ctx.textAlign =
        "left";
}


// ============================================================
// PCA MOUSE INTERACTION
// ============================================================

if (pcaCanvas) {

    pcaCanvas.addEventListener(
        "mousemove",
        event => {

            if (
                !pcaPoints.length
            ) {

                return;
            }


            const rect =
                pcaCanvas
                    .getBoundingClientRect();


            const x =
                event.clientX -
                rect.left;

            const y =
                event.clientY -
                rect.top;


            let closest =
                null;

            let bestDistance =
                Infinity;


            for (
                const point of
                pcaPoints
            ) {

                const dx =
                    point.screenX -
                    x;

                const dy =
                    point.screenY -
                    y;


                const distance =
                    Math.sqrt(
                        dx * dx +
                        dy * dy
                    );


                if (
                    distance <
                    bestDistance
                ) {

                    bestDistance =
                        distance;

                    closest =
                        point;
                }
            }


            if (
                !closest ||
                bestDistance > 13
            ) {

                pcaTooltip.style.display =
                    "none";

                return;
            }


            pcaTooltip.innerHTML =
                closest.isQuery
                    ? `
                        <strong>
                            ${escapeHTML(
                                closest.id
                            )}
                        </strong><br>

                        <span
                            style="
                                color:#2563eb;
                                font-weight:600;
                            "
                        >
                            Query
                        </span><br>

                        ${escapeHTML(
                            closest.question ||
                            ""
                        )}<br>

                        PC1:
                        ${closest.x.toFixed(3)}

                        <br>

                        PC2:
                        ${closest.y.toFixed(3)}
                    `
                    : `
                        <strong>
                            ${escapeHTML(
                                closest.id
                            )}
                        </strong><br>

                        PC1:
                        ${closest.x.toFixed(3)}

                        <br>

                        PC2:
                        ${closest.y.toFixed(3)}
                    `;


            pcaTooltip.style.display =
                "block";


            let left =
                x + 14;

            let top =
                y + 14;


            if (
                left + 180 >
                rect.width
            ) {

                left =
                    x - 190;
            }


            if (
                top + 80 >
                rect.height
            ) {

                top =
                    y - 80;
            }


            pcaTooltip.style.left =
                `${left}px`;

            pcaTooltip.style.top =
                `${top}px`;
        }
    );


    pcaCanvas.addEventListener(
        "mouseleave",
        () => {

            pcaTooltip.style.display =
                "none";
        }
    );


    pcaCanvas.addEventListener(
        "click",
        event => {

            if (
                !pcaPoints.length
            ) {

                return;
            }


            const rect =
                pcaCanvas
                    .getBoundingClientRect();


            const x =
                event.clientX -
                rect.left;

            const y =
                event.clientY -
                rect.top;


            let closest =
                null;

            let bestDistance =
                Infinity;


            for (
                const point of
                pcaPoints
            ) {

                const dx =
                    point.screenX -
                    x;

                const dy =
                    point.screenY -
                    y;


                const distance =
                    Math.sqrt(
                        dx * dx +
                        dy * dy
                    );


                if (
                    distance <
                    bestDistance
                ) {

                    bestDistance =
                        distance;

                    closest =
                        point;
                }
            }


            if (
                !closest ||
                bestDistance > 13
            ) {

                return;
            }


            pcaSelected.classList.add(
                "active"
            );


            pcaSelected.innerHTML =
                closest.isQuery
                    ? `
                        <strong>
                            ${escapeHTML(
                                closest.id
                            )}
                        </strong>
                        ·

                        Query:
                        ${escapeHTML(
                            closest.question ||
                            ""
                        )}

                        ·

                        PC1 =
                        ${closest.x.toFixed(4)}

                        ·

                        PC2 =
                        ${closest.y.toFixed(4)}
                    `
                    : `
                        <strong>
                            ${escapeHTML(
                                closest.id
                            )}
                        </strong>

                        PC1 =
                        ${closest.x.toFixed(4)}

                        ·

                        PC2 =
                        ${closest.y.toFixed(4)}
                    `;
        }
    );
}


// ============================================================
// RESIZE
// ============================================================

window.addEventListener(
    "resize",
    () => {

        if (
            pcaPoints.length
        ) {

            drawPCA();
        }
    }
);


// ============================================================
// HTML ESCAPE
// ============================================================

function escapeHTML(value) {

    return String(value)

        .replaceAll(
            "&",
            "&amp;"
        )

        .replaceAll(
            "<",
            "&lt;"
        )

        .replaceAll(
            ">",
            "&gt;"
        )

        .replaceAll(
            '"',
            "&quot;"
        )

        .replaceAll(
            "'",
            "&#039;"
        );
}


// ============================================================
// NUMBER FORMAT
// ============================================================

function formatNumber(value) {

    if (
        value === undefined ||
        value === null
    ) {

        return "—";
    }


    return Number(value)
        .toLocaleString();
}


// ============================================================
// INITIAL LOAD
// ============================================================

loadStats();

updateHealth();