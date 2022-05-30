#include <SPI.h>
#include <RH_RF95.h>
#include <AESLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 20, 4);

int pin = 4;
int btn = 3; //Interrupt pins UNO 2,3
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
String x ="";

boolean btnState;

// Singleton instance of the radio driver
RH_RF95 rf95;


void setup(){
  pinMode(btn, INPUT);

  lcd.begin();
  lcd.backlight();
  delay(100);
  
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
  if (!rf95.init())
    Serial.println("init failed");

}

//-----------------------------------------------------------------------------------------------------------------

String decrypt_func(String var){
  char y[17];
  var.toCharArray(y, 17);
  aes256_dec_single(key, y);
  x = ((String)(char*)y);
  return x;
  
}

//-----------------------------------------------------------------------------------------------------------------

String incMsg(){
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (rf95.waitAvailableTimeout(3000)){
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)){
      x = ((String)(char*)buf);      //Metatrepei to periexomeno tou buffer se string
      decrypt_func(x);
      Serial.print("got reply: ");
      Serial.println(x);
      return x;
    }
  }
}

void recvMessage(){
      incMsg();
        if (x == "0123456789000101"){       //Prepei na einai 16 byte, periorismos library
          //lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Valve on");
          Serial.println("Valve on");
         }else{
           //lcd.clear();
           lcd.setCursor(0,0);
           lcd.print("Valve off");
           Serial.println("Valve off");
         }
      
      if (x.startsWith("send_sensor")){
        String sensorData = x.substring(11);
        lcd.setCursor(0,1);
        lcd.print("Flow " + sensorData + " L/M");
      }

      if (x == "0011001100110011"){
        Serial.println("Valve Failure!");
        lcd.setCursor(3,0);
        lcd.print("FAILURE! VALVE OFF!");
      }
    }

void ntwrkData(){
  int RssI = rf95.lastRssi();
  Serial.println(RssI);
  //To lcd.print(variable +"string") exei problima. Thelei lcd.setCursor kai print gia to kathena
  lcd.setCursor(0,3);
  lcd.print("RSSI");
  lcd.setCursor(5,3);
  lcd.print(RssI);
  lcd.setCursor(9,3);
  lcd.print("dB");
}

//-----------------------------------------------------------------------------------------------------------------

boolean btnDebouncing(boolean state){         //Apotrepei anepithimita pollapla patimata (1 push == 2-3)
  boolean currentState = digitalRead(btn);
  if (state != currentState){
    delay(10);
    currentState = digitalRead(btn);
  }
  return currentState;
}

//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

void loop(){   
    ntwrkData();
    recvMessage();
    int myRssI = rf95.lastRssi();
    Serial.print("btn RSSI: ");
    Serial.println(myRssI);
    
    if (btnDebouncing(btnState) == true){
        Serial.println("Sending to rf95_server");
        uint8_t data[] = "0123456789000001";        // Send a message to rf95_server (supports only 16 bytes, hence the string)
        lcd.setCursor(0,0);
        aes256_enc_single(key, data);               // Encrypts the message https://github.com/DavyLandman/AESLib
        rf95.send(data, sizeof(data));
        rf95.waitPacketSent();
        lcd.clear();
        recvMessage();
        delay(100);
        
      }
}
