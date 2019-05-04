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

TIRSony IRSony;

void TIRSony::Initialize() {
#ifdef IRSonyTrace
  mTraceTop = 0;
  memset(mTrace, 0, sizeof mTrace);
  mInterruptCnt = 0;
#endif
  mPin = 0;
  mLastHalfBitTime = 0;
  mLastFlipTime = 0;
  mLastLevelIndex = ~0;
  mLastErrorAtLevelIndex = ~0;
  mRequestTinyErrorLevel = false;
  mLastMsgTime = 0;
  mRecvMsgTime = 0;
  mMsgIdx = 0;
  memset(mMsg, 0, sizeof mMsg);
}

void TIRSony::handlePCInterrupt(int8_t /*intNum*/, bool val) {
  bool isIROn = cIROn == val;
  uint32_t curFlipTime = micros();
#ifdef IRSonyTrace
  if (mTraceTop < cTraceLen) {
    TTrace traceRec = {mLastLevelIndex, curFlipTime-mLastFlipTime};
    mTrace[mTraceTop++] = traceRec; }
  ++mInterruptCnt;
#endif
  if (mLastLevelIndex == cGapIdx && !isIROn)
    return; // synchronize gap signal level
  if (!(mLastLevelIndex&1) == isIROn) {
    // is this a return to the level where we should have been all the time
    bool theErrorLevelThischangeFinishesIsTiny = curFlipTime - mLastFlipTime < cTickTime/2;
    if (mRequestTinyErrorLevel && theErrorLevelThischangeFinishesIsTiny)
      return; // ignore returns to the proper signal level from tiny errors
    mLastHalfBitTime = curFlipTime;
    goto hardError; // hard error when a half of a bit got too long active error
  }
  mRequestTinyErrorLevel = false;
  // the level we switched to now correctly corresponds to the mLastLevelIndex
  uint32_t bitTimeDif = curFlipTime - mLastHalfBitTime;
  if (mLastLevelIndex == cGapIdx) { // gap between messages -> start bit active level
    mLastLevelIndex = 0; // start receiving a start bit
    uint32_t gapTime = (curFlipTime - mLastMsgTime) >> 7;
    if (gapTime > 0xFFFF) mMsg[mMsgIdx].mGapTime = 0xFFFF;
    else mMsg[mMsgIdx].mGapTime = gapTime;
    mMsg[mMsgIdx].mLenTime = 0;
    mMsg[mMsgIdx].mCmd = 0;
    mMsg[mMsgIdx].mAdr = 0;
    mRecvMsgTime = curFlipTime;
  }else if (mLastLevelIndex == 0) { // start bit active level -> start bit inactive level
    if (bitTimeDif < cStartTime-cTickTime)
      goto softError; // soft error (hopefully only short signal drop which will quickly return back)
    if (bitTimeDif > cStartTime+cTickTime)
      goto hardError; // hard error (the active level on this start bit was too long)
    mLastLevelIndex = 1; // correct end of the start bit 1st half; we expect low level ending the bit
  }else if ((mLastLevelIndex&1)) { // any bit inactive level -> next bit start
    if (bitTimeDif < cTickTime/2) goto softError; // soft error (hopefully only short signal rise in start bit ending)
    if (bitTimeDif > cTickTime*3/2)
      goto hardError; // hard error (the active level on this start bit was too long)
    ++mLastLevelIndex; // go on to process next bit
  }else{ // any bit active level 1st half -> it's inactive level 2nd half (i.e. its ending)
    if (bitTimeDif < cZeroTime/2) goto softError; // soft error (hopefully only short signal drop in bit 1st half)
    if (bitTimeDif > cOneTime+cTickTime/2)
      goto hardError; // hard error (the active level on this bit was too long)
    uint8_t curBit = (bitTimeDif< (cZeroTime+cOneTime)/2)? 0 : 1;
    uint8_t bitIndex = (mLastLevelIndex >> 1) - 1;
    if (bitIndex < cCmdBits)
      mMsg[mMsgIdx].mCmd |= curBit << bitIndex;
    else if (bitIndex < cCmdBits+cAdrBits)
      mMsg[mMsgIdx].mAdr |= curBit << (bitIndex-cCmdBits);
    if (bitIndex == cCmdBits+cAdrBits-1) {
      // we received the last bit possibly with wrong ending (ignores 2nd half length check)
      mMsg[mMsgIdx].mLenTime = curFlipTime - mRecvMsgTime;
      mLastMsgTime = curFlipTime;
      mMsgIdx = (mMsgIdx+1) & 3;
      mLastErrorAtLevelIndex = cGapIdx;
      mLastLevelIndex = cGapIdx; // go back waiting for start bit
    } else ++mLastLevelIndex; // go on to process the 2nd half of this bit
  }
  mLastHalfBitTime = curFlipTime;
  mLastFlipTime = curFlipTime;
  return;
softError:
  mRequestTinyErrorLevel = true;
  mLastFlipTime = curFlipTime;
  return;
hardError:
  mLastErrorAtLevelIndex = mLastLevelIndex;
  mLastLevelIndex = cGapIdx;
  mLastFlipTime = curFlipTime;
  return;
}

// returns false if there is not message to pop
bool TIRSony::PopMsg(TMsg& msgToPopInto) {
  // each message is sent at least 3 times; error checking is done by checking that a compies are the same
  uint8_t msgIdx = mMsgIdx; // this is an atomic operation (and the only one we really need to be atomic)
  uint8_t curIdx = (msgIdx+3) & 3;
  if (0 == mMsg[curIdx].mLenTime) return false; // ignore a message which was already consumed
  if (mMsg[curIdx].mGapTime > cMaxRepeatGap) return false; // ignore the first message after button press
  uint8_t prevIdx = (msgIdx+2) & 3;
  if (mMsg[prevIdx].mAdr != mMsg[curIdx].mAdr || mMsg[prevIdx].mCmd != mMsg[curIdx].mCmd)
    return false; // ignore message which had an error in transmision (it is not the same as the previous one)
  msgToPopInto = mMsg[curIdx];
  mMsg[curIdx].mLenTime = 0; // mark the message as consumed
  return true;
}

bool TIRSony::Start(uint8_t pin) {
  if (digitalPinToPCINT(pin) == NOT_AN_INTERRUPT) return false;
  mPin = pin;
  pinMode(mPin, INPUT_PULLUP);
  attachPCInterrupt(digitalPinToPCINT(mPin));
  return true;
}

bool TIRSony::Stop() {
  if (digitalPinToPCINT(mPin) == NOT_AN_INTERRUPT) return false;
  detachPCInterrupt(digitalPinToPCINT(mPin));
  return true;
}
