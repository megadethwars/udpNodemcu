//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#define BUTTON 13 //botón en GPIO13 (D7)

AsyncWebServer server(80);

const char* ssid = "empires";
const char* password = "44863333";

const char* PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


///ports motor
int pwmMotorA = 5;
int pwmMotorB = 4;
int dirMotorA = 0;
int dirMotorB = 2;

double input;
double output;
volatile int count = 0;
int velocidad=0;

/////PID///
// Arbitrary setpoint and gains - adjust these as fit for your project:
double setpoint = 0;
double p = 1.5;
double i = 0.01;
double d = 0.0;

int limitUP=2000;
int limitDown=0;

int integralUp=2000;
int integralDown=-2000;

double errorint=0.0;
double erroractual=0.0;
double errorpast=0.0;
unsigned long startTime;

void ICACHE_RAM_ATTR ISRoutine ();

void configureSettings(){
  startTime = millis();

  analogWriteFreq(100);
  analogWriteRange(2047);
 
  pinMode(pwmMotorB, OUTPUT);
  pinMode(dirMotorB, OUTPUT);
  digitalWrite(dirMotorB, LOW);

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON),ISRoutine,RISING);
 }

double calculatePID(double input, double setpoint,double pr,double in,double dif){

  double error=setpoint-input;
  double integral = in*errorint;
  if(integral>integralUp){
      integral=integralUp;
    }

  else if(integral<integralDown){
      integral=integralDown;
    }
  else{
    errorint=errorint+error;
    }
  
  erroractual=error-errorpast;
  errorpast=error;
  output=(pr*error) +(integral) + dif*(erroractual);


  if(output>limitUP){
      output=limitUP;
    }

  if(output<limitDown){
      output=limitDown;
    }

  
  return output;
  }


void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    configureSettings();

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }

        if(request->hasHeader("P")){
          AsyncWebHeader* h = request->getHeader("P");
          message=h->value().c_str();
          request->send(200, "text/plain", "P:" + message);
        }

        if(request->hasHeader("I")){
          AsyncWebHeader* h = request->getHeader("D");
          message=h->value().c_str();
          request->send(200, "text/plain", "I:" + message);
        }

        if(request->hasHeader("D")){
          AsyncWebHeader* h = request->getHeader("D");
          message=h->value().c_str();
          
          request->send(200, "text/plain", "D:" + message);
        }

        if(request->hasHeader("SetPoint")){
          AsyncWebHeader* h = request->getHeader("SetPoint");
          message=h->value().c_str();
          int st = message.toInt();
          setpoint=(double)st;
          request->send(200, "text/plain", "SetPoint:" + message);
        }
        
        request->send(200, "text/plain", "Hello, POST: " + message);
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop() {

  if (millis() - startTime >= 250) { 

    velocidad = count;
    input=(double)velocidad;
    count=0;
    startTime = millis(); // reiniciar el contador
  }
  output=calculatePID(input,setpoint,p,i,d);
  analogWrite(pwmMotorB, (int)output);
}


void ISRoutine () {
  count=count+1;
}
