#include <SPI.h>
#include <RH_RF95.h>
#include <AESLib.h>

// Singleton instance of the radio driver
RH_RF95 rf95;

int valve = 4;
String x = "";
uint8_t key[]= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
int flowPin = 3;
float flowRate;
volatile int flowCount;

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

void setup() 
{
  pinMode(valve, OUTPUT);     
  Serial.begin(9600);
  pinMode(flowPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), Flow, RISING);
  //dht.begin();
  while (!Serial) ; // Wait for serial port to be available
  if (!rf95.init())
    Serial.println("init failed");
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void Flow(){
  flowCount++;
}

//---------------------------------------------------------------------------------------
//Elegxos omalis rohs nerou
float flowAvg(){
  flowCount = 0;
  attachInterrupt(digitalPinToInterrupt(3), Flow, RISING);   //Flow, RISING); //Anti gia interrupt()-noInterrupt()
  delay(1000);                                                //(kanoun stall to programma)
  detachInterrupt(digitalPinToInterrupt(3));
  flowRate = (7.5*flowCount)/60; //    L/m   y = ax+b (y L/h, a=7.5, x=Hz, b=0)
  Serial.print("Flow rate = ");
  Serial.println(flowRate);
  return flowRate;
}

//----------------------------------------------------------------------------------------
//Sunartisi apokruptografisis eiserxomenou minimatos
String decrypt_func(String var){
  char y[17];                 //Metatrepei to string tou buffer se char array gia na mporesei na to apokriptografisei
  var.toCharArray(y, 17);     //kai meta pali se string
  aes256_dec_single(key, y);
  x = ((String)(char*)y);
  return x;
}

//----------------------------------------------------------------------------------------
//Elegxos katastasis valvidas
void valveCheck(int state){
  if (state == 0){
        uint8_t data[] = "0123456789000010";
        aes256_enc_single(key, data);
        rf95.send(data, sizeof(data));
        rf95.waitPacketSent();
        Serial.println("Reply sent!");
      }else {
        uint8_t data[] = "0123456789000101";
        aes256_enc_single(key, data);
        rf95.send(data, sizeof(data));
        rf95.waitPacketSent();
        Serial.println("Reply sent!");
      }
}

//---------------------------------------------------------------------------------------
//Sunartisi se periptosi apotuxias tis vanas
void failure(){
  digitalWrite(valve, LOW);
  Serial.println("Failure");
  uint8_t data[] = "0011001100110011"; //failure: Error, valve off
  aes256_enc_single(key, data);
  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
  Serial.println("Sent");
      
}

//-----------------------------------------------------------------------------------------
//Xeirismos tis vanas analoga me to minima pou tha labei
void valveControl(){
    if (rf95.available()){
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if (rf95.recv(buf, &len))
      {
        x = ((String)(char*)buf);
        decrypt_func(x);
        int valveState = digitalRead(valve);
        if (x == "0123456789000001" && valveState == 0){      //Prepei na einai 16 byte, periorismos library
          digitalWrite(valve, HIGH);
        }else {
          digitalWrite(valve, LOW);
        }
      }else{
        Serial.println("Receive failed");
    }
}
}
//-------------------------------------------------------------------------------------------------

void sensorReading(){
  String datta = "send_sensor" + String(flowRate);
  uint8_t data[17];
  datta.getBytes(data, sizeof(data));   //myString.getBytes(): Antigrafei tous xaraktires mias string se ena buffer (edo buffer = data)
  aes256_enc_single(key, data);
  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
}

//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

void loop(){
  int valveState = digitalRead(valve);
  flowAvg();
  sensorReading(); 
  if (((flowRate <= 3) || (flowRate >= 30)) && valveState == 1){ //An leitourgei kai den exei arketi roh
      failure();
  }else {
      valveCheck(valveState);
      
  }
    valveControl();
  delay(2000);   //Delay gia na prolabei to nero na perasei apo tin bana ston sensor
}
