/*
 * Copyright (C) 2019  Peter Hercek (phercek@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "IRSony.h"
#include "Joystick.h"

// IR receive config
#define pinIR 8
const uint8_t cCycleDelay = 2;
const uint16_t cLedOffCycleMax = 500;
uint16_t ledOffCycleCnt = 0;

#ifdef IRSonyTrace
uint8_t lastErrIdx = TIRSony::cGapIdx;
uint16_t traceCnt = 0;
#endif //IrSonyTrace

// Pedals state (data are sent only when changes are detected)
int lastSensorValue = 0;
boolean isSensorValueGrowing = false;
signed char lastCalibratedValue = 0;


void setup() {
  while (!Serial);
  Serial.begin(115200);
  
  pinMode(LED_BUILTIN, OUTPUT);

  if (!IRSony.Start(pinIR))
    Serial.println(F("You did not choose a valid pin."));
    
#ifdef IRSonyTrace
  Serial.print("IR pin state: "); Serial.println(digitalRead(pinIR));
#endif //IRSonyTrace
}

void loop() {
  PedalsLoop();
  IrLoop();
  delay(cCycleDelay); // 1.25ms usb polling time; 1ms ADC settle time
  if (ledOffCycleCnt > 0)
    if (0 == --ledOffCycleCnt)
      digitalWrite(LED_BUILTIN, LOW);
}

void PedalsLoop() {
  // handle the input on analog pin 0:
  int sensorValue = analogRead(A0);
  if (lastSensorValue!=sensorValue) {
    if (isSensorValueGrowing) {
      if ( (lastSensorValue-1)!=sensorValue )
        handleSensorValueChange( sensorValue );
    } else { // it is falling
      if ( (lastSensorValue+1)!=sensorValue )
        handleSensorValueChange( sensorValue );
    }
  }  
}

void IrLoop() {
  TIRSony::TMsg msg;
  if (IRSony.PopMsg(msg)) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOffCycleCnt = cLedOffCycleMax;
#ifdef IRSonyTrace
    Serial.print(F("\n Address: ")); Serial.print(msg.mAdr);
    Serial.print(F("   Command: ")); Serial.print(msg.mCmd);
    Serial.print(F("   GapTime: ")); Serial.print(msg.mGapTime);
    Serial.print(F("   LenTime: ")); Serial.print(msg.mLenTime);
#else
    Serial.print(msg.mAdr); Serial.print(':'); Serial.print(msg.mCmd); Serial.print('\n');
#endif
  }
#ifdef IRSonyTrace
  // print the location of IR decoding error
  if (IRSony.mLastErrorAtLevelIndex != TIRSony::cGapIdx && IRSony.mLastErrorAtLevelIndex != lastErrIdx) {
    lastErrIdx = IRSony.mLastErrorAtLevelIndex; Serial.print("\n\tlastErrIdx: "); Serial.print(lastErrIdx); }
  if (++traceCnt > 1000) {
    IRSony.mLastErrorAtLevelIndex = TIRSony::cGapIdx; // reset IRSony error code
    lastErrIdx = TIRSony::cGapIdx; traceCnt = 0; // reset external IRSony traciking state
    // dump the IR receiver status
    if (IRSony.mInterruptCnt) {
      Serial.print("\ninterruptCnt: "); Serial.print(IRSony.mInterruptCnt);
      IRSony.mInterruptCnt = 0;
      Serial.println("\nlevel duration[us]");
    }//if
    for (uint8_t i = 0; i < IRSony.mTraceTop; ++i) { 
      if (IRSony.mTrace[i].mIndex == TIRSony::cGapIdx) Serial.print('X');
      else Serial.print(IRSony.mTrace[i].mIndex);
      Serial.print(" "); Serial.println(IRSony.mTrace[i].mTime);
    }//for 
    IRSony.mTraceTop = 0;
    Serial.print("*");
  }//if
#endif //IRSonyTrace
}

void handleSensorValueChange(int sensorValue) {
  isSensorValueGrowing = sensorValue > lastSensorValue;
  lastSensorValue = sensorValue;
  //Serial.println(sensorValue);
  int8_t calibratedValue = calibrate(sensorValue);
  if ( calibratedValue != lastCalibratedValue ) {
    lastCalibratedValue = calibratedValue;
    Joystick.SetRudder(calibratedValue);
    digitalWrite(LED_BUILTIN, HIGH);
    ledOffCycleCnt = cLedOffCycleMax;
    //Serial.print(' '); Serial.print(calibratedValue);
  }
}

int const calibData[] = {
120,121,122,123,124,125,126,127,128,129,
130,131,132,133,135,136,137,138,139,140,
141,142,143,145,146,148,149,151,152,154,
155,157,158,160,162,164,166,168,170,173,
175,177,179,181,183,185,187,189,191,193,
195,197,200,202,204,206,208,210,212,215,
217,219,221,223,226,228,231,234,236,239,
242,245,247,250,253,256,259,262,265,268,
271,274,277,280,283,286,289,293,296,300,
303,306,310,313,317,320,323,327,331,334,
338,341,345,349,352,356,359,363,367,371,
375,379,383,387,391,395,399,403,406,410,
414,417,421,424,428,432,436,    442,447,
451,455,459,463,468,472,476,480,484,488,
492,496,500,504,508,512,516,520,524,528,
532,536,540,545,549,553,557,561,565,569,
573,577,581,585,589,593,597,601,604,608,
612,616,620,624,628,632,635,639,643,647,
651,655,658,662,665,669,673,676,680,683,
687,690,694,697,700,703,707,710,713,716,
719,723,726,729,732,735,739,742,745,748,
752,755,758,761,764,767,769,772,775,778,
781,784,786,789,791,794,796,799,801,803,
806,808,810,813,815,817,819,821,823,825,
827,829,831,833,835,836,838,839,841,842,
844,845,847,848,850};
int const calibDataLen = sizeof(calibData) / sizeof(calibData[0]);
int const calibValLow = -127;
int const calibValHigh = 127;

signed char calibrate(int rawValue) {
  if ( rawValue >= calibData[calibDataLen-1] )
    return calibValHigh;
  int i = 0;
  int j = calibDataLen-1;
  while (i < j) {
    int k = (i+j)>>1;
    if (rawValue < calibData[k]) j = k;
    else i = k+1;
  }
  return i + calibValLow;
}
