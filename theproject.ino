#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <LittleFS.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino resetpin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// File names for storing data
const char* apModeFileName = "/apMode.txt";
const char* ssidFileName = "/ssid.txt";
const char* passwordFileName = "/password.txt";

bool btnFlag;
bool btn;
unsigned long pressTimer = 0; 
unsigned long holdStartTime = 0;
unsigned long lastTemperatureUpdate = 0; // Timer for temperature updates

const int oneWireBus = 0;  // GPIO where the DS18B20 is connected to

OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire devices

DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature sensor 

String tempC = "";
String tempF = "";

String ssid = "TP-Link2g";
String password = "Marvelous";

const char* ssidAP = "ESP32-Access-Point";

AsyncWebServer server(80);

bool showTemperatureScreen = true; 

byte tries = 10;

String ip = "none";
IPAddress ipString;

bool isAPMode = true;

String acessStatus = "none";

String nameOfWifi = "none";

int hours = 20;
int minutes = 52;
int seconds = 0;
long lastTime = 0;  // создаем объект класса long для хранения счетчика
String timeWatch = "none";

float volts = 0.0; //battery level
int perc = 0; //vattery charge percent

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    body {
      font-family: sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      background-color: #f0f0f0;
    }
    .container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      padding: 20px;
      background-color: #fff;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      width: 80%;
      max-width: 600px;
    }
    .item {
      width: 150px;
      margin: 10px;
      padding: 15px;
      border-radius: 5px;
      background-color: #e0e0e0;
      text-align: center; /* Center text horizontally */
      display: flex; /* Enable flexbox for item */
      align-items: center; /* Vertically center items */
    }
    .item h3 {
      margin-top: 0;
      font-size: 1.2em;
      margin-right: 5px; /* Add space between title and value */
    }
    .item .value {
      font-size: 1.5em;
      font-weight: bold;
      margin-bottom: 0; /* Remove bottom margin */
      margin-right: 5px; /* Add space between value and unit */
    }
    .item .unit {
      font-size: 0.8em;
      margin-bottom:0; /* Remove bottom margin */
      vertical-align:middle; /* Vertically align unit with value */

    }
    .form-container {
      width: 100%;
      padding: 20px;
    }
    .form-container label {
      display: block;
      margin-bottom: 5px;
    }
    .form-container input[type="submit"] {
      padding: 10px 20px;
      background-color: #4CAF50;
      color: white;
      border: none;
      cursor: pointer;
    }
    #time-display{
      font-size: 1.5em;
    }
  </style>
</head>
<body>
  <h1>ESP32 Dashboard</h1>
  <div class="container">
    <div class="item">
      <h3>Temperature</h3>
      <div class="value" id="temperature">%TEMPERATURE%</div>
      <div class="unit">&deg;C</div>
    </div>
    <div class="item">
      <h3>Voltage</h3>
      <div class="value" id="voltage">%VOLTAGE%</div>
      <div class="unit">V</div>
    </div>
    <div class="item">
      <h3>Time</h3>
      <div class="value" id="time-display">Time: </div>
    </div>
  </div>

  <div class="form-container">
      <form id="wifi-form" action="/setwifi" method="post">
        <label for="ssid">SSID:</label>
        <input type="text" id="ssid" name="ssid" required><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Connect">
      </form>
  </div>

  <div class="form-container">
      <form id="time-form" action="/settime" method="post">
        <label for="hours">Hours (0-23):</label>
        <input type="number" id="hours" name="hours" min="0" max="23" required><br><br>
        <label for="minutes">Minutes (0-59):</label>
        <input type="number" id="minutes" name="minutes" min="0" max="59" required><br><br>
        <label for="seconds">Seconds (0-59):</label>
        <input type="number" id="seconds" name="seconds" min="0" max="59" required><br><br>
        <input type="submit" value="Set Time">
      </form>
  </div>
</body>
<script>
setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000);
  setInterval(function() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("time-display").innerHTML = "Time: " + this.responseText;
      }
    };
    xhttp.open("GET", "/gettime", true);
    xhttp.send();
  }, 1000);
  setInterval(function () {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("voltage").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/voltage", true);
  xhttp.send();
}, 1000);
</script>
</html>
)rawliteral";

String readTemperatureC(){
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0)-3;
  Serial.print(temperatureC);
  Serial.println("ºC");
  return String(temperatureC);
}

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return readTemperatureC();
  }
    else if(var == "VOLTAGE"){
    return String(volts);
  }
  return String();
}

bool saveSettings(bool apMode, const String& ssid, const String& password) {
  if (!LittleFS.begin()) {
    Serial.println("Error: LittleFS Mount Failed");
    return false;
  }

  File apModeFile = LittleFS.open(apModeFileName, "w");
  if (!apModeFile) {
    Serial.println("Error: Opening apMode file for writing");
    LittleFS.end();
    return false;
  }
  apModeFile.print(apMode);
  apModeFile.close();

  File ssidFile = LittleFS.open(ssidFileName, "w");
  if (!ssidFile) {
    Serial.println("Error: Opening ssid file for writing");
    LittleFS.end();
    return false;
  }
  ssidFile.println(ssid);
  ssidFile.close();

  File passwordFile = LittleFS.open(passwordFileName, "w");
  if (!passwordFile) {
    Serial.println("Error: Opening password file for writing");
    LittleFS.end();
    return false;
  }
  passwordFile.println(password);
  passwordFile.close();

  LittleFS.end();
  Serial.println("Settings saved to LittleFS.");
  return true;
}

