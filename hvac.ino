#include <WiFiNINA.h>

// WiFi and InfluxDB credentials
const char ssid[] = "";
const char password[] = "";
const char server[] = "";
const int port = 8086; // default InfluxDB v2 port is 8086
const char org[] = "home";
const char bucket[] = "bucket";
const char token[] = "";

const float ADC_mV = 4.8828125;
const float SensorOffset = 200.0;
const float sensitivity = 4.413;
const float mmh2O_cmH2O = 10;
const float mmh2O_kpa = 0.00981;

unsigned long previousMillis = 0;
const unsigned long interval = 500;

#define NUM_READINGS 20
float readings[NUM_READINGS]; // the readings from the analog input
int readIndex = 0; // the index of the current reading
float total = 0; // the running total
float average = 0; // the average

WiFiClient wifiClient;

void setup() {
  Serial.begin(115200);

  WiFi.end();
  delay(1000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi...");
    Serial.print(" Status: ");
    Serial.println(WiFi.status());
  }
  Serial.println("Connected to WiFi");

  // initialize all the readings to 0
  for (int thisReading = 0; thisReading < NUM_READINGS; thisReading++) {
    readings[thisReading] = 0;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float sensorValue = (analogRead(A0) * ADC_mV - SensorOffset) / sensitivity;

    total = total - readings[readIndex];
    readings[readIndex] = sensorValue;
    total = total + readings[readIndex];
    readIndex = readIndex + 1;

    if (readIndex >= NUM_READINGS) {
      readIndex = 0;
    }

    average = total / NUM_READINGS;

    Serial.print(millis());
    Serial.print(" , ");
    Serial.println(average, 2);

    sendDataToInfluxDB(average);
  }
}

void sendDataToInfluxDB(float average) {
  if (wifiClient.connect(server, port)) {
    String postData = String("Pressure_Sensor,host=arduino_mkr1010 average=") + String(average);
    String postRequest =
      "POST /api/v2/write?org=" + String(org) + "&bucket=" + String(bucket) + "&precision=s HTTP/1.1\r\n" +
      "Host: " + String(server) + "\r\n" +
      "Authorization: Token " + String(token) + "\r\n" +
      "Content-Length: " + String(postData.length()) + "\r\n" +
      "Content-Type: text/plain\r\n" +
      "Connection: close\r\n\r\n" +
      postData;

    wifiClient.println(postRequest);

    // Don't wait for the server's response and close the client immediately
    wifiClient.stop();
  } else {
    Serial.println("Failed to connect to InfluxDB server");
  }
}
