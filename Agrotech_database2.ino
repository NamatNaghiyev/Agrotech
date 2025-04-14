#include <Wire.h>                     // I2C üçün kitabxana (BME680 sensoru üçün gərəkdi)
#include <Adafruit_Sensor.h>          // Adafruit sensor kitabxanası
#include <Adafruit_BME680.h>          // BME680 sensor üçün xüsusi kitabxana
#include <DHT.h>                      // DHT sensor kitabxanası (temperatur və rütubət üçün)
#include <WiFi.h>                     // WiFi bağlantısı üçün
#include <HTTPClient.h>               // HTTP request göndərmək üçün

// === Pin təyinləri ===
#define DHTPIN 15                     // DHT22 sensoru 15-ci pinə qoşulub
#define DHTTYPE DHT22                 // İstifadə etdiyimiz DHT sensor tipi — DHT22
#define SOIL_PIN 35                   // Torpaq nəmlik sensoru bu pinə qoşulub
#define SDA_PIN 21                    // I2C üçün SDA (data) pini
#define SCL_PIN 22                    // I2C üçün SCL (clock) pini

// === WiFi və Server məlumatları ===
#define WIFI_SSID "Smart"             // WiFi adı
#define WIFI_PASSWORD "1453571632"    // WiFi şifrə
#define SERVER_URL "http://79.133.182.96:9090/api/store"  // Məlumatın göndərildiyi server
#define PF "{\"temperature\": \"%.2f\", \"humidity\": \"%.2f\", \"soil\": \"%d\", \"bme_temperature\": \"%.2f\", \"bme_humidity\": \"%.2f\", \"pressure\": \"%.2f\", \"gas_resistance\": \"%.2f\", \"avg_temperature\": \"%.2f\", \"avg_humidity\": \"%.2f\"}"


// === Sensor obyektləri ===
DHT dht(DHTPIN, DHTTYPE);            // DHT sensor obyekti yaradılır
Adafruit_BME680 bme;                 // BME680 sensor obyekti

// === Məlumatların saxlanması üçün dəyişənlər ===
float dhtTemp, dhtHum, bmeTemp, bmeHum, pressure, gasRes, ortaTemp, ortaHum;
int soilValue;                       // Torpaq sensorundan gələn analog dəyər

void setup() {
  Serial.begin(115200);              // Serial monitor üçün sürət
  dht.begin();                       // DHT sensoru işə salırıq
  Wire.begin(SDA_PIN, SCL_PIN);      // I2C başlat (BME680 üçün)

  // === WiFi bağlantısı ===
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Bağlanır...");   // WiFi-ə qoşulana qədər yazır
  }
  Serial.println("WiFi bağlı!");     // WiFi bağlantısı uğurludur

  // === BME680 sensorunun yoxlanılması ===
  if (!bme.begin()) {
    Serial.println("❌ BME680 qoşulmayıb! Bağlantıya bax!"); // Sensor tapılmadısa, error
    while (1);                    // Dayanır, çünki sensor vacibdir
  }

  // BME680 sensorunun parametrləri
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);     // Qaz sensor üçün istilik və zaman

  Serial.println("Sensorlar hazırlanıb!"); // Hər şey qaydasındadır
}

