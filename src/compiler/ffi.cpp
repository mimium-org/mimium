/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/ffi.hpp"
#include <cmath>
#include <iostream>
#include "sndfile.h"

extern "C" {
MIMIUM_DLL_PUBLIC void dumpaddress(void* a) { std::cerr << a << "\n"; }

MIMIUM_DLL_PUBLIC void printdouble(double d) { std::cout << d; }
MIMIUM_DLL_PUBLIC void printlndouble(double d) { std::cout << d << "\n"; }

MIMIUM_DLL_PUBLIC void printlnstr(char* str) { std::cout << str << "\n"; }

MIMIUM_DLL_PUBLIC double mimiumrand() { return ((double)rand() / RAND_MAX) * 2 - 1; }

MIMIUM_DLL_PUBLIC bool mimium_dtob(double d) { return d > 0; }
MIMIUM_DLL_PUBLIC int64_t mimium_dtoi(double d) { return static_cast<int64_t>(d); }
MIMIUM_DLL_PUBLIC double mimium_gt(double d1, double d2) { return static_cast<double>(d1 > d2); }
MIMIUM_DLL_PUBLIC double mimium_lt(double d1, double d2) { return static_cast<double>(d1 < d2); }
MIMIUM_DLL_PUBLIC double mimium_ge(double d1, double d2) { return static_cast<double>(d1 >= d2); }
MIMIUM_DLL_PUBLIC double mimium_eq(double d1, double d2) { return static_cast<double>(d1 == d2); }
MIMIUM_DLL_PUBLIC double mimium_noteq(double d1, double d2) {
  return static_cast<double>(d1 != d2);
}
MIMIUM_DLL_PUBLIC double mimium_le(double d1, double d2) { return static_cast<double>(d1 <= d2); }
MIMIUM_DLL_PUBLIC double mimium_and(double d1, double d2) {
  return static_cast<double>(mimium_dtob(d1) && mimium_dtob(d2));
}
MIMIUM_DLL_PUBLIC double mimium_or(double d1, double d2) {
  return static_cast<double>(mimium_dtob(d1) || mimium_dtob(d2));
}
MIMIUM_DLL_PUBLIC double mimium_not(double d1) { return static_cast<double>(!mimium_dtob(d1)); }

MIMIUM_DLL_PUBLIC double mimium_lshift(double d1, double d2) {
  return static_cast<double>(mimium_dtoi(d1) << mimium_dtoi(d2));
}
MIMIUM_DLL_PUBLIC double mimium_rshift(double d1, double d2) {
  return static_cast<double>(mimium_dtoi(d1) >> mimium_dtoi(d2));
}

MIMIUM_DLL_PUBLIC double access_array_lin_interp(double* array, double index_d) {
  double fract = fmod(index_d, 1.000);
  size_t index = floor(index_d);
  if (fract == 0) { return array[index]; }
  return array[index] * (1 - fract) + array[index + 1] * fract;
}
struct MmmRingBuf {
  // int64_t size=5000;
  int64_t readi = 0;
  int64_t writei = 0;
  double buf[mimium::Intrinsic::fixed_delaysize]{};
};

MIMIUM_DLL_PUBLIC double mimium_memprim(double in, double* valptr) {
  auto res = *valptr;
  *valptr = in;
  return res;
}
MIMIUM_DLL_PUBLIC double mimium_delayprim(double in, double time, MmmRingBuf* rbuf) {
  auto size = sizeof(rbuf->buf) / sizeof(double);
  rbuf->writei = (rbuf->writei + 1) % size;
  double readi = fmod((size + rbuf->writei - time), size);
  rbuf->readi = (int64_t)readi;
  rbuf->buf[rbuf->writei] = in;
  return access_array_lin_interp(rbuf->buf, readi);
}

MIMIUM_DLL_PUBLIC double libsndfile_loadwavsize(char* filename) {
  SF_INFO sfinfo;
  auto* sfile = sf_open(filename, SFM_READ, &sfinfo);
  if (sfile == nullptr) { std::cerr << sf_strerror(sfile) << "\n"; }
  auto res = sfinfo.frames;
  sf_close(sfile);
  return res;
}

MIMIUM_DLL_PUBLIC double* libsndfile_loadwav(char* filename) {
  SF_INFO sfinfo;
  auto* sfile = sf_open(filename, SFM_READ, &sfinfo);
  if (sfile == nullptr) { std::cerr << sf_strerror(sfile) << "\n"; }

  const int bufsize = sfinfo.frames * sfinfo.channels;
  double* buffer = new double[bufsize];
  sf_readf_double(sfile, buffer, bufsize);
  // sf_close(sfile);
  // std::cerr<< filename << "(" << size << ") is succecfully loaded";
  return buffer;
}
}

