#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <DHT.h>

// === Pin təyinləri ===
#define DHTPIN 15            // DHT22 üçün pin
#define DHTTYPE DHT22        // DHT22 sensoru
#define SOIL_PIN 34          // Torpaq nəmlik sensoru (analog pin)

#define SDA_PIN 21           // I2C SDA pin
#define SCL_PIN 22           // I2C SCL pin

// DHT22 sensorunu başlat
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BME680 bme;        // BME680 sensor obyekti

void setup() {
  Serial.begin(115200);    // Serial monitoru başlat
  dht.begin();             // DHT sensorunu başlat
  Wire.begin(SDA_PIN, SCL_PIN);  // I2C başlat

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

  // Sensor məlumatlarını Serial monitoruna yazdır
  Serial.println("\n📊 [SENSOR PANELI]");
  Serial.println("======================================");

  // 🌡️ Temperatur
  Serial.print("🌡️ Temperatur (Orta): ");
  Serial.print(ortaTemp); Serial.print(" °C ");
  if (ortaTemp < 18 || ortaTemp > 30) Serial.println("⚠️ [Temperatur kritik!]");
  else Serial.println("✅");

  // 💧 Rütubət (Orta)
  Serial.print("💧 Rütubət (Orta): ");
  Serial.print(ortaHum); Serial.print(" % ");
  if (ortaHum < 30 || ortaHum > 60) Serial.println("⚠️ [Rütubət normadan çıxıb!]");
  else Serial.println("✅");

  // 🌱 Torpaq nəmlik
  Serial.print("🌱 Torpaq nəmlik (ADC): ");
  Serial.print(soilValue);
  if (soilValue < 1000) Serial.println(" ⚠️ [Torpaq çox yaşdı, suyu azaldın]");
  else if (soilValue > 3000) Serial.println(" ⚠️ [Torpaq çox qurudu, su lazımdır!]");
  else Serial.println(" ✅ [Torpaq normal]");

  // 📈 Təzyiq
  Serial.print("📈 Təzyiq: ");
  Serial.print(pressure); Serial.print(" hPa ");
  if (pressure < 950 || pressure > 1050) Serial.println("⚠️ [Təzyiq normal deyil!]");
  else Serial.println("✅");

  // 🌫️ Qaz müqaviməti
  Serial.print("🌫️ Qaz müqaviməti: ");
  Serial.print(gasRes); Serial.print(" KOhms ");
  if (gasRes < 5) Serial.println("⚠️ [Havada qaz çoxdu, pəncərə aç!]");
  else Serial.println("✅");

  Serial.println("======================================\n");

  delay(5000);  // 5 saniyəlik gecikmə
}
