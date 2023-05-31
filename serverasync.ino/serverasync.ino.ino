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
#include <AsyncWebSocket.h>
#define BUTTON 13 //botón en GPIO13 (D7)

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");



String htmlCode3 = "<!DOCTYPE html><html><head><title>Control de motor</title><style>body{background-color:#050b16;text-align:center;font-family:Arial,sans-serif}h1{margin-top:50px;color:#333}#slider{width:300px;margin-top:30px;margin-bottom:50px}#speed{font-size:24px;color:#00f7ff}#websocketValue{font-size:18px;color:#00f7ff;margin-top:20px}#titleset{font-size:18px;color:#00f7ff;margin-top:20px}</style></head><body><h1 style=\"color:#00f7ff\">Control de Velocidad</h1><input type=\"range\" min=\"0\" max=\"255\" value=\"0\" id=\"slider\" oninput=\"handleChange(this.value)\"><p id=\"titleset\">SETPOINT (counts per second)</p><p id=\"speed\">0</p><p id=\"websocketValue\"></p><script>function handleChange(value){if(value%5===0){document.getElementById(\"speed\").textContent=value;fetch(\"/post\",{method:\"POST\",headers:{\"Content-Type\":\"application/x-www-form-urlencoded\",\"SetPoint\":value.toString()}}).then(response=>{if(!response.ok){throw new Error(\"Error en la solicitud: \"+response.status)}return response.text()}).then(responseData=>{console.log(\"Respuesta del servidor:\",responseData)}).catch(error=>{console.error(\"Error en la solicitud:\",error)})}}const socket=new WebSocket(\"ws://\"+window.location.hostname+\"/ws\");socket.onopen=function(event){console.log(\"WebSocket connected\")};socket.onmessage=function(event){console.log(\"WebSocket message:\",event.data);var jsonData=JSON.parse(event.data);if(jsonData.hasOwnProperty(\"output\")){document.getElementById(\"websocketValue\").textContent=\"Salida Encoder: \"+jsonData.output}};</script></body></html>";
String htmlCode = "<!DOCTYPE html><html><head><title>Control de motor</title><style>body{background-color:#050b16;text-align:center;font-family:Arial,sans-serif}h1{margin-top:50px;color:#333}#slider{width:300px;margin-top:30px;margin-bottom:50px}#speed{font-size:24px;color:#00f7ff}#websocketValue{font-size:18px;color:#00f7ff;margin-top:20px}#titleset{font-size:18px;color:#00f7ff;margin-top:20px}.meter{width:200px;height:200px;background-color:#333;border-radius:50%;display:flex;align-items:center;justify-content:center;margin:0 auto;position:relative}.needle{width:5px;height:90px;background-color:#00f7ff;transform-origin:bottom center;position:absolute;bottom:50%;left:50%;transition:transform 0.3s ease}</style></head><body><h1 style=\"color:#00f7ff\">Control de Velocidad</h1><input type=\"range\" min=\"0\" max=\"255\" value=\"0\" id=\"slider\" oninput=\"handleChange(this.value)\"><p id=\"titleset\">SETPOINT</p><p id=\"speed\">0</p><div class=\"meter\"><div class=\"needle\" id=\"needle\"></div></div><p id=\"websocketValue\"></p><script>function handleChange(value){if(value%10===0){document.getElementById(\"speed\").textContent=value;fetch(\"/post\",{method:\"POST\",headers:{\"Content-Type\":\"application/x-www-form-urlencoded\",\"SetPoint\":value.toString()}}).then(response=>{if(!response.ok){throw new Error(\"Error en la solicitud: \"+response.status)}return response.text()}).then(responseData=>{console.log(\"Respuesta del servidor:\",responseData)}).catch(error=>{console.error(\"Error en la solicitud:\",error)})}}const socket=new WebSocket(\"ws://\"+window.location.hostname+\"/ws\");socket.onopen=function(event){console.log(\"WebSocket connected\")};socket.onmessage=function(event){console.log(\"WebSocket message:\",event.data);var jsonData=JSON.parse(event.data);if(jsonData.hasOwnProperty(\"output\")){document.getElementById(\"websocketValue\").textContent=\"Valor del WebSocket: \"+jsonData.output;var needleElement=document.getElementById(\"needle\");var value=Number(jsonData.output);var angle=(value/255)*180-90;needleElement.style.transform=\"rotate(\"+angle+\"deg)\"}};</script></body></html>";

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


void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("Cliente conectado: %u\n", client->id());
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("Cliente desconectado: %u\n", client->id());
  } else if(type == WS_EVT_DATA){
    String msg = "";
    for(size_t i=0; i < len; i++) {
      msg += (char) data[i];
    }
    Serial.printf("Cliente %u envió el siguiente mensaje: %s\n", client->id(), msg.c_str());
    ws.textAll(msg);
  }
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


    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    ////////////INDEX
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", htmlCode);
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        Serial.printf("entering---get");
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
      Serial.println("entering....");
      
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
          
          AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "SetPoint:" + message);
          response->addHeader("Access-Control-Allow-Origin", "*");
          request->send(response);
          //request->send(200, "text/plain", "SetPoint:" + message);
          Serial.println("sending HEADER, please check..");
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

    
    ws.textAll("{\"output\":"+String(velocidad)+"}");
  }
  output=calculatePID(input,setpoint,p,i,d);
  analogWrite(pwmMotorB, (int)output);
}


void ISRoutine () {
  count=count+1;
}
