/*
  Hardware interface between honda cbr600f4i ECM and yamaha yzf-r6 cluster
  ------------------------------------------------------------------------
  cbr outputs all parameters as analog values, yzf is using k-line serial comm for speed, engine coolant temperature, fault indicator
  this sketch takes analog parameters processes them, then converts to serial message

  the neutral, turn signals, and high beam function as standard bulb indicators. 
  however, the oil indicator on the YZF is an oil level sensor, while on the CBR, it is an oil pressure sensor.
  therefore there is a 30sec delay, also requires resisters
*/

#include <SoftwareSerial.h>
#include <math.h>

// Engine Coolant Temp vars
uint8_t ect_map[] = { 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 
                      51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 
                      72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
                      93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
                      112, 113, 115, 116, 118, 119, 121, 122, 124, 125, 126, 127, 128, 129, 130, 131, 132, 
                      133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 
                      150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 
                      167, 168, 169, 170 };

uint16_t ectRaw = 0;
uint8_t ect = 0;
float ectFloat = 0;
unsigned long ectUpdate = 0;

// Speed vars
uint8_t spd_map[] = { 0, 6, 12, 18, 24, 31, 37, 44, 50, 57, 63, 70, 76, 83, 89, 96, 102, 109, 115, 122,
                      128, 135, 141, 148, 154, 161, 167, 174, 180, 187, 193, 200, 206, 212, 219, 225, 232,
                      238, 245, 251, 258, 264, 271, 277, 284, 290, 297, 299 };
uint8_t spd = 0;
float spdFloat = 0;
volatile unsigned long spdTimeStart = 0;
volatile unsigned long spdTime = 0;
volatile uint8_t spdPulse = 0;
uint8_t real_spd = 0;  
uint8_t real_spd_prev = 0;
uint8_t spd_steps = 0;
uint8_t spd_diff = 0;
uint8_t spd_bit = 0;
unsigned long spdUpdate = 0;


// Fault Indicator var
uint8_t fi = 0;


// K-Line Vars
SoftwareSerial KLine(10, 11); // K-Line RX, TX
uint8_t inByte = 0;
uint8_t blankMessage[] = {0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t message[] = {0x00, 0x00, 0x00, 0x30, 0x30};
bool respond = false;
unsigned long respondDelay = 0;


float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void setup() {
  KLine.begin(16064);
  attachInterrupt(digitalPinToInterrupt(3), getSPD, RISING);
  
  pinMode(A0, INPUT);
  pinMode(3, INPUT);
  pinMode(6, INPUT_PULLUP);
}

void SendResponse() {    
  if (inByte == 0x01) { 
    message[0] = 0;     // Rpm 

    /*
      yamaha cluster accepts speed values from 0-47 and internally these values to 0-299
      it takes 6x messages to send speed value
      
      ie.  actual speed is 21kmh, which falls between 18kmh(index:3) and 24kmh(index:4)
      therefore we need to send 3x messages with speed value:4, then send 3x messages with
      speed value:3 to get desired result 21 on cluster
    */
    // spd_map
    if (real_spd != real_spd_prev) {
      real_spd_prev = real_spd;
      for (int i = 0; i < 48; i++) {
        if (real_spd < spd_map[i]) {
          spd_steps = spd_map[i] - spd_map[i-1];
          spd_diff = real_spd - spd_map[i-1]; 
          spd = i - 1;
          i = 48;
        }
      }
    }
    
    spd_bit = 0;
    if (spd_steps > 0) {
        if (spd_diff > 0) {
            spd_bit = 1;
            spd_diff--;
        }
        spd_steps--;
    } 
    
    message[1] = spd + spd_bit; // Spd 
    
    if (spd_steps == 0) {
       real_spd_prev = 0; 
       spd = 0;
    }   
        
    message[2] = fi;    // Error          
    message[3] = ect;   // Temp
    message[4] = message[3] + message[2] + message[1] + message[0]; // Checksum
    KLine.write(message, sizeof(message));    
  } else {
    /*
      Request Messages:
      0xFE -> Begin Communication
      0xCD -> Diagnostic Mode
    */
    KLine.write(blankMessage, sizeof(blankMessage));   
  }
  respond = false;
}
 
void loop() {
  
  if (millis() > spdUpdate) {
    if (spdTime < 1500000 && spdTime > 25000) {      
      spdFloat = (0.00207 / spdTime * 3600000000);      
      spdFloat = mapFloat(spdFloat, 0, 299, 0, 47);      
      real_spd = round(spdFloat);      
    }
    
    if (spd > 0 && micros() - spdTimeStart > 1600000){
      spdTime = 0;      
      spd = 0;     
    }    
  
    fi = 0x80 >> digitalRead(6) & 0xBF;  
    spdUpdate = millis() + 125;
  }

  if (millis() > ectUpdate) {
    ectRaw = analogRead(A0);  
    ectFloat = mapFloat(ectRaw, 1024, 540, 20, 120);     
    ect = round(ectFloat);
    
    if (ect > 117) {
      ect = 170;
    } else {
      ect = ect_map[ect];
    }
    
    ectUpdate = millis() + 2000;
  }

  if (KLine.available()) {
    inByte = KLine.read();
    respond = true;  
    respondDelay = micros() + 200;
  }

  if (respond && micros() > respondDelay) {
    SendResponse();   
  }   
}


void getSPD() {
  if (spdPulse == 0) {
    spdTimeStart = micros();
    spdPulse++;    
  } else if (spdPulse > 111) {
    spdTime =  micros() - spdTimeStart;
    spdPulse = 0;
  } else {
    spdPulse++;  
  }  
}
