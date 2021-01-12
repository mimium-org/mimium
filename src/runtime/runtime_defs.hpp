/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
namespace mimium {

// outputresult,input, clsaddress,memobjaddress
using DspFnPtr = void (*)(double*, const double*, void*, void*);

// Information set by definition of dsp function.
// number of in&out channels are determined by type of dsp function.
struct DspFnInfos {
  DspFnPtr fn = nullptr;
  void* cls_address = nullptr;
  void* memobj_address = nullptr;
  int in_numchs = 0;
  int out_numchs = 0;
};

// Information of AudioDriver(e.g. Hardware Device).
// number of in&out channels are determined by logical number of device and may be different from
// DspFnInfos.
struct AudioDriverParams {
  double samplerate = 0;
  int buffersize = 0;
  int in_numchs = 0;
  int out_numchs = 0;
};

}  // namespace mimium