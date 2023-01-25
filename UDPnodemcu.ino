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

unsigned int localPort = 8888;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged\r\n";       // a string to send back
String datReq;
WiFiUDP Udp;
unsigned long startTime;

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
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                  pktsize,
                  Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                  Udp.destinationIP().toString().c_str(), Udp.localPort(),
                  ESP.getFreeHeap());

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
    Serial.println("Contents:");
    Serial.println(datReq);

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
       Send("Start OK",1);
       Serial.println("Start ok");
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


  if (millis() - startTime >= 10000) { 
    // Si ha pasado 1 segundo (1000 milisegundos) desde startTime
    // Your code here
    if (WiFi.status() != WL_CONNECTED) {
      connectWifi();
    }
    startTime = millis(); // reiniciar el contador
  }

  

}

/*
  test (shell/netcat):
  --------------------
    nc -u 192.168.esp.address 8888
*/
