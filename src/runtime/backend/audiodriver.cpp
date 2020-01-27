#include "runtime/backend/audiodriver.hpp"

namespace mimium {

AudioDriver::AudioDriver(Scheduler& sch, unsigned int sr, unsigned int bs,
                       unsigned int chs)
      : sample_rate(sr), buffer_size(bs), channels(chs), sch(sch) {
  }
}  // namespace mimium