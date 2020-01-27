#include "runtime/backend/audiodriver.hpp"

namespace mimium {

AudioDriver::AudioDriver(Scheduler& sch, unsigned int sr, unsigned int bs,
                       unsigned int chs)
      : sample_rate(sr), buffer_size(bs), channels(chs), sch(sch) {
    dspfn = sch.getRuntime().getDspFn();
    dspfn_cls_address = sch.getRuntime().getDspFnCls();
  }
}  // namespace mimium