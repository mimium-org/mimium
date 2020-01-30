/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "runtime/mididriver.hpp"

namespace mimium {
Mididriver::Mididriver() { message.resize(6); }
Mididriver::~Mididriver() {
  if (midiout->isPortOpen()) {
    midiout->closePort();
  }
}
void Mididriver::init() {
  midiout = std::make_unique<RtMidiOut>(RtMidi::Api::UNSPECIFIED,
                                        "mimium-interpreter");
}
void Mididriver::setPort(int portnumber) {
  if (midiout->isPortOpen()) {
    midiout->closePort();
  }
  if (midiout->getPortCount() <= 0) {
    std::cerr << "No Availlable Midi Port" << std::endl;
  } else if (portnumber > midiout->getPortCount()) {
    std::cerr << "Port number out of range" << std::endl;
  } else {
    midiout->openPort(portnumber);
  }
}
void Mididriver::createVirtualPort() {
  midiout->openVirtualPort("mimium-midi");
}
void Mididriver::sendMessage(std::vector<unsigned char>& m) {
  midiout->sendMessage(&m);
}
void Mididriver::printCurrentPort(int portnumber) {
  Logger::debug_log("Current MIDI Port: " + midiout->getPortName(portnumber),
                    Logger::INFO);
}
}  // namespace mimium