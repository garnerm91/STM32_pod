#include <eXoCAN.h>
#include <Stepper.h>
#include <Adafruit_NeoPixel.h>

// backlight setup
Adafruit_NeoPixel pixels(3, PB4, NEO_GRB + NEO_KHZ800);
char lrgb[3];

// Stepper & erro light setup
int stepsPerRevolution = 635;
int stepperSpeed = 60;
int pos, val, light, motor;

Stepper x27(stepsPerRevolution, PB6, PB7, PB8, PB9); // initialize the stepper library


// Can setup
eXoCAN can(STD_ID_LEN, BR250K, PORTA_11_12_XCVR);
int PodId = 0x003;   // ID for rx filtering   <--- Must be different for each pod
int id, fltIdx;

//Read can Function
inline void canRead(bool print = false) 
{
  if (can.rxMsgLen >= 0)
  {
    if (print)
    {
      if (can.id == PodId)
      {// if statement catches any missed by filter.
      light = (can.rxData.bytes[0]); // first byte is for idiot light
      int m1 = (can.rxData.bytes[1]); // next 3 are added to make the 635 steps of the stepper motor
      int m2 = (can.rxData.bytes[2]);
      int m3 = (can.rxData.bytes[3]);
       motor =   m1 + m2 + m3;
    }
    if(can.id == 0x002){ //0x002 is the backlight
      lrgb[1] = (can.rxData.bytes[0]);
      lrgb[2] = (can.rxData.bytes[1]);
      lrgb[3] = (can.rxData.bytes[2]);
      }
    can.rxMsgLen = -1;
  }
  // return id;
}}

void canISR() // get bus msg frame passed by a filter to FIFO0
{
  can.filterMask16Init(0, PodId, 0x7ff, 0x002, 0x7ff); // supposed to filter messages... always misses the first wrong one
  can.rxMsgLen = can.receive(can.id, fltIdx, can.rxData.bytes); // get CAN msg
  
}

//light function
inline void idiot()
{
 if (light == 1)
 {
  digitalWrite(PB5, HIGH);
  } else {
  digitalWrite(PB5, LOW); 
  }
}

//zero function
inline void x27zero()
{
  x27.setSpeed(stepperSpeed);
  x27.step(-stepsPerRevolution);
  delay(10);
  x27.step(5); //less likely to bottom out now. leaves 595 useable steps but I suggest skipping last 5 also
  pos = 5;  // needs to be the same number as the skipped steps
}

void x27move(unsigned int npos)
{
 if (pos != npos && npos <= stepsPerRevolution){
   val = npos - pos;
   pos = npos;
   x27.step(val);
 }
}

void ws2812(){
  for(int i=0; i<3; i++) { 
    pixels.setPixelColor(i, pixels.Color(lrgb[1], lrgb[2], lrgb[3]));
    pixels.show();   
    }
}
void setup() {
  Serial.begin(112500);
  can.attachInterrupt(canISR);
  pinMode(PB5, OUTPUT); //idiot light pin
  pinMode(PB12, OUTPUT);
  digitalWrite(PB12, HIGH);
  x27zero();
  can.enableInterrupt(); // enable rx interrupt
  pixels.begin();
  pixels.clear();
}

void loop() {
  canRead(true); 
  idiot();
  x27move(motor);
  ws2812();
}
