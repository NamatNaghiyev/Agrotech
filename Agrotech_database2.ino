#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// === Pin təyinləri ===
#define DHTPIN 15            // DHT22 üçün pin
#define DHTTYPE DHT22        // DHT22 sensoru
#define SOIL_PIN 34          // Torpaq nəmlik sensoru (analog pin)

#define SDA_PIN 21           // I2C SDA pin
#define SCL_PIN 22           // I2C SCL pin

#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define SERVER_URL "http://<your-server-ip>/sensor_data"  // Python serverinin URL-i

// DHT22 sensorunu başlat
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BME680 bme;        // BME680 sensor obyekti

void setup() {
  Serial.begin(115200);    // Serial monitoru başlat
  dht.begin();             // DHT sensorunu başlat
  Wire.begin(SDA_PIN, SCL_PIN);  // I2C başlat

  // WiFi bağlantısı
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Bağlanır...");
  }
  Serial.println("WiFi bağlı!");

  // BME680 sensorunu başlat
  if (!bme.begin()) {
    Serial.println("❌ BME680 qoşulmayıb! Bağlantıya bax!");
    while (1);  // Qısa müddətə kodu dayanacaq
  }

  // BME680 ayarları
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // Qaz ölçümü üçün istilik

  Serial.println("Sensorlar hazırlanıb!");
}

void loop() {
  float dhtTemp = dht.readTemperature();  // DHT22 temperaturunu oxu
  float dhtHum = dht.readHumidity();     // DHT22 rütubətini oxu
  int soilValue = analogRead(SOIL_PIN);  // Torpaq nəmlik sensorunu oxu

  // BME680 sensorundan məlumat oxu
  if (!bme.performReading()) {
    Serial.println("⚠️ BME680 oxumaqda xəta!");
    delay(3000);
    return;
  }

  float bmeTemp = bme.temperature;      // BME680 temperaturu
  float bmeHum = bme.humidity;          // BME680 rütubəti
  float pressure = bme.pressure / 100.0; // BME680 təzyiqi
  float gasRes = bme.gas_resistance / 1000.0; // BME680 qaz müqaviməti

  // Orta temperatur və rütubət hesabla
  float ortaTemp = (!isnan(dhtTemp)) ? (dhtTemp + bmeTemp) / 2.0 : bmeTemp;
  float ortaHum = (!isnan(dhtHum)) ? (dhtHum + bmeHum) / 2.0 : bmeHum;

  // HTTP serverinə məlumat göndər
  HTTPClient http;
  http.begin(SERVER_URL);  // Python serverinin URL-i

  http.addHeader("Content-Type", "application/json");

  String payload = "{\"temperature\": " + String(ortaTemp) +
                   ", \"humidity\": " + String(ortaHum) +
                   ", \"soil_moisture\": " + String(soilValue) + "}";

  int httpResponseCode = http.POST(payload);  // Məlumatı göndər

  if (httpResponseCode > 0) {
    Serial.println("Məlumat serverə göndərildi!");
  } else {
    Serial.println("Məlumat göndərilmədi!");
  }

  http.end();

  delay(5000);  // 5 saniyəlik gecikmə
}