namespace mimium {
using FI = BuiltinFnInfo;
using Function = HType::Function;

namespace {
auto initBI = [](HType::Function&& f, std::string_view&& n) {
  return BuiltinFnInfo{HType::Value{std::move(f)}, n};
};
auto Void = []() { return HType::Value{HType::Unit{}}; };
auto Float = []() { return HType::Value{HType::Float{}}; };
auto String = []() { return HType::Value{HType::String{}}; };

auto Array = [](HType::Value&& t, int size) {
  return HType::Value{HType::Array{std::move(t), size}};
};

auto makeFun = [](HType::Value&& r, List<Box<HType::Value>>&& as) {
  return HType::Function{std::pair(std::move(as), std::move(r))};
};

auto&& makeFunFtoF = makeFun(Float(), {Float()});

}  // namespace

const std::unordered_map<std::string_view, BuiltinFnInfo> Intrinsic::ftable = {
    {"print", initBI(makeFun(Void(), {Float()}), "printdouble")},
    {"println", initBI(makeFun(Void(), {Float()}), "printlndouble")},
    {"printlnstr", initBI(makeFun(Void(), {String()}), "printlnstr")},

    {"sin", initBI(makeFun(Float(), {Float()}), "sin")},
    {"cos", initBI(makeFun(Float(), {Float()}), "cos")},
    {"tan", initBI(makeFun(Float(), {Float()}), "tan")},

    {"asin", initBI(makeFun(Float(), {Float()}), "asin")},
    {"acos", initBI(makeFun(Float(), {Float()}), "acos")},
    {"atan", initBI(makeFun(Float(), {Float()}), "atan")},
    {"atan2", initBI(makeFun(Float(), {Float(), Float()}), "atan2")},

    {"sinh", initBI(makeFun(Float(), {Float()}), "sinh")},
    {"cosh", initBI(makeFun(Float(), {Float()}), "cosh")},
    {"tanh", initBI(makeFun(Float(), {Float()}), "tanh")},
    {"exp", initBI(makeFun(Float(), {Float()}), "exp")},
    {"pow", initBI(makeFun(Float(), {Float(), Float()}), "pow")},

    {"log", initBI(makeFun(Float(), {Float()}), "log")},
    {"log10", initBI(makeFun(Float(), {Float()}), "log10")},
    {"random", initBI(makeFun(Float(), {}), "mimiumrand")},

    {"sqrt", initBI(makeFun(Float(), {Float()}), "sqrt")},
    {"abs", initBI(makeFun(Float(), {Float()}), "fabs")},

    {"ceil", initBI(makeFun(Float(), {Float()}), "ceil")},
    {"floor", initBI(makeFun(Float(), {Float()}), "floor")},
    {"trunc", initBI(makeFun(Float(), {Float()}), "trunc")},
    {"round", initBI(makeFun(Float(), {Float()}), "round")},

    {"fmod", initBI(makeFun(Float(), {Float(), Float()}), "fmod")},
    {"remainder", initBI(makeFun(Float(), {Float(), Float()}), "remainder")},

    {"min", initBI(makeFun(Float(), {Float(), Float()}), "fmin")},
    {"max", initBI(makeFun(Float(), {Float(), Float()}), "fmax")},

    {"ge", initBI(makeFun(Float(), {Float(), Float()}), "mimium_ge")},
    {"eq", initBI(makeFun(Float(), {Float(), Float()}), "mimium_eq")},
    {"noteq", initBI(makeFun(Float(), {Float(), Float()}), "mimium_noteq")},
    {"le", initBI(makeFun(Float(), {Float(), Float()}), "mimium_le")},
    {"gt", initBI(makeFun(Float(), {Float(), Float()}), "mimium_gt")},
    {"lt", initBI(makeFun(Float(), {Float(), Float()}), "mimium_lt")},
    {"and", initBI(makeFun(Float(), {Float(), Float()}), "mimium_and")},
    {"or", initBI(makeFun(Float(), {Float(), Float()}), "mimium_or")},
    {"not", initBI(makeFun(Float(), {Float()}), "mimium_not")},

    {"lshift", initBI(makeFun(Float(), {Float(), Float()}), "mimium_lshift")},
    {"rshift", initBI(makeFun(Float(), {Float(), Float()}), "mimium_rshift")},

    {"mem", initBI(makeFun(Float(), {Float()}), "mimium_memprim")},
    {"delay", initBI(makeFun(Float(), {Float(), Float()}), "mimium_delayprim")},

    {"loadwavsize", initBI(makeFun(Float(), {String()}), "libsndfile_loadwavsize")},
    {"loadwav", initBI(makeFun(Array(Float(), 0), {String()}), "libsndfile_loadwav")},

    {"access_array_lin_interp",
     initBI(makeFun(Float(), {Float(), Float()}), "access_array_lin_interp")}

};

}  // namespace mimium