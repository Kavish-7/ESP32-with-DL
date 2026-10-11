#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// OLED pins
#define OLED_SDA 21
#define OLED_SCL 22

// =====================================================
// WIFI
// =====================================================

const char* ssid = "MARY THIRD";
const char* password = "MARYT@1234";
// =====================================================
// NETWORK
// =====================================================

// ESP32 DEV MODULE
IPAddress local_IP(192, 168, 0, 158);

IPAddress gateway(192, 168, 0, 1);

IPAddress subnet(
  255,
  255,
  252,
  0
);

IPAddress primaryDNS(
  192,
  168,
  8,
  1
);

IPAddress secondaryDNS(
  8,
  8,
  8,
  8
);

// =====================================================
// OTHER DEVICES
// =====================================================

// ESP32-CAM
const char* CAMERA_IP = "192.168.0.182";
// Laptop
const char* LAPTOP_IP = "192.168.0.3";

// Flask AI
const char* FLASK_URL =
  "http://192.168.0.3:5000/camera";
// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);

// =====================================================
// PINS
// =====================================================

// Conveyor MOSFET
#define BELT_PIN 25

// Servo
#define SERVO_PIN 26

// IR sensor
#define IR_SENSOR_PIN 27

// =====================================================
// SERVO
// =====================================================

Servo sortingServo;

// Five sorting positions

#define HDPE_ANGLE  0
#define OTHER_ANGLE -72
#define PET_ANGLE 144
#define PP_ANGLE 216
#define PVC_ANGLE 288
// =====================================================
// VARIABLES
// =====================================================

bool beltState = false;

bool processingObject = false;

// One-shot sensor lock:
// false = the current object has already triggered.
// It becomes true only after the sensor is clear again.
bool sensorReadyForObject = true;

unsigned long lastDetectionTime = 0;

// Extra debounce/cooldown for sensor noise.
const unsigned long DETECTION_COOLDOWN = 1000;

// Sensor must remain CLEAR for this long before another
// object is allowed to trigger.
const unsigned long SENSOR_CLEAR_TIME = 300;

unsigned long sensorClearStart = 0;

int currentServoAngle = 90;

// =====================================================
// OLED DISPLAY FUNCTION
// =====================================================

void oledClear()
{
  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );
}

// =====================================================
// OLED MESSAGE
// =====================================================

void oledMessage(
  String line1,
  String line2,
  String line3,
  String line4
)
{
  oledClear();

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    line1
  );

  display.setCursor(
    0,
    16
  );

  display.println(
    line2
  );

  display.setCursor(
    0,
    32
  );

  display.println(
    line3
  );

  display.setCursor(
    0,
    48
  );

  display.println(
    line4
  );

  display.display();
}

// =====================================================
// OLED STARTUP
// =====================================================

void oledStartup()
{
  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  )
  {
    Serial.println(
      "OLED initialization FAILED!"
    );

    return;
  }

  Serial.println(
    "OLED initialized successfully."
  );

  oledMessage(
    "ReactX",
    "Smart Plastic",
    "Sorting System",
    "Starting..."
  );

  delay(2000);
}

// =====================================================
// BELT ON
// =====================================================

void beltON()
{
  digitalWrite(
    BELT_PIN,
    HIGH
  );

  beltState = true;

  Serial.println(
    "BELT: ON"
  );
}

// =====================================================
// BELT OFF
// =====================================================

void beltOFF()
{
  digitalWrite(
    BELT_PIN,
    LOW
  );

  beltState = false;

  Serial.println(
    "BELT: OFF"
  );
}

// =====================================================
// OLED BELT STATUS
// =====================================================

void oledBeltStatus()
{
  oledClear();

  display.setTextSize(2);

  display.setCursor(
    0,
    0
  );

  display.println(
    "ReactX"
  );

  display.setTextSize(1);

  display.setCursor(
    0,
    28
  );

  display.println(
    "System Ready"
  );

  display.setCursor(
    0,
    42
  );

  display.print(
    "Belt: "
  );

  if (beltState)
  {
    display.println(
      "ON"
    );
  }
  else
  {
    display.println(
      "OFF"
    );
  }

  display.setCursor(
    0,
    54
  );

  display.print(
    "Servo: "
  );

  display.println(
    currentServoAngle
  );

  display.display();
}

// =====================================================
// WIFI
// =====================================================

