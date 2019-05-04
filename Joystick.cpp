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
#include "Joystick.h"
#include <HID.h>
#if !defined(_USING_HID)
  #error "The targeted core is not pluggable-ready!"
#endif 

TJoystick Joystick;

static const uint8_t sReportDescriptor[] PROGMEM = {
// Joystick (rudder pedals only)
  0x05, 0x01,  // USAGE_PAGE (Generic Desktop) // 42
  0x09, 0x04,  // USAGE (Joystick)
  0xa1, 0x01,  // COLLECTION (Application)
  0x85, 0x04,  //   REPORT_ID (4)
  0x05, 0x09,  //   USAGE_PAGE (Button)
  0x19, 0x01,  //     USAGE_MINIMUM (Button 1)
  0x29, 0x08,  //     USAGE_MAXIMUM (Button 8)
  0x15, 0x00,  //     LOGICAL_MINIMUM (0)
  0x25, 0x01,  //     LOGICAL_MAXIMUM (1)
  0x75, 0x01,  //     REPORT_SIZE (1)
  0x95, 0x08,  //     REPORT_COUNT (8)
  0x81, 0x02,  //     INPUT (Data,Var,Abs)
  0x05, 0x02,  //   USAGE_PAGE (Simulation Controls)
  0x15, 0x81,  //     LOGICAL_MINIMUM (-127)
  0x25, 0x7f,  //     LOGICAL_MAXIMUM (127)
  0xa1, 0x00,  //     COLLECTION (Physical)
  0x09, 0xba,  //       USAGE (Rudder)
  0x75, 0x08,  //       REPORT_SIZE (8)
  0x95, 0x01,  //       REPORT_COUNT (1)
  0x81, 0x02,  //       INPUT (Data,Var,Abs)
  0xc0,        //     END_COLLECTION
  0xc0         // END_COLLECTION
};

TJoystick::TJoystick() {
  mRudder = 0;
  // Register HID Report Description
  static HIDSubDescriptor node(sReportDescriptor, sizeof sReportDescriptor);
  HID().AppendDescriptor(&node);
}

int TJoystick::SetRudder(int8_t val) {
  uint8_t data[2];
  data[0] = 0; // we do not use buttons but they must be there for OS to recognize us as joystick
  data[1] = mRudder = val;
  return HID().SendReport(/*myReportId=*/4, data, sizeof data);
}
