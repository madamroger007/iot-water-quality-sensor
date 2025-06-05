from app import db
from app.src.utils.get_timezone import get_timezone

class DataSensor(db.Model):
    __tablename__ = 'data_sensor'
    
    id = db.Column(db.Integer, primary_key=True)
    ph = db.Column(db.Float)
    tds = db.Column(db.Float)
    suhu = db.Column(db.Float)
    turbidity = db.Column(db.Float)
    kelayakan = db.Column(db.Boolean, default=False)
    dibuat_sejak = db.Column(db.DateTime(timezone=True), default=get_timezone)
