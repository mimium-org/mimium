#include "mididriver.hpp"

namespace mimium{
    Mididriver::Mididriver(){
        midiout = std::make_shared<RtMidiOut>();
        message.resize(6);
    }
    void Mididriver::setPort(unsigned int portnumber){
        midiout->openPort(portnumber);
    }
    void  Mididriver::sendMessage(std::vector<unsigned char>& m){
        midiout->sendMessage(&m)  ;
    }
}