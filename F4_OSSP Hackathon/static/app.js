
const $ = id => document.getElementById(id);

let toastTimer;

function showToast(message, error = false) {
    const toast = $("toast");
    toast.textContent = message;
    toast.className = error ? "show error" : "show";

    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => {
        toast.className = "";
    }, 3000);
}

function escapeHTML(value) {
    return String(value ?? "").replace(/[&<>"']/g, ch => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[ch]);
}

async function api(url, options = {}) {
    const response = await fetch(url, {
        ...options,
        headers: {
            "Content-Type": "application/json",
            ...(options.headers || {})
        }
    });

    const data = await response.json();

    if (!response.ok || data.ok === false) {
        throw new Error(data.error || "Request failed");
    }

    return data;
}

async function startOperation(button, url, payload) {
    button.disabled = true;

    try {
        const result = await api(url, {
            method: "POST",
            body: JSON.stringify(payload)
        });

        showToast(result.message || "Operation started");
        await refreshDashboard();
    } catch (error) {
        showToast(error.message, true);
    } finally {
        button.disabled = false;
    }
}

$("readBtn").addEventListener("click", () => {
    const worker = $("readerName").value.trim() || "Reader";

    startOperation($("readBtn"), "/api/read", {
        worker
    });
});

$("writeBtn").addEventListener("click", () => {
    const worker = $("writerName").value.trim() || "Writer";
    const key = $("recordKey").value.trim();
    const value = $("recordValue").value.trim();

    if (!key || !value) {
        showToast("Enter both the record key and value.", true);
        return;
    }

    startOperation($("writeBtn"), "/api/write", {
        worker,
        key,
        value
    });
});

$("resetBtn").addEventListener("click", async () => {
    try {
        await api("/api/reset", {
            method: "POST",
            body: "{}"
        });

        showToast("Demo database reset.");
        await refreshDashboard();
    } catch (error) {
        showToast(error.message, true);
    }
});

function renderRecords(records) {
    $("recordsBody").innerHTML = records.length
        ? records.map(record => `
            <tr>
                <td>${record.id}</td>
                <td>${escapeHTML(record.key)}</td>
                <td>${escapeHTML(record.value)}</td>
                <td>${escapeHTML(record.updated_at)}</td>
            </tr>
        `).join("")
        : '<tr><td colspan="4">No records found.</td></tr>';
}

function renderEvents(events) {
    $("eventList").innerHTML = events.length
        ? events.map(event => `
            <div class="event">${escapeHTML(event.message)}</div>
        `).join("")
        : "No events yet.";
}

async function refreshDashboard() {
    try {
        const data = await api("/api/state");
        const state = data.state;
        const stats = data.stats;

        $("activeReaders").textContent = state.active_readers;
        $("writerStatus").textContent =
            state.writer_active ? "Active" : "Idle";
        $("totalReads").textContent = stats.reads;
        $("totalWrites").textContent = stats.writes;

        $("sharedCount").textContent = state.active_readers;
        $("exclusiveState").textContent =
            state.writer_active ? "Yes" : "No";
        $("waitingWriters").textContent = state.waiting_writers;

        $("queueFill").style.width =
            Math.min(100, state.waiting_writers * 20) + "%";

        $("lockState").textContent = state.writer_active
            ? "Writer has exclusive access"
            : state.active_readers > 0
                ? "Readers have shared access"
                : "Ready for access";

        $("lockDescription").textContent = state.writer_active
            ? "Other readers and writers must wait."
            : state.active_readers > 0
                ? "Readers are accessing the database together."
                : "The database is available for new operations.";

        renderRecords(data.records || []);
        renderEvents(data.events || []);
    } catch (error) {
        console.error("Dashboard refresh failed:", error);
    }
}

refreshDashboard();
setInterval(refreshDashboard, 1000);