bool loadSettings(bool& apMode, String& ssid, String& password) {
  if (!LittleFS.begin()) {
    Serial.println("Error: LittleFS Mount Failed");
    return false;
  }

  File apModeFile = LittleFS.open(apModeFileName, "r");
  if (apModeFile) {
    apMode = apModeFile.parseInt();
    apModeFile.close();
  } else {
    Serial.println("Warning: apMode file not found. Using default.");
  }

  File ssidFile = LittleFS.open(ssidFileName, "r");
  if (ssidFile) {
    ssid = ssidFile.readStringUntil('\n');
    ssidFile.close();
  } else {
    Serial.println("Warning: ssid file not found. Using default.");
  }

  File passwordFile = LittleFS.open(passwordFileName, "r");
  if (passwordFile) {
    password = passwordFile.readStringUntil('\n');
    passwordFile.close();
  } else {
    Serial.println("Warning: password file not found. Using default.");
  }

  LittleFS.end();
  Serial.println("Settings loaded from LittleFS.");
  return true;
}

void setupAPMode()
{
  WiFi.mode(WIFI_AP);
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssidAP);
  ipString = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ipString);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTemperatureC().c_str());
  });
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTemperatureC().c_str());
  });
  server.on("/gettime", HTTP_GET, [](AsyncWebServerRequest *request){
    String timeWatch = String(hours) + ":" + String(minutes) + ":" + String(seconds);
    request->send_P(200, "text/plain", timeWatch.c_str());
  });
    server.on("/settime", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasArg("hours") && request->hasArg("minutes") && request->hasArg("seconds")) {
      hours = request->arg("hours").toInt();
      minutes = request->arg("minutes").toInt();
      seconds = request->arg("seconds").toInt();

      //Input validation.  Add more robust validation as needed.
      hours = constrain(hours, 0, 23);
      minutes = constrain(minutes, 0, 59);
      seconds = constrain(seconds, 0, 59);

      Serial.printf("Time set to: %02d:%02d:%02d\n", hours, minutes, seconds);
      request->send(200, "text/plain", "Time set successfully!");
    } else {
      request->send(400, "text/plain", "Invalid request");
    }
  });
  server.on("/setwifi", HTTP_POST, [](AsyncWebServerRequest *request){
  if (request->hasArg("ssid") && request->hasArg("password")) {
   ssid = request->arg("ssid");
   password = request->arg("password");
   // Here you would connect to the WiFi network using ssid and password
   // This is a simplified example and lacks proper error handling and security
    saveSettings(isAPMode, ssid, password);
   Serial.print("Received SSID: ");
   Serial.println(ssid);
   Serial.print("Received Password: ");
   Serial.println(password); // In real application, avoid printing password to serial
   // Connect to the provided wifi network. Add proper error handling.
   request->send(200, "text/plain", "WiFi settings saved. Please refresh.");
  } else {
   request->send(400, "text/plain", "Invalid request");
  }
 });
  server.begin();
}

void clientWF(){
  WiFi.begin(ssid, password);
  while (--tries && WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Non Connecting to WiFi..");
  }
  else
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    ipString = WiFi.localIP();
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTemperatureC().c_str());
  });
  server.on("/gettime", HTTP_GET, [](AsyncWebServerRequest *request){
    String timeWatch = String(hours) + ":" + String(minutes) + ":" + String(seconds);
    request->send_P(200, "text/plain", timeWatch.c_str());
  });
    server.on("/settime", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasArg("hours") && request->hasArg("minutes") && request->hasArg("seconds")) {
      hours = request->arg("hours").toInt();
      minutes = request->arg("minutes").toInt();
      seconds = request->arg("seconds").toInt();

      //Input validation.  Add more robust validation as needed.
      hours = constrain(hours, 0, 23);
      minutes = constrain(minutes, 0, 59);
      seconds = constrain(seconds, 0, 59);

      Serial.printf("Time set to: %02d:%02d:%02d\n", hours, minutes, seconds);
      request->send(200, "text/plain", "Time set successfully!");
    } else {
      request->send(400, "text/plain", "Invalid request");
    }
  });
  server.on("/setwifi", HTTP_POST, [](AsyncWebServerRequest *request){
  if (request->hasArg("ssid") && request->hasArg("password")) {
   ssid = request->arg("ssid");
   password = request->arg("password");
   // Here you would connect to the WiFi network using ssid and password
   // This is a simplified example and lacks proper error handling and security
   Serial.print("Received SSID: ");
   Serial.println(ssid);
   Serial.print("Received Password: ");
   Serial.println(password); // In real application, avoid printing password to serial
    saveSettings(isAPMode, ssid, password);
   // Connect to the provided wifi network. Add proper error handling.
   request->send(200, "text/plain", "WiFi settings saved. Please refresh.");
  } else {
   request->send(400, "text/plain", "Invalid request");
  }
 });

  server.begin();
}

