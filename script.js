const socket = io();

socket.on("connect", () => {
    console.log("Connected to server ✅");
});

let cards = {};
let activeAlerts = {};

// Helper: Generate random vibrant color
function getRandomColor() {
    const hue = Math.floor(Math.random() * 360);
    return `hsl(${hue}, 75%, 60%)`;
}

function showTab(tab) {
    document.getElementById("dashboard").style.display = tab === "dashboard" ? "block" : "none";
    document.getElementById("alerts").style.display = tab === "alerts" ? "block" : "none";

    document.querySelectorAll(".tab-btn").forEach(btn => btn.classList.remove("active"));
    event.target.classList.add("active");
}

function addAlertRow(house, type, start, end, duration) {
    const table = document.querySelector("#alertTable tbody");

    const row = `
        <tr>
            <td>${house}</td>
            <td style="color:${type === "danger" ? "red" : "orange"}">${type.toUpperCase()}</td>
            <td>${start}</td>
            <td>${end}</td>
            <td>${duration}s</td>
        </tr>
    `;

    table.innerHTML = row + table.innerHTML;
}

// Initialize Chart
const ctx = document.getElementById("chart").getContext("2d");
const chart = new Chart(ctx, {
    type: "line",
    data: {
        labels: [],
        datasets: [] 
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: { position: 'top', labels: { color: '#fff' } },
            tooltip: { mode: 'nearest', intersect: true }
        },
        // IMPROVED CLICK LOGIC
        onClick: (e) => {
            const points = chart.getElementsAtEventForMode(e, 'nearest', { intersect: true }, true);
            if (points.length > 0) {
                const datasetIndex = points[0].datasetIndex;
                const houseId = chart.data.datasets[datasetIndex].label;
                console.log("Selected via Chart:", houseId);
                selectHouse(houseId);
            }
        },
        scales: {
            y: {
                min: 350,   // 🔥 this sets starting point
                grid: {
                    color: "rgba(255,255,255,0.08)"
                },
                ticks: {
                    color: "#94a3b8"
                }
            },
            x: { 
                grid: { color: 'rgba(255,255,255,0.1)' },
                ticks: { color: '#94a3b8' }
            }
        }
    }
});

function selectHouse(id) {
    // Clear old selection
    document.querySelectorAll('.card').forEach(c => c.classList.remove('selected-card'));

    const selected = document.getElementById(id);
    if (selected) {
        selected.scrollIntoView({ behavior: 'smooth', block: 'center' });
        selected.classList.add('selected-card');
        
        // Visual feedback pulse
        selected.style.transform = "scale(1.05)";
        setTimeout(() => { selected.style.transform = "scale(1)"; }, 400);
    }
}

socket.on("new_data", (data) => {
    const cardContainer = document.getElementById("cards");

    // 1. Create Card & Dataset if New House
    if (!cards[data.house_id]) {
        let div = document.createElement("div");
        div.id = data.house_id;
        div.className = `card ${data.alert_level}`;
        cardContainer.appendChild(div);
        cards[data.house_id] = div;

        const color = getRandomColor();
        chart.data.datasets.push({
            label: data.house_id,
            data: [],
            borderColor: color,
            backgroundColor: color,
            tension: 0.3,
            borderWidth: 3,
            pointRadius: 5,
            pointHitRadius: 15 // Makes clicking lines much easier
        });
    }

    // 2. Update Card UI
    let card = cards[data.house_id];
    const isSelected = card.classList.contains('selected-card');
    card.className = `card ${data.alert_level} ${isSelected ? 'selected-card' : ''}`;
    card.innerHTML = `
        <h3>${data.house_id}</h3>
        <div class="data-row"><span>MQ2:</span> <b>${data.ppm_mq2} ppm</b></div>
        <div class="data-row"><span>MQ7:</span> <b>${data.ppm_mq7} ppm</b></div>
        <div class="data-row"><span>Status:</span> <b>${data.alert_level.toUpperCase()}</b></div>
        <div class="data-row"><span>Fan:</span> <b>${data.fan_status.toUpperCase()}</b></div>
    `;

    const nowAlert = new Date();
    const house = data.house_id;
    const status = data.alert_level;

    // START alert
    if ((status === "warning" || status === "danger") && !activeAlerts[house]) {
        activeAlerts[house] = {
            start: nowAlert,
            type: status
        };
    }

    // END alert (back to safe)
    if (status === "safe" && activeAlerts[house]) {
        const startTime = activeAlerts[house].start;
        const endTime = nowAlert;

        const duration = Math.floor((endTime - startTime) / 1000);

        addAlertRow(
            house,
            activeAlerts[house].type,
            startTime.toLocaleTimeString(),
            endTime.toLocaleTimeString(),
            duration
        );

        delete activeAlerts[house];
    }

    // 3. Update Chart Data
const now = new Date();
const timeLabel = now.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
});    
    chart.data.labels.push(timeLabel);

    if (chart.data.labels.length > 20) {
        chart.data.labels.shift();
    }

    const dataset = chart.data.datasets.find(ds => ds.label === data.house_id);
    if (dataset) {
        dataset.data.push({ x: timeLabel, y: data.ppm_mq2 });
        if (dataset.data.length > 20) dataset.data.shift();
    }
    
    chart.update('active'); 
});