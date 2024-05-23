#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <MicroWakeupper.h>

#ifndef STASSID
#define STASSID "Sam's Galaxy A50"
#define STAPSK "wachtwoord"
#endif

MicroWakeupper microWakeupper;

const int SPEAKER_PIN = D7; // Digitale pin D7 voor de speaker (SIG)
const int NC_PIN = D6;       // Digitale pin D6 voor NC

const char* ssid = STASSID;
const char* password = STAPSK;



ESP8266WebServer server(80);

void handleRoot() {
  
  server.send(200, "text/plain", "usage:\n/speaker");
  
}

void handleNotFound() {
 
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);

}

void speaker(){
  tone(SPEAKER_PIN, 500);
  delay(1000); // Wacht 1 seconde voordat we een nieuwe toon afspelen
  noTone(SPEAKER_PIN);
}

void setup(void) {
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("SamD")) { Serial.println("MDNS responder started"); }

  server.on("/", handleRoot);

  server.on("/speaker", []() {
    server.send(200, "text/plain", "geluid wordt afgespeeld");
    tone(SPEAKER_PIN, 1000, 100); // Speel een toon van 1000 Hz af gedurende 100 milliseconden
    delay(200);

    tone(SPEAKER_PIN, 500);
    delay(1000); // Wacht 1 seconde voordat we een nieuwe toon afspelen
    noTone(SPEAKER_PIN);
  }
   
  );


  server.onNotFound(handleNotFound);

  /////////////////////////////////////////////////////////
  // Hook examples

  server.addHook([](const String& method, const String& url, WiFiClient* client, ESP8266WebServer::ContentTypeFunction contentType) {
    (void)method;       // GET, PUT, ...
    (void)url;          // example: /root/myfile.html
    (void)client;       // the webserver tcp client connection
    (void)contentType;  // contentType(".html") => "text/html"
    Serial.printf("A useless web hook has passed\n");
    Serial.printf("(this hook is in 0x%08x area (401x=IRAM 402x=FLASH))\n", esp_get_program_counter());
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String& url, WiFiClient*, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/fail")) {
      Serial.printf("An always failing web hook has been triggered\n");
      return ESP8266WebServer::CLIENT_MUST_STOP;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  server.addHook([](const String&, const String& url, WiFiClient* client, ESP8266WebServer::ContentTypeFunction) {
    if (url.startsWith("/dump")) {
      Serial.printf("The dumper web hook is on the run\n");

      // Here the request is not interpreted, so we cannot for sure
      // swallow the exact amount matching the full request+content,
      // hence the tcp connection cannot be handled anymore by the
      // webserver.
#ifdef STREAMSEND_API
      // we are lucky
      client->sendAll(Serial, 500);
#else
      auto last = millis();
      while ((millis() - last) < 500) {
        char buf[32];
        size_t len = client->read((uint8_t*)buf, sizeof(buf));
        if (len > 0) {
          Serial.printf("(<%d> chars)", (int)len);
          Serial.write(buf, len);
          last = millis();
        }
      }
#endif
      // Two choices: return MUST STOP and webserver will close it
      //                       (we already have the example with '/fail' hook)
      // or                  IS GIVEN and webserver will forget it
      // trying with IS GIVEN and storing it on a dumb WiFiClient.
      // check the client connection: it should not immediately be closed
      // (make another '/dump' one to close the first)
      Serial.printf("\nTelling server to forget this connection\n");
      static WiFiClient forgetme = *client;  // stop previous one if present and transfer client refcounter
      return ESP8266WebServer::CLIENT_IS_GIVEN;
    }
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });

  // Hook examples
  /////////////////////////////////////////////////////////

  server.begin();
  Serial.println("HTTP server started");

  pinMode(SPEAKER_PIN, OUTPUT); // Stel de speaker pin in als uitgang
  pinMode(NC_PIN, OUTPUT);      // Stel de NC pin in als uitgang
  digitalWrite(NC_PIN, LOW);    // Zet de NC pin laag
  
  // Testgeluid bij opstarten
  tone(SPEAKER_PIN, 1000, 100); // Speel een toon van 1000 Hz af gedurende 100 milliseconden
  delay(200);

  tone(SPEAKER_PIN, 500);
  delay(1000); // Wacht 1 seconde voordat we een nieuwe toon afspelen
  noTone(SPEAKER_PIN);
}

void loop(void) {
  //Serial.print("Current Battery Voltage:");
  //Serial.println(microWakeupper.readVBatt());
  Serial.println(WiFi.localIP());


  server.handleClient();
  MDNS.update();
  delay(1000);

}
