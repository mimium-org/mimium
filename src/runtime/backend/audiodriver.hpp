/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <memory>
#include "runtime/runtime.hpp"

namespace mimium {

class MIMIUM_DLL_PUBLIC AudioDriver {
 protected:
  std::unique_ptr<AudioDriverParams> params;
  std::unique_ptr<DspFnInfos> dspfninfos;
  Scheduler sch;

 public:
  AudioDriver()
      : params(nullptr),
        sch(){

        };
  virtual ~AudioDriver() = default;
  Scheduler& getScheduler() { return sch; }
  void setDspFnInfos(std::unique_ptr<DspFnInfos> p) {
    dspfninfos = std::move(p);
    Logger::debug_log("dsp function:" + std::to_string(dspfninfos->in_numchs) + " input, " +
                          std::to_string(dspfninfos->out_numchs) + " output",
                      Logger::INFO);
  }
  virtual void setup(std::unique_ptr<AudioDriverParams> p) {
    params = std::move(p);
    interleaved_in.resize(params->audioframesize * dspfninfos->in_numchs);
    interleaved_out.resize(params->audioframesize * dspfninfos->out_numchs);
    if (dspfninfos->in_numchs > params->in_numchs || dspfninfos->out_numchs > params->out_numchs) {
      Logger::debug_log(
          "Number of inputs/outputs is bigger than number of the audio driver's inputs/outputs.",
          Logger::WARNING);
    }
  }

  virtual bool start() {
    assert(params != nullptr);
    assert(dspfninfos != nullptr);
    return true;
  }
  virtual bool stop() = 0;
  [[nodiscard]] virtual std::unique_ptr<AudioDriverParams> getDefaultAudioParameter(
      std::optional<int> samplerate, std::optional<int> framesize) const = 0;
  // Main dsp process function
  bool process(const double** input, double** output, int framesize) {
    if (dspfninfos->fn != nullptr) { return processInternal<true>(input, output, framesize); }
    return processInternal<false>(input, output, framesize);
  }
  // Interleaved version of main dsp process.
  bool process(const double* input, double* output, int framesize) {
    if (dspfninfos->fn != nullptr) {
      return processInternalInterleaved<true>(input, output, framesize);
    }
    return processInternalInterleaved<false>(input, output, framesize);
  }

 protected:
  inline static constexpr int default_framesize = 256;

 private:
  std::vector<double> interleaved_in;
  std::vector<double> interleaved_out;
  // buffer copy into vector from pointer of poitner
  static void interleaveSamples(const double** src, std::vector<double>& dest, int framesize,
                                int dsp_chans, int device_chans) {
    for (int ch = 0; ch < dsp_chans; ch++) {
      const auto* samples_ch = *std::next(src, ch);
      if (ch < device_chans) {
        for (int count = 0; count < framesize; count++) {
          dest.at(count + count * ch) = *std::next(samples_ch, count);
        }
      } else {
        for (int count = 0; count < framesize; count++) { dest.at(count + count * ch) = 0; }
      }
    }
  }
  // buffer copy into pointer of poitner from vector
  static void deinterleaveSamples(std::vector<double> const& src, double** dest, int framesize,
                                  int dsp_chans, int device_chans) {
    for (int ch = 0; ch < dsp_chans; ch++) {
      auto* dest_ch = *std::next(dest, ch);
      if (ch < device_chans) {
        for (int count = 0; count < framesize; count++) {
          *std::next(dest_ch, count) = src.at(count + count * ch);
        }
      } else {
        std::fill(dest_ch, std::next(dest_ch, framesize), 0);
      }
    }
  }

  template <bool HASDSP>
  bool processInternal(const double** input, double** output, int framesize) {
    assert(framesize == params->audioframesize);
    bool res = true;
    interleaveSamples(input, interleaved_in, framesize, dspfninfos->in_numchs, params->in_numchs);
    for (int count = 0; count < framesize; count++) {
      res &= this->processSample<HASDSP>(interleaved_in.data(), interleaved_out.data());
    }
    deinterleaveSamples(interleaved_out, output, framesize, dspfninfos->out_numchs,
                        params->out_numchs);
    return res;
  }
  template <bool HASDSP>
  bool processSample(const double* input, double* output) {
    auto shouldstop = sch.incrementTime();
    if (shouldstop) {
      sch.stop();
      return false;
    }
    if constexpr (HASDSP) {
      dspfninfos->fn(output, input, dspfninfos->cls_address, dspfninfos->memobj_address);
    }
    return true;
  }

  template <bool HASDSP>
  bool processInternalInterleaved(const double* input, double* output, int framesize) {
    if constexpr (HASDSP) {
      int dsp_ins = dspfninfos->in_numchs;
      int device_ins = params->in_numchs;
      int dsp_outs = dspfninfos->out_numchs;
      int device_outs = params->out_numchs;
      for (int ch = 0; ch < dsp_ins; ch++) {
        for (int count = 0; count < framesize; count++) {
          if (ch > device_ins) {
            interleaved_in[ch + dsp_ins * count] = input[ch + device_ins * count];
          } else {
            interleaved_in[ch + dsp_ins * count] = 0;
          }
        }
      }
      bool res = true;
      for (int count = 0; count < framesize; count++) {
        res &= processSample<true>(std::next(interleaved_in.data(), count * dsp_ins),
                                   std::next(interleaved_out.data(), count * dsp_outs));
      }
      for (int ch = 0; ch < dsp_outs; ch++) {
        for (int count = 0; count < framesize; count++) {
          if (ch < device_outs) {
            output[ch + device_outs * count] = interleaved_out[ch + dsp_outs * count];
          } else {
            output[ch + device_outs * count] = 0;
          }
        }
      }
      return res;
    } else {
      return processSample<false>(input, output);
    }
  }
};
}  // namespace mimium