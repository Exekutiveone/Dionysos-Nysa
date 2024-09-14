from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

# Globale Variablen für die Sensordaten
sensor_data = {
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
}

# Globale Variablen für Steuerbefehle
commands = {
    "motor1": "off",
    "motor2": "off",
    "motor3": "off",  # Neuer Motor 3 hinzugefügt
    "servo1": 0,
    "servo2": 0
}

@app.route('/')
def index():
    return render_template('index.html', sensor_data=sensor_data, commands=commands)

@app.route('/update_data', methods=['POST'])
def update_data():
    global sensor_data
    data = request.json
    sensor_data = data
    print("Empfangene Sensordaten:", sensor_data)
    return jsonify({"status": "success"})

@app.route('/get_commands', methods=['GET'])
def get_commands():
    print("Sende Befehle an ESP32:", commands)
    return jsonify(commands)

@app.route('/set_motor/<motor>/<state>', methods=['POST'])
def set_motor(motor, state):
    if motor in commands and state in ["on", "off"]:
        commands[motor] = state
        print(f"Motor {motor} auf {state} gesetzt")
        return jsonify({"status": "success", "command": commands}), 200
    else:
        print("Fehler: Ungültiger Befehl für Motorsteuerung")
        return jsonify({"status": "error", "message": "Invalid command"}), 400

@app.route('/set_servo/<servo>/<int:position>', methods=['POST'])
def set_servo(servo, position):
    if servo in commands and 0 <= position <= 180:
        commands[servo] = position
        print(f"Servo {servo} auf Position {position} gesetzt")
        return jsonify({"status": "success", "command": commands}), 200
    else:
        print("Fehler: Ungültiger Befehl für Servosteuerung")
        return jsonify({"status": "error", "message": "Invalid command"}), 400

@app.route('/front_end', methods=['GET'])
def update_data_front_end():
    # Return the sensor data as a JSON response
    return jsonify(sensor_data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)



