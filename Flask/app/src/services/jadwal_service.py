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
    delete_old_data_sensor_repository,
)

load_dotenv()
scheduler = BackgroundScheduler(timezone=pytz.timezone('Asia/Jakarta'))

# ✅ Fungsi hanya untuk menyimpan data ke database (setiap jam)
def _scheduled_store_sensor_data(app):
    with app.app_context():
        print(f"💾 Menyimpan data kualitas air: {latest_sensor_data}")

        if None in latest_sensor_data.values():
            print("❌ Data sensor tidak lengkap.")
            return

        kelayakan = fuzzy_service(
            latest_sensor_data.get('PH'),
            latest_sensor_data.get('TDS'),
            latest_sensor_data.get('Turbidity'),
            latest_sensor_data.get('Suhu')
        )

        data_sensor = {
            'ph': latest_sensor_data.get('PH'),
            'tds': latest_sensor_data.get('TDS'),
            'suhu': latest_sensor_data.get('Suhu'),
            'turbidity': latest_sensor_data.get('Turbidity'),
            'kelayakan': 1 if kelayakan == 'Layak Minum' else 0,
            'dibuat_sejak': datetime.now(pytz.timezone('Asia/Jakarta')).strftime('%Y-%m-%d %H:%M:%S')
        }

        create_data_sensor_repository(data_sensor)
        print("📦 Data sensor berhasil disimpan ke database.")

# 📲 Fungsi hanya untuk mengirim notifikasi WA (jam tertentu)
def _scheduled_notify_sensor_status(app):
    with app.app_context():
        print(f"📲 Mengirim notifikasi kualitas air: {latest_sensor_data}")

        if None in latest_sensor_data.values():
            print("❌ Data sensor tidak lengkap.")
            return

        kelayakan = fuzzy_service(
            latest_sensor_data.get('PH'),
            latest_sensor_data.get('TDS'),
            latest_sensor_data.get('Turbidity'),
            latest_sensor_data.get('Suhu')
        )

        message = (
            f"⚠️ Air {kelayakan}\n"
            f"PH: {latest_sensor_data.get('PH')}\n"
            f"TDS: {latest_sensor_data.get('TDS')} ppm\n"
            f"Suhu: {latest_sensor_data.get('Suhu')} °C\n"
            f"Turbidity: {latest_sensor_data.get('Turbidity')} NTU\n"
            f"📡 Akses: {os.getenv('FLASK_URL')}"
        )

        notify_sensor_data_Service(message, app)
        print("✅ Notifikasi WA berhasil dikirim.")

# 🧹 Fungsi hapus data lama (seminggu sekali)
def _scheduled_data_cleanup(app):
    with app.app_context():
        batas_waktu = datetime.now(pytz.timezone('Asia/Jakarta')) - timedelta(days=7)
        jumlah = delete_old_data_sensor_repository(batas_waktu.strftime('%Y-%m-%d %H:%M:%S'))
        print(f"🧹 Menghapus {jumlah} data sensor yang lebih lama dari 7 hari.")

# 🚀 Inisialisasi seluruh penjadwalan
def start_water_quality_scheduler(app):
    # ⏰ Simpan data sensor setiap jam
    scheduler.add_job(
        _scheduled_store_sensor_data,
        CronTrigger(minute=0, second=0),
        args=[app],
        id="store_sensor_data_hourly",
        replace_existing=True
    )

    # 📲 Notifikasi WA hanya jam tertentu
    jam_notifikasi = ['00:00', '03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00']
    for idx, jam in enumerate(jam_notifikasi):
        hour, minute = map(int, jam.split(':'))
        scheduler.add_job(
            _scheduled_notify_sensor_status,
            CronTrigger(hour=hour, minute=minute, second=10),
            args=[app],
            id=f"notify_{jam.replace(':', '')}",
            replace_existing=True
        )

    # 🧼 Penghapusan data setiap Senin jam 01:00
    scheduler.add_job(
        _scheduled_data_cleanup,
        CronTrigger(day_of_week='mon', hour=1, minute=0, second=0),
        args=[app],
        id="cleanup_old_data",
        replace_existing=True
    )

    scheduler.start()
    print("🕒 Penjadwalan aktif: simpan tiap jam, notifikasi jam tertentu, hapus mingguan.")

# 📥 Akses data manual
def get_all_water_data_service():
    return get_all_data_sensors_repository()
