from app import create_app, socketio
import threading
from app.src.services.mqtt_service import run_mqtt_service
from app.src.services.jadwal_service import start_water_quality_scheduler

app = create_app()

def start_background_services():
    threading.Thread(target=lambda: run_mqtt_service(app), daemon=True).start()
    threading.Thread(target=lambda: start_water_quality_scheduler(app), daemon=True).start()

if __name__ == "__main__":
    start_background_services()
    socketio.run(app, debug=False, host="0.0.0.0", port=5005, use_reloader=False)
