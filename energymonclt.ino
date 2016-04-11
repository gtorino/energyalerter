/*
 * UIPEthernet TcpClient for Energymon.
 * nota: per funzionare con LCD shield occorre modificare il CS//SS verso l'ENC da D10 (usato dall'LCD)
 * a D2
 * occorre modificare il file \Arduino\libraries\arduino_uip-master\utility\Enc28J60Network.h
 * modificando ENC28J60_CONTROL_CS da "SS" a "2"
 * modificato anche \Arduino\libraries\arduino_uip-master\utility\uipethernet-conf.h 
 * per il timeout della client.connect()
 * #define UIP_CONNECT_TIMEOUT      1 (era -1)
 */

#include <avr/wdt.h>    //per il watchdog

#include <LiquidCrystal.h>
 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);           // select the pins used on the LCD panel
 
// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
int elapsed = 0;
int curr_mill;
String strpwr="";

#define MaxPower 3990
#define backlight 10 
#define buzzer  3

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#include <UIPEthernet.h>

EthernetClient client;
signed long next;
IPAddress ip(192, 168, 1, 151);
uint8_t  mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };


int read_LCD_buttons(){               // read the buttons
    adc_key_in = analogRead(0);       // read the value from the sensor 
 
    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result
 
    if (adc_key_in > 1000) return btnNONE; 
 
    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  
 
   // For V1.0 comment the other threshold and use the one below:
 
    return btnNONE;                // when all others fail, return this.
}



void setup() {

  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  Serial.print("localIP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());

  lcd.begin(16, 2);               // start the library
  lcd.setCursor(0,0);             // set the LCD cursor   position 
  pinMode(backlight, OUTPUT);      // sets the digital pin as output
  digitalWrite(backlight, HIGH);   // accende la luce di sfondo del display  
  //pinMode(buzzer, OUTPUT);
  tone(buzzer, 1000);
  delay(50);
  noTone(buzzer);
  wdt_enable(WDTO_8S);   //abilito il watchdog
  next=0;
}

void loop() {

  lcd.setCursor(0,0);             // move to the begining of the second line
  lcd_key = read_LCD_buttons();   // read the buttons
  unsigned long time;
  time= millis();
  wdt_reset();      //watchdog reset
   
  switch (lcd_key){               // depending on which button was pushed, we perform an action
    case btnSELECT:{
        digitalWrite(backlight, HIGH);   
        curr_mill = millis()/1000;
        break;
    }
    case btnLEFT:{
        break;
    }

  } 
  elapsed= time/1000 - curr_mill;
  if (elapsed > 350) digitalWrite(backlight, LOW);

  if (((signed long)(millis() - next)) > 0)  {
      next = millis() + 5000;
      Serial.println("Connecting emontx...");
      if (client.connect(IPAddress(192,168,1,150),1500)>0 )    {
          Serial.println("Connected!");
          client.write('@');
          Serial.println("DATA from Client");
          while(client.available()==0)    {
              if (next - millis() < 0)
                goto close;
          }
          int size;
          while((size = client.available()) > 0)  {
              uint8_t* msg = (uint8_t*)malloc(size);
              size = client.read(msg,size);
              //copio il buffer nella stringa
              strpwr="";
              for (int i=0; i<= size-1; i++) { 
                if (char(msg[i])!= '#')
                      strpwr += char(msg[i]);
                else 
                  break;
              }
              Serial.println(strpwr);
              lcd.clear();
              lcd.setCursor(0,0);            // move to the begining of the second line
              lcd.print("Power ");
//              lcd.setCursor(0,1);            // move to the begining of the second line
              lcd.print(strpwr);
              lcd.print(" watts");
              if(strpwr.toInt()>MaxPower){       //se la potenza supera la massima da contratto
                digitalWrite(backlight, HIGH);   
                lcd.setCursor(0,1);            // move to the begining of the second line
                lcd.print("Pot. superata!");
                curr_mill = millis()/1000;
                tone(buzzer, 1000);          //il buzzer suona
                delay(500);
                noTone(buzzer);
                delay(1000);
              }
              
              free(msg);
          }
close:
          //disconnect client
          Serial.println("Client disconnect");
          client.stop();
      } else {
          lcd.clear();
          lcd.setCursor(0,0);            // move to the begining of the second line
          lcd.print("No connection");
          Serial.println("Client connect failed");
     }
  }
}

