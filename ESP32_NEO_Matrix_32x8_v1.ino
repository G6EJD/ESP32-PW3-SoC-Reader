#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Wifi connection  credentials
char ssid[]     = "yourSSID";      // your network SSID (name)
char password[] = "yourPASSWORD";  // your network key

#define UpdateTime 10 // 10-mins
#define PIN 16
float SoC;
unsigned long previousMillis, currentMillis;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT +
NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
NEO_GRB + NEO_KHZ400);

const uint16_t colors[] = {
  matrix.Color(204, 0, 204), matrix.Color(204, 204, 0), matrix.Color(0, 255, 255),
  matrix.Color(255, 10, 127), matrix.Color(127, 0, 255), matrix.Color(0, 255, 0),
  matrix.Color(255, 99, 255)};

void setup() {
  Serial.begin(115200);
  Serial.println(__FILE__);
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(10);
  matrix.setTextColor(colors[0]);
  matrix.setRotation(2);
  StartWiFi();
  GetBatteryData();
  previousMillis = millis();
  currentMillis = millis();
}

void loop() {
  matrix.fillScreen(0);    
  matrix.setCursor(x, 0);
  matrix.print("SoC=" + String(SoC,1) + "%");
  if( --x < -96 ) {
    x = matrix.width();
    if(++pass >= 8) pass = 0;
    matrix.setTextColor(colors[pass]);
  }
  matrix.show();
  delay(80);
  currentMillis = millis();
  if (currentMillis - previousMillis >= 60000 * UpdateTime) {  // every 10-mins
    GetBatteryData();
    previousMillis = currentMillis;
  }
}

// Support functions/procedures

void StartWiFi() {
  Serial.println("\r\nConnecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection) {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000) { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED) {
    Serial.println("\n\nWiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("\n\nWiFi connection *** FAILED ***");
}

void GetBatteryData() {
  WiFiClientSecure client;
  HTTPClient http;
  String response;
  int httpResponseCode;
  Serial.println("Getting Battery data...");
  String site_id = "yourSerial";   // From Netzero information screen
  String api_token = "yourAPIKey"; // From Netzero App
  String config = "{operational_mode: autonomous}";
  String url= "https://api.netzero.energy/api/v1/" + site_id + "/config";
  String Auth = "Bearer " + api_token;
  String Content = "application/json";
  String Accept = "application/json";
  String Payload = "";
  http.begin(url);  //Specify destination for HTTP request
  http.addHeader("Authorization", Auth);
  http.addHeader("Content-type", Content);
  http.addHeader("Accept", Accept);
  httpResponseCode = http.GET();
  Serial.println("GOT: " + String(httpResponseCode));
  if (httpResponseCode >= 200) {
    String response = http.getString();  //Get the response to the request
    DecodePlant(response);
  } else reportHTTP_Error(httpResponseCode);
  client.stop();
  http.end();
}

void reportHTTP_Error(int httpErrorCode) {
  Serial.print("Error on sending POST: " + String(httpErrorCode));
}

void DecodePlant(String input) {
  Serial.println("Decoding Battery Data...");
  Serial.println(input);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }
  SoC = doc["percentage_charged"]; // 89
  Serial.println("SoC = " + String(SoC,1) + "%");
}
