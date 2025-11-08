#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <vector>  // For tracking multiple processed orders

// ---------------- Wi-Fi Credentials ----------------
#define WIFI_SSID "Ram"
#define WIFI_PASSWORD "12345678"

// ---------------- Firebase Configuration ----------------
#define FIREBASE_HOST "student-326fc-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "URVuyElbtkE5P168WQ4KH0TgkxWEXVTg88nCL12K"

// ---------------- Pins ----------------
#define BUFFER_PIN 14   // Buzzer output pin (D5 on NodeMCU)

// ---------------- Objects ----------------
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD address 0x27, 16x2 display
std::vector<String> processedOrders;  // Store processed order IDs

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BUFFER_PIN, OUTPUT);
  digitalWrite(BUFFER_PIN, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println("\n✅ WiFi Connected!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");

  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("✅ Firebase Connected!");
  lcd.setCursor(0, 1);
  lcd.print("Firebase Ready");
  delay(1000);
}

// ---------------- Loop ----------------
void loop() {
  // Try to read orders from Firebase
  if (Firebase.getJSON(fbdo, "/orders")) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, fbdo.stringData());

    if (error) {
      Serial.println("❌ JSON Parse Failed!");
      return;
    }

    // Loop through each order in Firebase
    for (JsonPair kv : doc.as<JsonObject>()) {
      JsonObject order = kv.value().as<JsonObject>();
      String orderId = order["orderId"].as<String>();
      float totalAmount = order["totalAmount"].as<float>();

      // ✅ Check if this order is already processed
      bool alreadyProcessed = false;
      for (String id : processedOrders) {
        if (id == orderId) {
          alreadyProcessed = true;
          break;
        }
      }

      // ✅ New order detected
      if (!alreadyProcessed) {
        Serial.println("\n🚨 NEW ORDER DETECTED!");
        Serial.println("Order ID: " + orderId);
        Serial.print("Total: ₹");
        Serial.println(totalAmount);

        // ✅ Display on LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Order: ");
        lcd.print(orderId);
        lcd.setCursor(0, 1);
        lcd.print("Total: Rs ");
        lcd.print(totalAmount);

        // ✅ Activate buzzer
        digitalWrite(BUFFER_PIN, HIGH);
        delay(1000);   // Buzz for 1 second
        digitalWrite(BUFFER_PIN, LOW);
        delay(300);

        // ✅ Add order ID to processed list
        processedOrders.push_back(orderId);

        // ✅ (Optional) Remove order from Firebase after processing
        // Firebase.deleteNode(fbdo, "/orders/" + orderId);

        // ✅ Prevent memory overflow (keep last 10 only)
        if (processedOrders.size() > 10) {
          processedOrders.erase(processedOrders.begin());
        }
      }
    }
  } else {
    Serial.println("⚠️ Firebase Read Failed: " + fbdo.errorReason());
  }

  delay(2000); // Check every 2 seconds
}
1
