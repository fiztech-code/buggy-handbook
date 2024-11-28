#include <LedControl.h>


/* RPM */
int rpm = 0;
const byte rpmPin = 2;
volatile unsigned long rpmTime1 = 99999;
unsigned long rpmTime = 0;
unsigned long rpmUpdate = 0;
/* /RPM */

/* SPEED */
int spd = 0;
const byte spdPin = 3;
volatile unsigned long spdTime1 = 99999;
unsigned long spdTime = 0; 
unsigned long spdUpdate = 0;
/* /SPEED */

/* ENGINE COOLANT TEMP */
int ect = 0, ectValue = 0, ectLedCount = 0;
const byte ectPin = 0;
unsigned long ectUpdate = 0;
/* /ENGINE COOLANT TEMP */

/* OIL NEUTRAL */
const byte oilPin = 4, neutralPin = 5;
byte oil = 0x80, neutral = 0x40;
unsigned long oanUpdate = 0;
/* /OIL NEUTRAL */

/* Matrix */
int DIN = 12, CS = 11, CLK = 10, i = 0;

LedControl lc = LedControl(DIN, CLK, CS, 0);

byte displayData[8] = {
	0x00, // rpm1
	0x00, // rpm2
	0x00, // spd1
	0x00, // spd2
	0x00, // spd3
	0x00, // temp1
	0x00, // temp2
	0x00  // oilSwitch - 0x80, neutralSwitch - 0x40
};

byte displayDataOld[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

byte segDigits[10] {
	0xFC, // 0
	0x60, // 1
	0xDA, // 2
	0xF2, // 3
	0x66, // 4
	0x96, // 5
	0xBE, // 6
	0xE0, // 7
	0xFE, // 8
	0xF6  // 9
};
/* /Matrix */

void setup() { 	
	//attachInterrupt(digitalPinToInterrupt(rpmPin), getRPM, RISING); // add monitor pin to RPM pulse
	//attachInterrupt(digitalPinToInterrupt(spdPin), getSPD, RISING); // add monitor pin to SPD pulse
	
	lc.shutdown(0, false);       // The MAX72XX is in power-saving mode on startup
	lc.setIntensity(0, 15);      // Set the brightness to maximum value
	lc.clearDisplay(0);          // and clear the display
}

void loop() { 
	if (millis() > rpmUpdate) {
		calcRpm();		
		rpmUpdate = millis() + 50;
	}
	if (millis() > spdUpdate) {
		calcSpd();		
		spdUpdate = millis() + 500;
	}
	if (millis() > ectUpdate) {
		calcEct();		
		ectUpdate = millis() + 5000;		
		
		/*
		if (rpmTime1 + 1000000 < micros()) {
			spdTime = 0;
			rpmTime = 0;
		}
		*/		
	}		

	calcOan();	
	printByte(displayData, displayDataOld); 
}

void calcRpm() {
	//rpm = 60000 / (rpmTime * 6);	
	rpm = 60000 / (pulseIn(rpmPin, HIGH, 1000) * 6);
	displayData[0] = 0xFFFF >> (16 - rpm);	// Set rpm1 byte	
	displayData[1] = 0xFFFF >> (24 - rpm); 	// Set rpm2 byte	
}

void calcSpd() {
	//spd = 36000 / spdTime;
	spd = 36000 / (pulseIn(spdPin, HIGH, 1000));
	
	if (spd < 100) {
		displayData[3] = 0xFF;
	} else {
		displayData[3] = segDigits[spd / 100];
	}

	if (spd < 10) {
		displayData[2] = 0xFF;
	} else {
		displayData[2] = segDigits[spd % 100 / 10];	
	}

	displayData[4] = segDigits[spd % 10];	
}

void calcEct() {
    ect = analogRead(ectPin);
	ect = (ect + 12) / 85;
	//displayData[5] = 0xFF >> ((ect + 12) / 128);
	
	displayData[5] = 0xFFFF >> (16 - ect);	// Set temp1 byte	
	displayData[6] = 0xFFFF >> (24 - ect); 	// Set temp2 byte	
}

void calcOan() { 
	if (digitalRead(oilPin) == LOW) {
		oil = 0x00;
	} else {
		oil = 0x80;		
	}		

	if (digitalRead(neutralPin) == LOW) {
		neutral = 0x00;
	} else {
		netural = 0x40;	
	}

	displayData[7] = oil | neutral;
}
/*
void getRPM() {	
	if (rpmTime1 == 99999) {
		rpmTime1 = micros();
	} else {
		rpmTime = micros() - rpmTime1;
		rpmTime1 = 99999;
	}
}

void getSPD() {
	if (spdTime1 == 99999) {
		spdTime1 = micros();
	} else {
		spdTime = micros() - spdTime1;
		spdTime1 = 99999;
	}
}
*/
void printByte(byte character [], byte characterOld []) {
	for (i = 0; i < 7; i++) {
		if (character[i] != characterOld[i]) {
			lc.setRow(0, i, character[i]);
			characterOld[i] = character[i];
		}
	}
}
