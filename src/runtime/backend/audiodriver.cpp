#include "audiodriver.hpp"
namespace mimium {

bool AudioDriver::processSample(double* input, double* output) {
  auto shouldstop = sch.incrementTime();
  if (shouldstop) {
    sch.stop();
    return false;
  }
  if (dspfninfos.fn != nullptr) {
    double res = dspfninfos.fn(static_cast<double>(sch.getTime()), dspfninfos.cls_address, dspfninfos.memobj_address);
    for (int ch = 0; ch < this->channels; ch++) { output[ch] = res; }
  }
  return true;
}

}  // namespace mimium