/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include <vector>
#include "RtMidi.h"
#include "basic/helper_functions.hpp"

namespace mimium {
class Mididriver {
  std::unique_ptr<RtMidiOut> midiout;
  std::vector<unsigned char> message;

 public:
  Mididriver();
  ~Mididriver();

  void init();
  void setPort(int portnumber);
  void createVirtualPort();
  void sendMessage(std::vector<unsigned char>& m);
  void printCurrentPort(int portnumber=0);
};
}  // namespace mimium
