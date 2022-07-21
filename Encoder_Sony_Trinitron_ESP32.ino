#include <ESP32Encoder.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

const char* host = "HOSTNAME"; // Update with your desired hostname
const char* ssid = "SSID"; // Update with your SSID
const char* password = "PASSWORD"; // Update with your WiFi password

WebServer server(80);

ESP32Encoder Enc1;
int Enc1btn = 18;
ESP32Encoder Enc2;
int Enc2btn = 22;
int volup = 13;
int voldown = 23;
int chup = 14;
int chdown = 27;
int input = 33;
int menu = 32;
long oldPosition1  = -999;
long oldPosition2  = -999;

/*
 * Login page
 * Update this line with your desired username and password for OTA update
 * "if(form.userid.value=='admin' && form.pwd.value=='admin')"
 */

const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

void setup(){
  
  Serial.begin(115200);
  WiFi.setHostname(host);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
  
  pinMode (Enc1btn, INPUT_PULLUP);
  pinMode (Enc2btn, INPUT_PULLUP);
  pinMode (volup, OUTPUT);
  digitalWrite(volup, LOW);
  pinMode (voldown, OUTPUT);
  digitalWrite(voldown, LOW);
  pinMode (chup, OUTPUT);
  digitalWrite(chup, LOW);
  pinMode (chdown, OUTPUT);
  digitalWrite(chdown, LOW);
  pinMode (input, OUTPUT);
  digitalWrite(input, LOW);
  pinMode (menu, OUTPUT);
  digitalWrite(menu, LOW);

  // Enable the weak pull down resistors

  //ESP32Encoder::useInternalWeakPullResistors=DOWN;
  // Enable the weak pull up resistors
  ESP32Encoder::useInternalWeakPullResistors=UP;

  // use pin 19 and 18 for the first encoder
  Enc1.attachSingleEdge(4, 5);
  // use pin 17 and 16 for the second encoder
  Enc2.attachSingleEdge(19, 21);
    
  // set starting count value after attaching
  //encoder.setCount(37);
  Enc1.setFilter(1023);
  Enc2.setFilter(1023);
  // clear the encoder's raw count and set the tracked count to zero
  //encoder2.clearCount();
  Serial.println("Encoder Start = " + String((int32_t)Enc1.getCount()) + " " + String((int32_t)Enc2.getCount()));
  // set the lastToggle
  //encoder2lastToggled = millis();
}

void loop(){
  server.handleClient();
  
  long newPosition1 = Enc1.getCount();
  if (newPosition1 != oldPosition1) {
    Serial.println(newPosition1);
    if (newPosition1 > oldPosition1) {
      digitalWrite(volup, HIGH);
      delay(100);
      digitalWrite(volup, LOW);
    } else {
      digitalWrite(voldown, HIGH);
      delay(100);
      digitalWrite(voldown, LOW);
    }
    long newPosition1 = Enc1.getCount();
    oldPosition1 = newPosition1;
  }
  long newPosition2 = Enc2.getCount();
  if (newPosition2 != oldPosition2) {
    Serial.println(newPosition2);
    if (newPosition2 > oldPosition2) {
      digitalWrite(chup, HIGH);
      delay(100);
      digitalWrite(chup, LOW);
    } else {
      digitalWrite(chdown, HIGH);
      delay(100);
      digitalWrite(chdown, LOW);
    }
    long newPosition2 = Enc2.getCount();
    oldPosition2 = newPosition2;
  }
  if (digitalRead(Enc1btn) == LOW) {
    digitalWrite(menu, HIGH);
      delay(100);
      digitalWrite(menu, LOW);
  }
  if (digitalRead(Enc2btn) == LOW) {
    digitalWrite(input, HIGH);
      delay(100);
      digitalWrite(input, LOW);
  }
  
  // Loop and read the count
//  Serial.println("Encoder count = " + String((int32_t)encoder.getCount()) + " " + String((int32_t)encoder2.getCount()));
//  delay(100);
//
//  // every 5 seconds toggle encoder 2
//  if (millis() - encoder2lastToggled >= 5000) {
//    if(encoder2Paused) {
//      Serial.println("Resuming Encoder 2");
//      encoder2.resumeCount();
//    } else {
//      Serial.println("Paused Encoder 2");
//      encoder2.pauseCount();
//    }
//
//    encoder2Paused = !encoder2Paused;
//    encoder2lastToggled = millis();
//  }


}