void loop() {
  // === DHT22 sensorundan temperatur və rütubət oxunur ===
  dhtTemp = dht.readTemperature();     
  dhtHum = dht.readHumidity();         

  // === Torpaq nəmlik sensorundan dəyər oxunur ===
  soilValue = analogRead(SOIL_PIN);    

  // === BME680 sensorundan məlumat oxumaq ===
  if (!bme.performReading()) {
    Serial.println("BME680 oxuma problemi!");  // Problem olarsa çıxır
    return;
  }

  // === BME680 məlumatlarını dəyişənlərə yazırıq ===
  bmeTemp = bme.temperature;
  bmeHum = bme.humidity;
  pressure = bme.pressure / 100.0;          // Pa → hPa
  gasRes = bme.gas_resistance / 1000.0;     // Ohm → KOhm

  // === Orta temperatur və rütubət hesablanır ===
  ortaTemp = (dhtTemp + bmeTemp) / 2;
  ortaHum = (dhtHum + bmeHum) / 2;

  // === Temperaturun vəziyyətini yoxlayırıq ===
  Serial.print("🌡 Temperatur (Orta): ");
  Serial.print(ortaTemp); Serial.print(" °C ");
  if (ortaTemp < 10) {
    Serial.println("⚠ Çox soyuq: Toxum çürüyə bilər, cücərməz");
  } else if (ortaTemp >= 10 && ortaTemp < 15) {
    Serial.println("⚠ Soyuq: Bitkilər yaxşı böyüməz");
  } else if (ortaTemp >= 15 && ortaTemp < 25) {
    Serial.println("✅ Əla temperatur: Bitkilər mükəmməl böyüyər");
  } else if (ortaTemp >= 25 && ortaTemp <= 30) {
    Serial.println("⚠ Normal temperatur: Bitkilər böyüyür amma diqqətli ol");
  } else if (ortaTemp > 30 && ortaTemp <= 35) {
    Serial.println("⚠ Çox isti: Kök sistemi yanar, bitki quruyar");
  } else {
    Serial.println("❌ Çox isti: Hər şey yandı! Temperatur təhlükəlidi!");
  }

  // === Rütubətin vəziyyəti ===
  float h = dht.readHumidity();
  String humidityStatus;

  if (h < 30) {
    humidityStatus = "Çox quru (0-30%) - Rütubət çox aşağıdır, bu halda bitkilər üçün kifayət qədər su təmin olunmur. Bitkilərin su itirməsi risklidir.";
  } else if (h >= 30 && h < 50) {
    humidityStatus = "Normal quru (30-50%) - Rütubət səviyyəsi orta səviyyədədir. Bitkilər üçün ideal şəraitdə deyil, amma hələ də böyüməyə imkan verir.";
  } else if (h >= 50 && h < 70) {
    humidityStatus = "Normal (50-70%) - Bitkilər üçün əla şərait. Bu rütubət səviyyəsi, əksər bitkilərin yaxşı inkişaf etməsi üçün idealdır.";
  } else if (h >= 70 && h < 90) {
    humidityStatus = "Yüksək rutubət (70-90%) - Çox yüksək rütubət bitkilərin böyüməsini mənfi təsir edə bilər. Bəzi bitkilər rutubətin yüksək olduğu mühitlərdə inkişaf etməyə çətinlik çəkir.";
  } else {
    humidityStatus = "Çox yüksək rutubət (90-100%) - Bitkilərin kök sistemi zədələnə bilər. Çox yüksək rütubət, bəzi bitkilərdə çürüməyə səbəb ola bilər.";
  }

  Serial.print("🌱 Rütubət: ");
  Serial.print(h); Serial.print("% - ");
  Serial.println(humidityStatus);

  // === Torpaq nəmlik səviyyəsi şərh edilir ===
  Serial.print("🌱 Torpaq nəmlik (ADC): ");
  Serial.print(soilValue);
  if (soilValue < 800) {
    Serial.println(" ✅ Torpaq çox yaşdır");
  } else if (soilValue < 1400) {
    Serial.println(" ✅ Torpaq yaşdır");
  } else if (soilValue < 2000) {
    Serial.println(" ✅ Torpaq normaldır");
  } else if (soilValue < 2400) {
    Serial.println(" ✅ Torpaq quru, sulamağa ehtiyac var");
  } else {
    Serial.println(" ⚠ [Sensor çıxıb!]");  // ADC çox böyük dəyər verirsə — problem var
  }

  // === Təzyiqə əsasən hava şəraiti proqnozu ===
  Serial.print("📈 Təzyiq: ");
  Serial.print(pressure); Serial.print(" hPa ");
  if (pressure < 1000) {
    Serial.println("⚠ Aşağı təzyiq — yağış, külək, qasırğa ola bilər");
  } else if (pressure >= 1000 && pressure < 1013) {
    Serial.println("🌦 Orta təzyiq — hava dəyişkən ola bilər");
  } else if (pressure == 1013) {
    Serial.println("✅ Normal təzyiq — standart atmosfer təzyiqi");
  } else if (pressure > 1013 && pressure <= 1025) {
    Serial.println("🌞 Yüksək təzyiq — yaxşı hava");
  } else {
    Serial.println("⚠ Çox yüksək təzyiq — yağışsız");
  }

  // === Gas Resistance dəyərini Serial Monitor-da göstəririk ===
  Serial.print("💨 Gas Resistance: ");
  Serial.print(gasRes); Serial.print(" KOhm ");

  if (gasRes > 40) {
    Serial.println("✅ Hava çox təmizdir — rural (kənd) səviyyəsi");
  } else if (gasRes > 20 && gasRes <= 40) {
    Serial.println("✅ Təmiz hava — şəhər ətrafı səviyyə");
  } else if (gasRes > 10 && gasRes <= 20) {
    Serial.println("⚠ Orta hava keyfiyyəti — şəhər havası, avtomobil qazı ola bilər");
  } else if (gasRes > 5 && gasRes <= 10) {
    Serial.println("❌ Pis hava — sənaye qazları və VOC çirkliliyi yüksəkdir");
  } else if (gasRes <= 5) {
    Serial.println("💀 AĞIR ZƏHƏRLİ MÜHİT — qaz sızması, ciddi təhlükə!!!");
  } else {
    Serial.println("❓ Naməlum qaz müqaviməti dəyəri");
  }

  // === Data göndərilməsi ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // URL bağlantısı
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    String payload = String("{\"temperature\": \"") + ortaTemp + String("\", \"humidity\": \"") + ortaHum + String("\", \"soil\": \"") + soilValue + String("\", \"bme_temperature\": \"") + bmeTemp + String("\", \"bme_humidity\": \"") + bmeHum + String("\", \"pressure\": \"") + pressure + String("\", \"gas_resistance\": \"") + gasRes + String("\"}");

    // HTTP POST sorğusu göndərmək
    int httpCode = http.POST(payload);

    // Status kodunu Serial Monitor-da göstərmək
    if (httpCode > 0) {
      Serial.println("Məlumat göndərildi!");
    } else {
      Serial.println("HTTP sorğusu göndərilərkən xəta baş verdi!");
    }

    // Bağlantıyı kəsirik
    http.end();
  } else {
    Serial.println("WiFi ilə bağlı problem var");
  }

  // 10 saniyəlik gözləmə
  delay(10000); 
}