void connectWiFi()
{
  oledMessage(
    "ReactX",
    "Connecting WiFi",
    "Please wait...",
    ""
  );

  Serial.println();
  Serial.println(
    "================================="
  );

  Serial.println(
    "CONNECTING TO WIFI"
  );

  Serial.println(
    "================================="
  );

  WiFi.mode(
    WIFI_STA
  );

  if (
    !WiFi.config(
      local_IP,
      gateway,
      subnet,
      primaryDNS,
      secondaryDNS
    )
  )
  {
    Serial.println(
      "Static IP configuration failed"
    );
  }

  WiFi.begin(
    ssid,
    password
  );

  int count = 0;

  while (
    WiFi.status() != WL_CONNECTED
  )
  {
    delay(500);

    Serial.print(
      "."
    );

    count++;

    if (
      count > 40
    )
    {
      Serial.println();

      Serial.println(
        "WiFi connection failed!"
      );

      oledMessage(
        "WiFi ERROR",
        "Connection",
        "Failed!",
        "Restarting..."
      );

      delay(2000);

      ESP.restart();
    }
  }

  Serial.println();

  Serial.println(
    "WiFi connected!"
  );

  Serial.print(
    "ESP32 DEV IP: "
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.print(
    "ESP32-CAM IP: "
  );

  Serial.println(
    CAMERA_IP
  );

  Serial.print(
    "Laptop IP: "
  );

  Serial.println(
    LAPTOP_IP
  );

  oledMessage(
    "WiFi Connected",
    "ESP32: .158",
    "CAM: .182",
    "Ready..."
  );

  delay(1500);
}

// =====================================================
// CAMERA CONNECTION TEST - RAW IMAGE ONLY
// =====================================================

bool captureCamera()
{
  Serial.println();
  Serial.println("=================================");
  Serial.println("CAMERA CONNECTION TEST");
  Serial.println("=================================");

  HTTPClient http;

  String url =
    "http://" +
    String(CAMERA_IP) +
    "/image?t=" +
    String(millis());

  Serial.print("Camera test URL: ");
  Serial.println(url);

  http.begin(url);
  http.setTimeout(15000);

  int code = http.GET();

  Serial.print("Camera test HTTP code: ");
  Serial.println(code);

  bool ok = (code == 200);

  if (ok)
  {
    Serial.println("CAMERA TEST SUCCESS");
  }
  else
  {
    Serial.println("CAMERA TEST FAILED");
  }

  http.end();
  return ok;
}

// =====================================================
// CAMERA + AI - ONE CAPTURE ONLY

// =====================================================
//
// IMPORTANT:
// ESP32-CAM /capture does BOTH:
//   1. captures exactly one image
//   2. sends that image to Flask /camera
//   3. Flask runs AI and returns JSON
//
// Therefore the ESP32 Dev Module must NOT call
// Flask /process_camera afterwards.
// That old flow caused TWO captures:
//   CAM /capture  -> image #1
//   Flask /process_camera -> CAM /image -> image #2
//
// New flow:
//   SENSOR -> DEV -> CAM /capture -> FLASK /camera -> RESULT
// =====================================================

String captureAndGetAI()
{
  oledMessage(
    "OBJECT DETECTED",
    "Belt: OFF",
    "Camera + AI",
    "Please wait..."
  );

  Serial.println();
  Serial.println("=================================");
  Serial.println("ONE CAPTURE + AI REQUEST");
  Serial.println("=================================");

  HTTPClient http;

  String url =
    "http://" +
    String(CAMERA_IP) +
    "/capture?t=" +
    String(millis());

  Serial.print("Camera URL: ");
  Serial.println(url);

  http.begin(url);

  // Flask AI can take several seconds.
  // ESP32-CAM /capture may wait up to 30 s for Flask.
  http.setTimeout(45000);

  int code = http.GET();

  Serial.print("Camera HTTP code: ");
  Serial.println(code);

  if (code != 200)
  {
    Serial.println("CAMERA + AI REQUEST FAILED");

    oledMessage(
      "AI ERROR",
      "Camera/Flask",
      "request failed",
      "Belt restarting"
    );

    http.end();
    return "";
  }

  String response = http.getString();

  Serial.println();
  Serial.println("CAMERA + FLASK RESPONSE:");
  Serial.println(response);
  Serial.println();

  http.end();

  return response;
}

// =====================================================
// JSON VALUE
// =====================================================

String getJSONValue(
  String json,
  String key
)
{
  String search =
    "\"" + key + "\"";

  int keyPos =
    json.indexOf(search);

  if (keyPos < 0)
  {
    return "";
  }

  int colon =
    json.indexOf(":", keyPos);

  if (colon < 0)
  {
    return "";
  }

  // Find the first quote after the colon.
  // This works for "plastic":"PET".
  int quote1 =
    json.indexOf("\"", colon);

  if (quote1 < 0)
  {
    return "";
  }

  int quote2 =
    json.indexOf("\"", quote1 + 1);

  if (quote2 < 0)
  {
    return "";
  }

  return json.substring(
    quote1 + 1,
    quote2
  );
}

// =====================================================
// SORT PLASTIC
// =====================================================
void sortPlastic(
  String type,
  String confidence
)
{
  type.trim();

  type.toUpperCase();

  Serial.println();
  Serial.println(
    "================================="
  );

  Serial.print(
    "DETECTED PLASTIC: "
  );

  Serial.println(
    type
  );

  Serial.print(
    "CONFIDENCE: "
  );

  Serial.println(
    confidence
  );

  Serial.println(
    "================================="
  );

  // Stop belt
  beltOFF();

  // ===================================================
  // OLED SORTING
  // ===================================================

  oledClear();

  display.setTextSize(2);

  display.setCursor(
    0,
    0
  );

  display.println(
    "SORTING"
  );

  display.setTextSize(2);

  display.setCursor(
    0,
    23
  );

  display.println(
    type
  );

  display.setTextSize(1);

  display.setCursor(
    0,
    46
  );

  display.print(
    "Conf: "
  );

  display.println(
    confidence
  );

  display.display();

  delay(300);

  // ===================================================
  // SERVO POSITION
  // ===================================================

  if (
    type == "HDPE"
  )
  {
    sortingServo.write(
      HDPE_ANGLE
    );

    currentServoAngle =
      HDPE_ANGLE;
  }

  else if (
    type == "OTHER"
  )
  {
    sortingServo.write(
      OTHER_ANGLE
    );

    currentServoAngle =
      OTHER_ANGLE;
  }

  else if (
    type == "PET"
  )
  {
    sortingServo.write(
      PET_ANGLE
    );

    currentServoAngle =
      PET_ANGLE;
  }

  else if (
    type == "PP"
  )
  {
    sortingServo.write(
      PP_ANGLE
    );

    currentServoAngle =
      PP_ANGLE;
  }

  else if (
    type == "PVC"
  )
  {
    sortingServo.write(
      PVC_ANGLE
    );

    currentServoAngle =
      PVC_ANGLE;
  }

  else
  {
    Serial.println(
      "UNKNOWN PLASTIC TYPE"
    );

    oledMessage(
      "UNKNOWN TYPE",
      type,
      "Belt restarting",
      ""
    );

    beltON();

    delay(1500);

    oledBeltStatus();

    return;
  }

  Serial.print(
    "SERVO ANGLE: "
  );

  Serial.println(
    currentServoAngle
  );

  delay(1000);

  // ===================================================
  // BELT ON
  // ===================================================

  beltON();

  oledBeltStatus();

  Serial.println(
    "BELT RESUMED"
  );
}

// =====================================================
// COMPLETE OBJECT PROCESS
// =====================================================

void processObject()
{
  if (
    processingObject
  )
  {
    return;
  }

  processingObject =
    true;

  Serial.println();
  Serial.println();
  Serial.println(
    "#################################"
  );

  Serial.println(
    "IR OBJECT DETECTED"
  );

  Serial.println(
    "#################################"
  );

  // ===================================================
  // STOP BELT
  // ===================================================

  beltOFF();

  delay(300);

  // ===================================================
  // ONE CAMERA CAPTURE + AI
  // ===================================================
  //
  // IMPORTANT:
  // Do not call /image or /process_camera here.
  // ESP32-CAM /capture performs exactly one capture,
  // uploads it to Flask /camera, and returns the AI result.
  // ===================================================

  String result =
    captureAndGetAI();

  if (
    result.length() == 0
  )
  {
    Serial.println(
      "AI RESPONSE EMPTY"
    );

    beltON();

    delay(1000);

    oledBeltStatus();

    processingObject =
      false;

    return;
  }

  // ===================================================
  // PLASTIC
  // ===================================================

  String plastic =
    getJSONValue(
      result,
      "plastic"
    );

  // ===================================================
  // CONFIDENCE
  // ===================================================

  String confidence =
    getJSONValue(
      result,
      "confidence"
    );

  Serial.println();
  Serial.println(
    "---------------------------------"
  );

  Serial.print(
    "PLASTIC: "
  );

  Serial.println(
    plastic
  );

  Serial.print(
    "CONFIDENCE: "
  );

  Serial.println(
    confidence
  );

  Serial.println(
    "---------------------------------"
  );

  // ===================================================
  // SORT
  // ===================================================

  if (
    plastic.length() > 0
  )
  {
    sortPlastic(
      plastic,
      confidence
    );
  }
  else
  {
    Serial.println(
      "NO PLASTIC TYPE RECEIVED"
    );

    oledMessage(
      "AI ERROR",
      "No plastic type",
      "received",
      "Belt restarting"
    );

    beltON();

    delay(1500);

    oledBeltStatus();
  }

  processingObject =
    false;

  lastDetectionTime =
    millis();

  Serial.println();
  Serial.println(
    "READY FOR NEXT OBJECT"
  );
}

// =====================================================
// IR SENSOR
// =====================================================
//
// IMPORTANT:
// The sensor can stay LOW while the same object is in front
// of it. Therefore LOW must NOT directly mean "capture now".
//
// Sequence:
//   CLEAR -> DETECT -> CAPTURE ONCE -> LOCK
//                                  |
//                         wait for sensor CLEAR
//                                  |
//                              UNLOCK
//
// This prevents one object from causing two camera captures.
// =====================================================

void checkIR()
{
  int state =
    digitalRead(
      IR_SENSOR_PIN
    );

  unsigned long now =
    millis();

  // ---------------------------------------------------
  // OBJECT DETECTED
  // LOW = object detected for the sensor used here
  // ---------------------------------------------------

  if (state == LOW)
  {
    // The same object is still in front of the sensor.
    // Do NOT capture again.
    if (!sensorReadyForObject)
    {
      return;
    }

    // Do not trigger while another object is being processed.
    if (processingObject)
    {
      return;
    }

    // Extra protection against electrical/noise retriggers.
    if (
      now -
      lastDetectionTime
      <
      DETECTION_COOLDOWN
    )
    {
      return;
    }

    // LOCK IMMEDIATELY before starting the capture.
    // This is the important part.
    sensorReadyForObject = false;

    sensorClearStart = 0;

    Serial.println();
    Serial.println(
      ">>> NEW OBJECT TRIGGER"
    );
    Serial.println(
      ">>> SENSOR LOCKED"
    );
    Serial.println(
      ">>> ONE CAPTURE ONLY"
    );

    processObject();

    return;
  }

  // ---------------------------------------------------
  // SENSOR CLEAR
  // HIGH = no object detected for the sensor used here
  // ---------------------------------------------------

  if (state == HIGH)
  {
    // Start timing how long the sensor has remained clear.
    if (sensorClearStart == 0)
    {
      sensorClearStart = now;
    }

    // Only unlock after the sensor has been continuously
    // clear for SENSOR_CLEAR_TIME milliseconds.
    if (
      !sensorReadyForObject &&
      (
        now -
        sensorClearStart
        >=
        SENSOR_CLEAR_TIME
      )
    )
    {
      sensorReadyForObject = true;

      Serial.println();
      Serial.println(
        ">>> SENSOR CLEAR"
      );
      Serial.println(
        ">>> SENSOR UNLOCKED"
      );
      Serial.println(
        ">>> READY FOR NEXT OBJECT"
      );
    }
  }
}

// =====================================================
// BELT ON
// =====================================================

void handleBeltOn()
{
  beltON();

  oledBeltStatus();

  server.send(
    200,
    "application/json",
    "{\"belt\":\"ON\"}"
  );
}

// =====================================================
// BELT OFF
// =====================================================

void handleBeltOff()
{
  beltOFF();

  oledBeltStatus();

  server.send(
    200,
    "application/json",
    "{\"belt\":\"OFF\"}"
  );
}

// =====================================================
// BELT CONTROL
// Flask uses:
//
// /belt_control?action=on
// /belt_control?action=off
// =====================================================

void handleBeltControl()
{
  if (
    !server.hasArg(
      "action"
    )
  )
  {
    server.send(
      400,
      "application/json",
      "{\"error\":\"missing action\"}"
    );

    return;
  }

  String action =
    server.arg(
      "action"
    );

  action.toLowerCase();

  if (
    action == "on"
  )
  {
    beltON();

    oledBeltStatus();

    server.send(
      200,
      "application/json",
      "{\"belt\":\"ON\"}"
    );
  }

  else if (
    action == "off"
  )
  {
    beltOFF();

    oledBeltStatus();

    server.send(
      200,
      "application/json",
      "{\"belt\":\"OFF\"}"
    );
  }

  else
  {
    server.send(
      400,
      "application/json",
      "{\"error\":\"invalid action\"}"
    );
  }
}

// =====================================================
// BELT STATUS
// =====================================================

void handleBeltStatus()
{
  String response;

  if (
    beltState
  )
  {
    response =
      "{\"belt\":\"ON\"}";
  }
  else
  {
    response =
      "{\"belt\":\"OFF\"}";
  }

  server.send(
    200,
    "application/json",
    response
  );
}

// =====================================================
// MANUAL SORT
// =====================================================

void handleSort()
{
  if (
    !server.hasArg(
      "type"
    )
  )
  {
    server.send(
      400,
      "application/json",
      "{\"error\":\"missing type\"}"
    );

    return;
  }

  String type =
    server.arg(
      "type"
    );

  sortPlastic(
    type,
    ""
  );

  server.send(
    200,
    "application/json",
    "{\"sort\":\"done\"}"
  );
}

// =====================================================
// MANUAL SERVO
// =====================================================

void handleServo()
{
  if (
    !server.hasArg(
      "angle"
    )
  )
  {
    server.send(
      400,
      "application/json",
      "{\"error\":\"missing angle\"}"
    );

    return;
  }

  int angle =
    server.arg(
      "angle"
    ).toInt();

  angle =
    constrain(
      angle,
      0,
      180
    );

  beltOFF();

  sortingServo.write(
    angle
  );

  currentServoAngle =
    angle;

  delay(700);

  oledBeltStatus();

  server.send(
    200,
    "application/json",
    "{\"servo\":\"moved\"}"
  );
}

// =====================================================
// CAMERA TEST
// =====================================================

void handleCameraTest()
{
  bool result =
    captureCamera();

  if (
    result
  )
  {
    oledMessage(
      "CAMERA TEST",
      "SUCCESS",
      "ESP32-CAM",
      "Connected"
    );

    server.send(
      200,
      "application/json",
      "{\"camera\":\"OK\"}"
    );
  }
  else
  {
    oledMessage(
      "CAMERA TEST",
      "FAILED",
      "Check CAM IP",
      "192.168.0.182"
    );

    server.send(
      500,
      "application/json",
      "{\"camera\":\"FAILED\"}"
    );
  }
}

// =====================================================
// STATUS
// =====================================================

void handleStatus()
{
  int ir =
    digitalRead(
      IR_SENSOR_PIN
    );

  String response =
    "{";

  response +=
    "\"device\":\"ESP32-DEV\",";

  response +=
    "\"ip\":\"";

  response +=
    WiFi.localIP().toString();

  response +=
    "\",";

  response +=
    "\"camera_ip\":\"";

  response +=
    CAMERA_IP;

  response +=
    "\",";

  response +=
    "\"laptop_ip\":\"";

  response +=
    LAPTOP_IP;

  response +=
    "\",";

  response +=
    "\"belt\":\"";

  if (
    beltState
  )
    response +=
      "ON";
  else
    response +=
      "OFF";

  response +=
    "\",";

  response +=
    "\"servo_angle\":";

  response +=
    String(
      currentServoAngle
    );

  response +=
    ",";

  response +=
    "\"ir_sensor\":";

  response +=
    String(
      ir
    );

  response +=
    ",";

  response +=
    "\"processing\":";

  if (
    processingObject
  )
    response +=
      "true";
  else
    response +=
      "false";

  response +=
    "}";

  server.send(
    200,
    "application/json",
    response
  );
}

// =====================================================
// ROOT
// =====================================================

void handleRoot()
{
  String html;

  html +=
    "<!DOCTYPE html>";

  html +=
    "<html>";

  html +=
    "<head>";

  html +=
    "<meta name='viewport' "
    "content='width=device-width,initial-scale=1'>";

  html +=
    "<title>CHEMOVISTA ESP32</title>";

  html +=
    "</head>";

  html +=
    "<body>";

  html +=
    "<h1>CHEMOVISTA</h1>";

  html +=
    "<h2>ESP32 DEV MODULE</h2>";

  html +=
    "<p>ESP32 IP: ";

  html +=
    WiFi.localIP().toString();

  html +=
    "</p>";

  html +=
    "<p>CAM IP: ";

  html +=
    CAMERA_IP;

  html +=
    "</p>";

  html +=
    "<p>Flask IP: ";

  html +=
    LAPTOP_IP;

  html +=
    "</p>";

  html +=
    "<p>Belt: ";

  if (
    beltState
  )
    html +=
      "ON";
  else
    html +=
      "OFF";

  html +=
    "</p>";

  html +=
    "<p><a href='/belt_control?action=on'>"
    "BELT ON"
    "</a></p>";

  html +=
    "<p><a href='/belt_control?action=off'>"
    "BELT OFF"
    "</a></p>";

  html +=
    "<p><a href='/camera/test'>"
    "TEST CAMERA"
    "</a></p>";

  html +=
    "<p><a href='/status'>"
    "STATUS"
    "</a></p>";

  html +=
    "</body>";

  html +=
    "</html>";

  server.send(
    200,
    "text/html",
    html
  );
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(
    115200
  );

  delay(1000);

  Serial.println();
  Serial.println(
    "================================="
  );

  Serial.println(
    "CHEMOVISTA ESP32 DEV MODULE"
  );

  Serial.println(
    "================================="
  );

  // ===================================================
  // OLED FIRST
  // ===================================================

  oledStartup();

  // ===================================================
  // BELT
  // ===================================================

  pinMode(
    BELT_PIN,
    OUTPUT
  );

  digitalWrite(
    BELT_PIN,
    LOW
  );

  // ===================================================
  // IR
  // ===================================================

  pinMode(
    IR_SENSOR_PIN,
    INPUT
  );

  // Do not trigger immediately at startup if an object is
  // already sitting in front of the sensor.
  if (digitalRead(IR_SENSOR_PIN) == HIGH)
  {
    sensorReadyForObject = true;
    sensorClearStart = millis();
  }
  else
  {
    sensorReadyForObject = false;
    sensorClearStart = 0;

    Serial.println(
      "IR SENSOR BLOCKED AT STARTUP"
    );
    Serial.println(
      "Waiting for sensor to become CLEAR..."
    );
  }

  // ===================================================
  // SERVO
  // ===================================================

  sortingServo.setPeriodHertz(
    50
  );

  sortingServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  sortingServo.write(
    90
  );

  currentServoAngle =
    90;

  // ===================================================
  // WIFI
  // ===================================================

  connectWiFi();

  // ===================================================
  // HTTP ROUTES
  // ===================================================

  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );

  server.on(
    "/belt/on",
    HTTP_GET,
    handleBeltOn
  );

  server.on(
    "/belt/off",
    HTTP_GET,
    handleBeltOff
  );

  server.on(
    "/belt_control",
    HTTP_GET,
    handleBeltControl
  );

  server.on(
    "/belt",
    HTTP_GET,
    handleBeltStatus
  );

  server.on(
    "/sort",
    HTTP_GET,
    handleSort
  );

  server.on(
    "/servo",
    HTTP_GET,
    handleServo
  );

  server.on(
    "/status",
    HTTP_GET,
    handleStatus
  );

  server.on(
    "/camera/test",
    HTTP_GET,
    handleCameraTest
  );

  server.begin();

  Serial.println();
  Serial.println(
    "================================="
  );

  Serial.println(
    "HTTP SERVER STARTED"
  );

  Serial.print(
    "ESP32 IP: "
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.print(
    "CAMERA IP: "
  );

  Serial.println(
    CAMERA_IP
  );

  Serial.print(
    "FLASK: "
  );

  Serial.println(
    "CAM /capture -> Flask /camera"
  );

  Serial.println(
    "OLED: SDA 21 / SCL 22"
  );

  Serial.println(
    "IR: GPIO 27"
  );

  Serial.println(
    "BELT: GPIO 25"
  );

  Serial.println(
    "SERVO: GPIO 26"
  );

  Serial.println(
    "================================="
  );

  // ===================================================
  // START BELT
  // ===================================================

  delay(500);

  beltON();

  oledBeltStatus();

  Serial.println();
  Serial.println(
    "SYSTEM READY"
  );
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  server.handleClient();

  checkIR();

  delay(5);
}