void switchMod(){
    if (isAPMode) {
      setupAPMode(); 
      acessStatus = "|APmode";
      nameOfWifi = ssidAP;
    } else {
      clientWF();
      nameOfWifi = ssid;
      acessStatus = "|Client";
    } 
}


void voltMetr(){
  volts = analogRead(2) * 5.0/4095*1.058;
  perc = map(volts, 3.6, 4.2, 0, 100);
}

void watch(){
    // как только разница между текущим временем и временем записанным в lastTime становится больше 1000 миллисекунд...
  if(millis()-lastTime > 1000) {
    //...обновляем  lastTime и добавляем к счетчику Секунд +1
    lastTime = millis();
    seconds++;
    // как только счетчик секунд достигнет 60, обнуляем его и добавляем к счетчику Минут +1...
    if (seconds >= 60) {
       seconds = 0;
       minutes++;
    }
    // ...тоже самое для Часов...
    if (minutes >= 60) {
       minutes = 0;
       hours++;
    }
    // ... и обнуляем счетчик Часов в конце дня
    if (hours >= 24) {
       hours = 0;
    }
    //Serial.println(hours);
    //Serial.println(minutes);
    //Serial.println(seconds);
    timeWatch = String(hours)+":"+String(minutes)+":"+String(seconds);
    Serial.print("Time: ");
    Serial.println(timeWatch);
    Serial.print("volts: ");
    Serial.println(volts);
    voltMetr();
  }
}

bool handleButtonPress() {
  if (btn == true && btnFlag == false && millis() - pressTimer > 100) { 
    btnFlag = true; 
    pressTimer = millis(); 
    Serial.println("press"); 
    holdStartTime = millis(); 
    showTemperatureScreen = !showTemperatureScreen;
  } 
  return true;
}

void handleButtonReset() { //reset in 6 seconds
  if (btn == true && btnFlag == true && millis() - holdStartTime > 6000) { 
    Serial.println("Reset"); 
    btnFlag = false; 
    holdStartTime = 0; 
    if (isAPMode) {
    isAPMode = false;
    } else {
    isAPMode = true;
    } 
    switchMod();
    saveSettings(isAPMode, ssid, password);
  } 
}

void handleButtonRelease() {
  if (btn == false && btnFlag == true && millis() - pressTimer > 500) { 
    btnFlag = false; 
    pressTimer = millis(); 
    Serial.println("release"); 
  } 
}

String readTemperatureF(){
  sensors.requestTemperatures();
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureF);
  Serial.println("ºF");
  return String(temperatureF);
}

void temperatureScreen(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);
  display.setCursor(4, 4);
  display.setTextSize(1);
  display.println("Temps|");
  display.setCursor(85, 4);
  display.println(acessStatus);

  display.setCursor(47, 4);
  display.print(volts);
  display.println("V");

  display.setTextSize(2);
  display.setCursor(4, 25);
  display.println(timeWatch);

  display.setCursor(4, 50);
  display.println(readTemperatureC());
  display.setCursor(69, 50);
  display.write(0xF7);
  display.println("C");
  display.display();
}

void ipScreen(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 4);
  display.setTextSize(1);
  display.println("Adress|");

  display.setCursor(85, 4);
  display.println(acessStatus);

  display.setCursor(47, 4);
  display.print(perc);
  display.println("%");

  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);

  display.setCursor(5, 50);
  display.println("ip: ");
  display.setCursor(25, 50);
  display.println(ipString);

  display.setCursor(5, 30);
  display.println("WIFI name: ");
  display.setCursor(5, 40);
  display.println(nameOfWifi);

  display.display();
}

void setup() {
  Wire.begin(8, 9);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  Serial.begin(115200);
   if (loadSettings(isAPMode, ssid, password)) {
      if (ssid.length() == 0 || password.length() == 0) {
          //Default Values
          isAPMode = true;
          ssid = "TP-Link2g";
          password = "Marvelous";
          saveSettings(isAPMode, ssid, password);
          Serial.println("Using default WIFI settings");
      } else {
          Serial.print("SSID: ");
          Serial.println(ssid);
          Serial.print("Password: ");
          Serial.println(password);
          Serial.print("AP Mode: ");
          Serial.println(isAPMode);
      }
  }
  sensors.begin();    // Start the DS18B20 sensor
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  switchMod();
}

void loop() {
  btn = digitalRead(1); 

  watch();
  handleButtonPress(); 
  handleButtonReset(); 
  handleButtonRelease(); 

  if (showTemperatureScreen) {
    if( millis() - lastTemperatureUpdate >1000){
      lastTemperatureUpdate = millis();
      temperatureScreen();
    }  
  } else {
    ipScreen();
  }
}