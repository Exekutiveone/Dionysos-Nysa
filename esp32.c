#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <HX711.h>
#include <ArduinoOTA.h> // OTA-Bibliothek

// WiFi-Anmeldeinformationen
const char* ssid = "Super-Pevesi";
const char* password = "v843W#AL#JQy!Uj";
const char* OTA_Password = "your_ota_password_here"; // OTA Passwort
// Server-URL für externe Datenübertragung (z.B. Sensordaten an Flask-Server)
const char* serverUrl = "http://192.168.2.190:5000";  // Flask-Server-IP

// Waage 1
const int DOUT_PIN1 = 15;
const int SCK_PIN1 = 4;
HX711 scale1;

// Waage 2
const int DOUT_PIN2 = 16;
const int SCK_PIN2 = 17;
HX711 scale2;

// Waage 3
const int DOUT_PIN3 = 5;
const int SCK_PIN3 = 18;
HX711 scale3;

float calibration_factor1 = 95;
float calibration_factor2 = 95;
float calibration_factor3 = 95;
const float known_weight = 1000.0; // Bekanntes Gewicht: 1000g

// BME280 Sensor
Adafruit_BME280 bme;

// I2C Pins
const int sdaPinBME = 23;
const int sclPinBME = 22;

// Sensorwerte
float temperature = 0.0;
float pressure = 0.0;
float humidity = 0.0;

// HD-38 Sensor Pins
const int digitalPin1 = 14;
const int analogPin1 = 34;
const int digitalPin2 = 26;
const int analogPin2 = 35;
const int digitalPin3 = 33;
const int analogPin3 = 32;

// Motor Steuerpins
const int motor1Pin1 = 3;
const int motor2Pin1 = 21;
const int motor3Pin1 = 19;

// Servo Pins
const int servoPin1 = 13;
const int servoPin2 = 12;

// Servo Objekte
Servo servo1;
Servo servo2;

void setup() {
  Serial.begin(115200);
  Serial.println("Initialisiere alle Waagen...");

  // Verbindung mit WiFi herstellen
  WiFi.begin(ssid, password);
  Serial.print("Versuche, mit dem WiFi-Netzwerk zu verbinden");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts <500) { // 10 Versuche
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nErfolgreich mit dem WiFi-Netzwerk verbunden");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFehler beim Verbinden mit dem WiFi-Netzwerk");
    while (1); // Stoppt die Ausführung, wenn die Verbindung fehlschlägt
  }

  // OTA-Setup
  ArduinoOTA.setHostname("ESP32_Device"); // Setze den Hostnamen für OTA
  ArduinoOTA.setPassword("OTA_Password"); // Setze das Passwort für OTA

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else if (ArduinoOTA.getCommand() == U_SPIFFS) {
      type = "filesystem";
    }
    Serial.println("OTA Update gestartet: " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update abgeschlossen.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Update Fortschritt: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Update Fehler [%u]: ", error);
    switch (error) {
      case OTA_AUTH_ERROR:
        Serial.println("Authentifizierungsfehler");
        break;
      case OTA_BEGIN_ERROR:
        Serial.println("Begin-Fehler");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("Verbindungsfehler");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("Empfangsfehler");
        break;
      case OTA_END_ERROR:
        Serial.println("Abschlussfehler");
        break;
    }
  });
  ArduinoOTA.begin();

  // Waage 1
  scale1.begin(DOUT_PIN1, SCK_PIN1);
  Serial.println("Waage 1 initialisiert.");
  
  // Waage 2
  scale2.begin(DOUT_PIN2, SCK_PIN2);
  Serial.println("Waage 2 initialisiert.");
  
  // Waage 3
  scale3.begin(DOUT_PIN3, SCK_PIN3);
  Serial.println("Waage 3 initialisiert.");
  
  delay(1000);
  
  // Kalibrierung der Waagen
  Serial.println("Kalibrierung beginnt...");
  calibrateScale(scale1, 1, calibration_factor1);
  calibrateScale(scale2, 2, calibration_factor2);
  calibrateScale(scale3, 3, calibration_factor3);

  // HD-38 Pins als Eingänge konfigurieren
  pinMode(digitalPin1, INPUT);
  pinMode(analogPin1, INPUT);
  pinMode(digitalPin2, INPUT);
  pinMode(analogPin2, INPUT);
  pinMode(digitalPin3, INPUT);
  pinMode(analogPin3, INPUT);

  // Motor Pins als Ausgang konfigurieren
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor3Pin1, OUTPUT);

  // Servos initialisieren
  servo1.setPeriodHertz(50);  // Standardfrequenz für Servos
  servo1.attach(servoPin1);
  servo2.setPeriodHertz(50);
  servo2.attach(servoPin2);

  // Initialisiere I2C-Bus-Pins für BME280
  Wire.begin(sdaPinBME, sclPinBME);
  if (!bme.begin(0x76)) {
    Serial.println("BME280 Sensor nicht gefunden auf Pins 23/22. Bitte überprüfen Sie die Verbindung.");
    while (1); // Stopp die Ausführung, wenn der Sensor nicht gefunden wird
  }
  Serial.println("BME280 Sensor initialisiert.");
}

