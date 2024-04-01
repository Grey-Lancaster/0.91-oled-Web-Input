#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

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
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -4 * 3600, 60000); // NTP client for EDT timezone

bool showTimeMode = false; // Flag to keep track of "Show Time" mode
unsigned long lastSwitchTime = 0; // Last time the display switched between date and day
bool showDate = true; // Flag to determine whether to show the date or day of the week

void handleRoot() {
  server.send(200, "text/html",
    "Home of the Wizard<br>" 
    "For size 1 you have 4 lines 20 characters each line<br>" 
    "For Size 2 you can only use the first 2 lines of 10 characters each line<br>"
    "<form action=\"/display\" method=\"POST\">"
    "Font Size: <input type=\"radio\" name=\"size\" value=\"1\" checked> 1"
    " <input type=\"radio\" name=\"size\" value=\"2\"> 2<br>"
    "Line 1: <input type=\"text\" name=\"line1\" maxlength=\"20\"><br>"
    "Line 2: <input type=\"text\" name=\"line2\" maxlength=\"20\"><br>"
    "Line 3: <input type=\"text\" name=\"line3\" maxlength=\"20\"><br>"
    "Line 4: <input type=\"text\" name=\"line4\" maxlength=\"20\"><br>"
    "<input type=\"submit\" name=\"action\" value=\"Display Input\">"
    "<br>"
    "Or Show Time "
    "<input type=\"submit\" name=\"action\" value=\"Show Time\">"
    "</form>");
}

void handleDisplay() {
  if (server.arg("action") == "Show Time") {
    showTimeMode = true; // Set the flag to true
    lastSwitchTime = millis(); // Reset the timer
  } else {
    showTimeMode = false; // Set the flag to false when displaying custom text
    int size = server.arg("size").toInt();
    String line1 = server.arg("line1");
    String line2 = server.arg("line2");
    String line3 = server.arg("line3");
    String line4 = server.arg("line4");

    display.clearDisplay();
    display.setTextSize(size);
    display.setCursor(0, 0);
    display.println(line1);
    display.println(line2);
    if (size == 1) {
      display.println(line3);
      display.println(line4);
    }
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

  // Start NTP client
  timeClient.begin();

  // Set up the web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/display", HTTP_POST, handleDisplay);
  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update(); // Update the mDNS responder

  if (showTimeMode) {
    if (millis() - lastSwitchTime >= 3000) { // Switch every 3 seconds
      showDate = !showDate; // Toggle between date and day
      lastSwitchTime = millis(); // Reset the timer
    }

    timeClient.update(); // Update the time
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);

    // Get the current time
    time_t now = timeClient.getEpochTime();
    struct tm *timeinfo = localtime(&now);

    // Format time as "HH:MM:SS"
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%I:%M:%S", timeinfo);
    display.print(timeStr);

    // Display colon and AM/PM in font size 1 on the same line
    display.setTextSize(2);
    display.setCursor(100, 0); // Adjust cursor position for colon and AM/PM
    //display.print(":");
    char ampmStr[3];
    strftime(ampmStr, sizeof(ampmStr), "%p", timeinfo);
    display.println(ampmStr);

    // Switch back to font size 2 for the date or day
    display.setTextSize(2);

    // Display either the date or the day of the week
    if (showDate) {
      // Format date as "MM/DD/YYYY"
      char dateStr[11];
      strftime(dateStr, sizeof(dateStr), "%m/%d/%Y", timeinfo);
      display.println(dateStr);
    } else {
      // Format day as "Day of the Week"
      char dayStr[10];
      strftime(dayStr, sizeof(dayStr), "%A", timeinfo);
      display.println(dayStr);
    }

    display.display();
  }

  yield(); // Yield in the main loop to prevent watchdog timer resets
}
