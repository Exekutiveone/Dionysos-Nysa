from flask import Flask, request, jsonify, render_template
import sqlite3

app = Flask(__name__)

# SQLite-Datenbankinitialisierung
DATABASE = 'sensor_data.db'

def create_connection():
    conn = sqlite3.connect(DATABASE)
    return conn

def create_table():
    conn = create_connection()
    with conn:
        conn.execute('''
            CREATE TABLE IF NOT EXISTS sensor_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                temperature REAL,
                pressure REAL,
                humidity REAL,
                weight1 INTEGER,
                weight2 INTEGER,
                weight3 INTEGER,
                digital1 INTEGER,
                analog1 INTEGER,
                digital2 INTEGER,
                analog2 INTEGER,
                digital3 INTEGER,
                analog3 INTEGER,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        ''')

create_table()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/sensor_data')
def sensor_data_page():
    return render_template('sensor_data.html')

@app.route('/update_data', methods=['POST'])
def update_data():
    data = request.json
    print("Empfangene Sensordaten:", data)
    save_to_db(data)
    return jsonify({"status": "success"})

def save_to_db(data):
    conn = create_connection()
    with conn:
        conn.execute('''
            INSERT INTO sensor_data (temperature, pressure, humidity, weight1, weight2, weight3, digital1, analog1, digital2, analog2, digital3, analog3)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (data['temperature'], data['pressure'], data['humidity'],
              data['weight1'], data['weight2'], data['weight3'],
              data['digital1'], data['analog1'], data['digital2'],
              data['analog2'], data['digital3'], data['analog3']))

@app.route('/get_sensor_data', methods=['GET'])
def get_sensor_data():
    conn = create_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM sensor_data')
    rows = cursor.fetchall()
    columns = [column[0] for column in cursor.description]
    # Achte darauf, dass der Zeitstempel auch in der Datenbank vorhanden ist
    return jsonify([dict(zip(columns, row)) for row in rows])


@app.route('/front_end', methods=['GET'])
def update_data_front_end():
    return jsonify({
        "temperature": 0.0,
        "pressure": 0.0,
        "humidity": 0.0,
        "weight1": 0,
        "weight2": 0,
        "weight3": 0,
        "digital1": 0,
        "analog1": 0,
        "digital2": 0,
        "analog2": 0,
        "digital3": 0,
        "analog3": 0
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)
