import requests
import random
import time
from datetime import datetime

URL = "http://127.0.0.1:5000/data"
houses = [f"House_{i:02d}" for i in range(1, 4)]

while True:
    for house in houses:
        mq2 = random.randint(0, 600)
        level = "safe" if mq2 < 250 else "warning" if mq2 < 450 else "danger"
        
        data = {
            "house_id": house,
            "gas_type": "LPG",
            "ppm_mq2": mq2,
            "ppm_mq7": random.randint(10, 50),
            "alert_level": level,
            "fan_status": "on" if mq2 > 300 else "off",
            "timestamp": datetime.now().strftime("%H:%M:%S")
        }
        try:
            requests.post(URL, json=data)
        except:
            pass
    print("Cycle complete. All 3 houses updated.")
    time.sleep(3)