void loop() {
  // OTA-Handling
  ArduinoOTA.handle();

  // Ausgabe der gemessenen Werte
  temperature = bme.readTemperature();
  pressure = bme.readPressure() / 100.0;
  humidity = bme.readHumidity();

  float weight1 = scale1.get_units();
  float weight2 = scale2.get_units();
  float weight3 = scale3.get_units();
  int digitalValue1 = digitalRead(digitalPin1);
  int analogValue1 = analogRead(analogPin1);
  int digitalValue2 = digitalRead(digitalPin2);
  int analogValue2 = analogRead(analogPin2);
  int digitalValue3 = digitalRead(digitalPin3);
  int analogValue3 = analogRead(analogPin3);

  Serial.print("Gewicht Waage 1: ");
  Serial.print(weight1);
  Serial.println(" g");

  Serial.print("Gewicht Waage 2: ");
  Serial.print(weight2);
  Serial.println(" g");

  Serial.print("Gewicht Waage 3: ");
  Serial.print(weight3);
  Serial.println(" g");

  Serial.print("Temperatur: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Luftdruck: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("Luftfeuchtigkeit: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Digitalwert 1: ");
  Serial.println(digitalValue1);
  Serial.print("Analogwert 1: ");
  Serial.println(analogValue1);

  Serial.print("Digitalwert 2: ");
  Serial.println(digitalValue2);
  Serial.print("Analogwert 2: ");
  Serial.println(analogValue2);

  Serial.print("Digitalwert 3: ");
  Serial.println(digitalValue3);
  Serial.print("Analogwert 3: ");
  Serial.println(analogValue3);

  Serial.println("------------------------");
  // Sensordaten lesen und an den Server senden
  readAndSendSensorData();

  // Steuerbefehle vom Server abrufen
  checkForCommands();


  delay(5000);  // Alle 5 Sekunden Daten ausgeben
}

void calibrateScale(HX711 &scale, int scaleNumber, float &calibration_factor) {
  Serial.print("Kalibriere Waage ");
  Serial.println(scaleNumber);

  // Rohwert ermitteln ohne Gewicht
  long zero_value = scale.read_average();
  scale.set_offset(zero_value);
  
  Serial.println("Bitte das bekannte Gewicht auf die Waage legen.");
  delay(5000);  // 5 Sekunden warten, um das Gewicht zu platzieren

  // Durchschnittswert mit Gewicht
  long value_with_weight = scale.read_average();
  calibration_factor = (value_with_weight - zero_value) / known_weight;
  scale.set_scale(calibration_factor);

  Serial.print("Kalibrierungsfaktor Waage ");
  Serial.print(scaleNumber);
  Serial.print(": ");
  Serial.println(calibration_factor);

  Serial.println("Kalibrierung abgeschlossen.");
}

void readAndSendSensorData() {
  // Lese BME280 Sensordaten
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0;
  float humidity = bme.readHumidity();

  // Lese HD-38 Sensordaten
  float weight1 = scale1.get_units();
  float weight2 = scale2.get_units();
  float weight3 = scale3.get_units();
  int digitalValue1 = digitalRead(digitalPin1);
  int digitalValue2 = digitalRead(digitalPin2);
  int analogValue1 = analogRead(analogPin1);
  int analogValue2 = analogRead(analogPin2);
  int analogValue3 = analogRead(analogPin3);
  int digitalValue3 = digitalRead(digitalPin3);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(serverUrl) + "/update_data");
    http.addHeader("Content-Type", "application/json");

    // Erstelle das JSON-Payload mit allen analogen Werten auf einmal
    String jsonPayload = "{\"temperature\": " + String(temperature) + 
                         ", \"pressure\": " + String(pressure) + 
                         ", \"humidity\": " + String(humidity) + 
                         ", \"weight1\": " + String(weight1) +
                         ", \"weight2\": " + String(weight2) +
                         ", \"weight3\": " + String(weight3) +
                         ", \"digital1\": " + String(digitalValue1) + 
                         ", \"digital2\": " + String(digitalValue2) + 
                         ", \"analog1\": " + String(analogValue1) + 
                         ", \"analog2\": " + String(analogValue2) + 
                         ", \"analog3\": " + String(analogValue3) + 
                         ", \"digital3\": " + String(digitalValue3) + "}";

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server Antwort: " + response);
    } else {
      Serial.println("Fehler bei der HTTP-Anfrage: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi nicht verbunden");
  }
}



void checkForCommands() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(serverUrl) + "/get_commands");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Befehle erhalten: " + payload);

      // JSON-Dokument erstellen und deserialisieren
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Fehler beim Parsen der JSON-Daten: ");
        Serial.println(error.c_str());
        return;
      }

      // Motorsteuerung
      if (doc["motor1"] == "on") {
        Serial.println("Motor 1 einschalten");
        digitalWrite(motor1Pin1, HIGH);
      } else if (doc["motor1"] == "off") {
        Serial.println("Motor 1 ausschalten");
        digitalWrite(motor1Pin1, LOW);
      }

      if (doc["motor2"] == "on") {
        Serial.println("Motor 2 einschalten");
        digitalWrite(motor2Pin1, HIGH);
      } else if (doc["motor2"] == "off") {
        Serial.println("Motor 2 ausschalten");
        digitalWrite(motor2Pin1, LOW);
      }
      
      if (doc["motor3"] == "on") {
        Serial.println("Motor 3 einschalten");
        digitalWrite(motor3Pin1, HIGH);
      } else if (doc["motor3"] == "off") {
        Serial.println("Motor 3 ausschalten");
        digitalWrite(motor3Pin1, LOW);
      }

      // Servosteuerung
      int servo1Pos = doc["servo1"];
      int servo2Pos = doc["servo2"];

      Serial.println("Servo 1 auf Position: " + String(servo1Pos));
      servo1.write(servo1Pos);

      Serial.println("Servo 2 auf Position: " + String(servo2Pos));
      servo2.write(servo2Pos);

    } else {
      Serial.println("Fehler bei der HTTP-Anfrage: " + String(httpResponseCode));
    }

    http.end();
  }
}
