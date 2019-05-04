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
#pragma once
//#define IRSonyTrace // comment out this line to save memory by disabling debug features
#include "PinChangeInterruptHandler.h"

class TIRSony : protected PinChangeInterruptHandler {
public:
  struct TMsg {
    uint16_t mGapTime; // time of the gap between the last two receptions in 128 µs increments
    uint16_t mLenTime; // how long did it take to receive this message in µs (non-zero means new valid message)
    uint8_t mCmd; // command code of the last sucessful reception
    uint8_t mAdr; // address code of the last sucessful reception
  };
private:
// Sony SIRC Protocol:
// Pulse width modulation with carrier frequency of 40kHz.
//  * 12-bit version, 7 command bits, 5 address bits.
//  * 15-bit version, 7 command bits, 8 address bits.
//  * 20-bit version, 7 command bits, 5 address bits, 8 extended bits.
// For more details see: https://www.sbprojects.net/knowledge/ir/sirc.php
  static constexpr uint16_t cMaxRepeatGap = 586; // max gap in 128 us units between msg copies for error checking
  static constexpr uint32_t cTickTime = 600; // Sony IR Tick Time in µs (minimum time for high or low  signal level)
  static constexpr uint32_t cStartTime = 2400; // time of the start bit in µs
  static constexpr uint32_t cZeroTime = 600; // time indicating logical 0 in µs
  static constexpr uint32_t cOneTime = 1200; // time indicating logical 1 in µs
  static constexpr uint8_t cCmdBits = 7; // number of bits in command field
  static constexpr uint8_t cAdrBits = 5; // number of bits in address field
  static constexpr bool cIROn = false; // logical level on mPin when IR is shining
#ifdef IRSonyTrace
public:
  struct TTrace {
    uint8_t mIndex;
    uint16_t mTime; 
  };
  static constexpr uint8_t cTraceLen = (1+cCmdBits+cAdrBits+1)*2;
  uint8_t mTraceTop;
  TTrace mTrace[cTraceLen];
  uint32_t mInterruptCnt;
#endif
  // Configuration
  uint8_t mPin; // arduino pin number connected to the IR receiver
  // Bit reception data (information about the last bit received over IR)
  uint32_t mLastHalfBitTime; // time of the last proper bit transition in µs
  uint32_t mLastFlipTime;    // time of the last IR signal change in µs
  uint8_t mLastLevelIndex;   // inactive gap: ~0; start bit 1st half: 0; start bit 2nd half: 1;
                             // 1st bit 1st half: 2, 1st bit 2nd half: 3, 2nd bit 1st half: 4; ...
  uint8_t mLastErrorAtLevelIndex; // the level index at which the last IR read error accoured
  bool mRequestTinyErrorLevel;    // true when a too early level change happened
  // Message reception data
  uint32_t mLastMsgTime; // time of the last bit of the last valid message in µs
  uint32_t mRecvMsgTime; // start bit time of the message being received just now
  uint8_t mMsgIdx; // index of the message which is being received just now
  TMsg mMsg[4]; // the last fully received messages and the message just being received
protected:
  virtual void handlePCInterrupt(int8_t intNum, bool val) override; // processing of the IR input state change
public:
  static constexpr uint8_t cGapIdx = 0xff; // index of the signal level between messages
  TIRSony() { Initialize(); }
  void Initialize();
  bool PopMsg(TMsg& msgToPopInto); // returns false if there is not message to pop
  bool Start(uint8_t pin); // start receiving on the IR receiver
  bool Stop(); // stop receiving on the IR receiver
  uint8_t GetLastErrorLevelIndex() { return mLastErrorAtLevelIndex; }
};

extern TIRSony IRSony;
