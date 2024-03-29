#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Wi-Fi credentials
const char* ssid = "shop2";
const char* password = "mine0313";

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
Adafruit_SSD1306 display(128, 32, &Wire, OLED_RESET);

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html",
    "<form action=\"/display\" method=\"POST\">"
    "Line 1: <input type=\"text\" name=\"line1\" maxlength=\"20\"><br>"
    "Line 2: <input type=\"text\" name=\"line2\" maxlength=\"20\"><br>"
    "Line 3: <input type=\"text\" name=\"line3\" maxlength=\"20\"><br>"
    "Line 4: <input type=\"text\" name=\"line4\" maxlength=\"20\"><br>"
    "<input type=\"submit\" value=\"Display\">"
    "</form>");
}

void handleDisplay() {
  if (server.hasArg("line1") && server.hasArg("line2") && server.hasArg("line3") && server.hasArg("line4")) {
    String line1 = server.arg("line1");
    String line2 = server.arg("line2");
    String line3 = server.arg("line3");
    String line4 = server.arg("line4");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(line1);
    display.println(line2);
    display.println(line3);
    display.println(line4);
    display.display();
  }
  server.send(200, "text/plain", "Text displayed!");
}

void setup() {
  Serial.begin(115200);

  // Start I2C communication for OLED
  Wire.begin(D2, D5); // GPIO4, GPIO14 respectively sda on D2 sck on D5
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  display.print("Connecting...");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // This already yields to the system
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connected to:");
  display.println(WiFi.SSID());
  display.println("IP Address:");
  display.println(WiFi.localIP());
  display.display();

  // Start mDNS with the hostname "oled"
  if (!MDNS.begin("oled")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }

  // Set up the web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/display", HTTP_POST, handleDisplay);
  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update(); // Update the mDNS responder
  yield(); // Yield in the main loop to prevent watchdog timer resets
}
