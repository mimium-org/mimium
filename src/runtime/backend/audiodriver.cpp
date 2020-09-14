#include "audiodriver.hpp"
namespace mimium {

bool AudioDriver::processSample(double* input, double* output) {
  auto shouldstop = sch.incrementTime();
  if (shouldstop) {
    sch.stop();
    return false;
  }
  if (dspfn != nullptr) {
    double res = dspfn(static_cast<double>(sch.getTime()), dspfn_cls_address, dspfn_memobj_address);
    for (int ch = 0; ch < this->channels; ch++) { output[ch] = res; }
  }
  return true;
}

}  // namespace mimium