#include <memory>
#include <vector>
#include "RtMidi.h"
namespace mimium {
class Mididriver {
  std::shared_ptr<RtMidiOut> midiout;
  std::vector<unsigned char> message;

 public:
  Mididriver();
  void setPort(int portnumber);
  void sendMessage(std::vector<unsigned char>& m);
  void printCurrentPort(int portnumber);
};
}  // namespace mimium
