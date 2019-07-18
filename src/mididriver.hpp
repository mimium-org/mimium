#pragma once
#include <memory>
#include <vector>
#include "RtMidi.h"
#include "helper_functions.hpp"

namespace mimium {
class Mididriver {
  std::unique_ptr<RtMidiOut> midiout;
  std::vector<unsigned char> message;

 public:
  Mididriver();
  ~Mididriver();

  void init();
  void setPort(int portnumber);
  void sendMessage(std::vector<unsigned char>& m);
  void printCurrentPort(int portnumber);
};
}  // namespace mimium
