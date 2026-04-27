from flask import Flask, request, jsonify, render_template
from flask_socketio import SocketIO
import sqlite3
import csv
import os

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# 🔥 CSV FILE (ABSOLUTE PATH FIX)
CSV_FILE = os.path.join(os.getcwd(), "dataset.csv")
print("CSV FILE PATH:", CSV_FILE)

# -------------------- INIT DB --------------------
def init_db():
    with sqlite3.connect("database.db") as conn:
        conn.execute('''
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                house_id TEXT,
                gas_type TEXT,
                ppm_mq2 INTEGER,
                ppm_mq7 INTEGER,
                alert_level TEXT,
                fan_status TEXT,
                timestamp TEXT
            )
        ''')

init_db()

# -------------------- INIT CSV --------------------
def init_csv():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(["mq2", "mq7", "label"])
            file.flush()
            os.fsync(file.fileno())
            print("CSV file created with header")

init_csv()

# -------------------- API --------------------
@app.route('/data', methods=['POST'])
def receive_data():
    print("API HIT")

    data = request.json
    print("Incoming:", data)

    if not data:
        return jsonify({"error": "no data"}), 400

    mq2 = data.get("ppm_mq2")
    mq7 = data.get("ppm_mq7")
    label = data.get("alert_level")

    print("Parsed:", mq2, mq7, label)

    # -------------------- SAVE TO DB --------------------
    with sqlite3.connect("database.db") as conn:
        conn.execute('''
            INSERT INTO readings 
            (house_id, gas_type, ppm_mq2, ppm_mq7, alert_level, fan_status, timestamp)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            data.get("house_id"),
            data.get("gas_type"),
            mq2,
            mq7,
            label,
            data.get("fan_status"),
            data.get("timestamp")
        ))

    # # -------------------- SAVE TO CSV (FIXED) --------------------
    # try:
    #     with open(CSV_FILE, mode='a', newline='') as file:
    #         writer = csv.writer(file)
    #         writer.writerow([mq2, mq7, label])

    #         # 🔥 FORCE WRITE TO DISK
    #         file.flush()
    #         os.fsync(file.fileno())

    #         print("✅ Written to CSV:", mq2, mq7, label)

    # except Exception as e:
    #     print("❌ CSV ERROR:", e)

    # print("Saved to CSV:", mq2, mq7, label)

    # -------------------- REAL-TIME UPDATE --------------------
    socketio.emit("new_data", data)

    return jsonify({"status": "success"}), 200


# -------------------- UI --------------------
@app.route('/')
def index():
    return render_template("index.html")


# -------------------- RUN --------------------
if __name__ == '__main__':
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)