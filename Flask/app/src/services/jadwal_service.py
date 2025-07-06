from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.cron import CronTrigger
from datetime import datetime, timedelta
import pytz
import os
from dotenv import load_dotenv

from app.src.services.mqtt_service import latest_sensor_data
from app.src.services.fuzzy_service import cek_kelayakan_air as fuzzy_service
from app.src.services.notification_service import notify_sensor_data_Service
from app.src.repositories.data_sensor_repositories import (
    create_data_sensor_repository,
    get_all_data_sensors_repository,
    delete_old_data_sensor_repository,  # pastikan fungsi ini ada
)

load_dotenv()

# Inisialisasi scheduler
scheduler = BackgroundScheduler(timezone=pytz.timezone('Asia/Jakarta'))


# Fungsi untuk menyimpan dan mengecek data kualitas air
def _scheduled_water_quality_check(app):
    with app.app_context():
        print(f"🔄 Memeriksa data kualitas air: {latest_sensor_data}")

        if None in latest_sensor_data.values():
            print("❌ Data sensor tidak lengkap.")
            return

        kelayakan = fuzzy_service(
            latest_sensor_data.get('PH'),
            latest_sensor_data.get('TDS'),
            latest_sensor_data.get('Turbidity'),
            latest_sensor_data.get('Suhu')
        )
        print(f"✅ Status kelayakan air: {kelayakan}")

        data_sensor = {
            'ph': latest_sensor_data.get('PH'),
            'tds': latest_sensor_data.get('TDS'),
            'suhu': latest_sensor_data.get('Suhu'),
            'turbidity': latest_sensor_data.get('Turbidity'),
            'kelayakan': 1 if kelayakan == 'Layak Minum' else 0,
            'dibuat_sejak': datetime.now(pytz.timezone('Asia/Jakarta')).strftime('%Y-%m-%d %H:%M:%S')
        }

        notify_sensor_data_Service(
            f"⚠️ Air {kelayakan}\n"
            f"PH: {data_sensor['ph']}\n"
            f"TDS: {data_sensor['tds']} ppm\n"
            f"Suhu: {data_sensor['suhu']} °C\n"
            f"Turbidity: {data_sensor['turbidity']} NTU\n"
            f"📡 Akses: {os.getenv('FLASK_URL')}",
            app
        )

        create_data_sensor_repository(data_sensor)
        print("💾 Data sensor disimpan.")


# Fungsi untuk menghapus data yang lebih dari 7 hari
def _scheduled_data_cleanup(app):
    with app.app_context():
        batas_waktu = datetime.now(pytz.timezone('Asia/Jakarta')) - timedelta(days=7)
        jumlah = delete_old_data_sensor_repository(batas_waktu.strftime('%Y-%m-%d %H:%M:%S'))
        print(f"🧹 Menghapus {jumlah} data sensor yang lebih lama dari 7 hari.")


# Fungsi untuk memulai semua penjadwalan
def start_water_quality_scheduler(app):
    # Jadwal pengambilan data
    times = ['03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00', '00:00', '01:24']
    for time_str in times:
        hour, minute = map(int, time_str.strip().split(':'))
        scheduler.add_job(
            _scheduled_water_quality_check,
            CronTrigger(hour=hour, minute=minute, second=0),
            args=[app],
            id=f"water_quality_{hour:02d}{minute:02d}",
            replace_existing=True
        )

    # Jadwal penghapusan data (sekali sehari)
    scheduler.add_job(
        _scheduled_data_cleanup,
        CronTrigger(hour=1, minute=0, second=0),  # setiap hari jam 01:00
        args=[app],
        id="cleanup_old_data",
        replace_existing=True
    )

    scheduler.start()
    print("🕒 Penjadwalan pengecekan kualitas air dan penghapusan data lama dimulai.")


def get_all_water_data_service():
    return get_all_data_sensors_repository()
