/*
  created 21 Aug 2010
  by Michael Margolis

  This code is in the public domain.

  adapted from Ethernet library examples
*/


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "empires"
#define STAPSK  "44863333"
#endif
#include "ArduPID.h"
///ports motor
 int pwmMotorA = 5;
 int pwmMotorB = 4;
 int dirMotorA = 0;
 int dirMotorB = 2;
#define BUTTON 13 //botón en GPIO13 (D7)

//pid
ArduPID myController;
double input;
double output;
volatile int count = 0;
int velocidad=0;


/////PID///
// Arbitrary setpoint and gains - adjust these as fit for your project:
double setpoint = 0;
double p = 0.01;
double i = 0.00005;
double d = 0.0;


unsigned int localPort = 8888;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged\r\n";       // a string to send back
String datReq;
WiFiUDP Udp;
unsigned long startTime;

void ICACHE_RAM_ATTR ISRoutine ();

void setupPID(){

    myController.begin(&input, &output, &setpoint, p, i, d);

  // myController.reverse()               // Uncomment if controller output is "reversed"
    //myController.setSampleTime(20);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
    myController.setOutputLimits(0, 2000);
    //myController.setBias(255.0 / 2.0); /// una especie de offset, un 0 referencial, relativo
    myController.setWindUpLimits(-2000, 2000); // Groth bounds for the integral term to prevent integral wind-up
    
    myController.start();
    // myController.reset();               // Used for resetting the I and D terms - only use this if you know what you're doing
    // myController.stop();                // Turn off the PID controller (compute() will not do anything until start() is called)
 }


void connectWifi(){
    WiFi.mode(WIFI_STA);
    WiFi.begin(STASSID, STAPSK);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(500);
    }
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    Serial.printf("UDP server on port %d\n", localPort);
    Udp.begin(localPort);
  }

void setup() {
  Serial.begin(115200);
  connectWifi();
  startTime = millis();

  analogWriteFreq(100);
  analogWriteRange(2047);
 
  pinMode(pwmMotorB, OUTPUT);
  pinMode(dirMotorB, OUTPUT);
  digitalWrite(dirMotorB, LOW);

  pinMode(BUTTON, INPUT_PULLUP);
 attachInterrupt(digitalPinToInterrupt(BUTTON),ISRoutine,RISING);

 setupPID();
}

void Send(String estatus,int cuenta){
  /////cmd
    char cstr[16];
    itoa(cuenta, cstr, 10);
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());  //Initialize Packet send 
    Udp.print(estatus); //Send string back to client 
    Udp.print("|");
    Udp.print(cuenta);
    Udp.print('\n');
    Udp.endPacket(); //Packet has been sent
  }

void receiveData(int pktsize){
    //Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
    //              pktsize,
    //              Udp.remoteIP().toString().c_str(), Udp.remotePort(),
    //              Udp.destinationIP().toString().c_str(), Udp.localPort(),
    //              ESP.getFreeHeap());

    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
   
    String cadenaString;
    String datReq(packetBuffer);

    /*
    for (int i = 0; i < sizeof(packetBuffer); i++) {
      cadenaString += packetBuffer[i];
    }
    */
    //Serial.println("Contents:");
    //Serial.println(datReq);

    String CMD="";
    long DATA=0;
    
    for (int i = 0; i < datReq.length(); i++) {
        if (datReq.substring(i, i+1) == "|") {
            CMD = datReq.substring(0, i);
            DATA = datReq.substring(i+1).toInt();
            
              break;
        }
    }
    
  
    if(CMD=="S"){
       //Start();
       setpoint=(double)DATA;
       Send("Start OK",1);
       //Serial.println("Start ok");
    }

  if(CMD=="ST"){
      //Stop();
      Send("Stop OK",1);
      Serial.println("Stop ok");
    }
    

    // send a reply, to the IP address and port that sent us the packet we received
    /*
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
    */
 }


 

void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    receiveData(packetSize);
  }
  
  /*
  if (millis() - startTime >= 10000) { 
    // Si ha pasado 1 segundo (1000 milisegundos) desde startTime
    // Your code here
    if (WiFi.status() != WL_CONNECTED) {
      connectWifi();
    }
    startTime = millis(); // reiniciar el contador
  }
  */

  
  

  if (millis() - startTime >= 250) { 

    velocidad = count;
    input=(double)velocidad;
    count=0;
    startTime = millis(); // reiniciar el contador
  }
  myController.compute();
  
  analogWrite(pwmMotorB, (int)output);


   Serial.print(setpoint);
   Serial.print(",");
   Serial.print(input);
   Serial.print(",");
   Serial.print(output);
   Serial.print(",");
   Serial.println(0);
  
}

void ISRoutine () {
  count=count+1;
}

/*
  test (shell/netcat):
  --------------------
    nc -u 192.168.esp.address 8888
*/
