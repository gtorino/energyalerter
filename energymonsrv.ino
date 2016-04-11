#include <EmonLib.h>

/*
 TCP/IP server/Energymonitor merge
 */

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <avr/wdt.h>                                                     
#include "EmonLib.h"

//-----------------------codice dal emontx --------
//#define FILTERSETTLETIME 5000                                           //  Time (ms) to allow the filters to settle before sending data

#define CT1  1 

//const int nodeID = 10;                                                  // emonTx RFM12B node ID
//const int networkGroup = 210;                                           // emonTx RFM12B wireless network group - needs to be same as emonBase and emonGLCD
const int UNO = 1;                                                      // Set to 0 if your not using the UNO bootloader (i.e using Duemilanove) - All Atmega's shipped from OpenEnergyMonitor come with Arduino Uno bootloader

EnergyMonitor ct1;                                              // Create  instances for each CT channel

int emontx_power;
const int LEDpin = 9;                                                   // On-board emonTx LED 

//-----------------------codice dal emontx --------
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 150);
unsigned int localPort = 1500;      // local port to listen on
//IPAddress myDns(192,168,1, 3);
//IPAddress gateway(192, 168, 1, 3);
//IPAddress subnet(255, 255, 255, 0);


// Initialize the Ethernet server library
// with the IP address and port you want to use
EthernetServer server(localPort);
boolean alreadyConnected = false; // whether or not the client was connected previously

void setup() {
 // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
 
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  Serial.print("port: ");
  Serial.println(localPort);

  if (CT1) ct1.currentTX(1, 111.1);                                     // Setup emonTX CT channel (ADC input, calibration)

  pinMode(LEDpin, OUTPUT);                                              // Setup indicator LED
  digitalWrite(LEDpin, LOW);
  
//  if (UNO) wdt_enable(WDTO_8S);                                         // Enable anti crash (restart) watchdog if UNO bootloader 
                                                                        // Restarts emonTx if sketch hangs for more than 8s
}

void loop() {
// listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    if (!alreadyConnected) {
        // clear out the input buffer:
        client.flush();
        Serial.println("New client");
        alreadyConnected = true;
    }
  
    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      if (thisChar == '@') {  //il carattere "trigger" della risposta del server
        // invio il dato sulla potenza incluso tra"#"
        emontx_power = ct1.calcIrms(1480) * 230.0;      //ct.calcIrms(number of wavelengths sample)*AC RMS voltage
        //server.write("#");
        server.print(emontx_power);
        server.write("#");
        //output su seriale
        Serial.print("Power: ");                                         
        Serial.println(emontx_power);                                         
        // give the client time to receive the data
        delay(5);
        // close the connection:
     }
     client.stop();
     Serial.println("client disconnected");
    }
 }
}  
