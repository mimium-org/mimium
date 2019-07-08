#include "RtMidi.h"
#include <vector>
#include <memory>
namespace mimium{
    class Mididriver{
        std::shared_ptr<RtMidiOut> midiout;
        std::vector<unsigned char> message;
        public:
        Mididriver();
        void setPort(unsigned int portnumber);
        void sendMessage(std::vector<unsigned char>& m);
    };
}

