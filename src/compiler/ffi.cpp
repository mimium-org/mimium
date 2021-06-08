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


MIMIUM_DLL_PUBLIC double mimium_add(double d1, double d2) { return d1 + d2; }
MIMIUM_DLL_PUBLIC double mimium_sub(double d1, double d2) { return d1 - d2; }
MIMIUM_DLL_PUBLIC double mimium_mul(double d1, double d2) { return d1 * d2; }
MIMIUM_DLL_PUBLIC double mimium_div(double d1, double d2) { return d1 / d2; }
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
using Function = IType::Function;

namespace {
const auto init_bi = [](IType::Function&& f, std::string_view&& n) {
  return BuiltinFnInfo{IType::Value{std::move(f)}, n};
};
const auto unit = []() { return IType::Value{IType::Unit{}}; };
const auto float_t = []() { return IType::Value{IType::Float{}}; };
const auto string_t = []() { return IType::Value{IType::String{}}; };

const auto array_t = [](IType::Value&& t, int size) {
  return IType::Value{IType::Array{std::move(t), size}};
};

const auto make_fun = [](IType::Value&& r, List<Box<IType::Value>>&& as) {
  return IType::Function{std::pair(std::move(as), std::move(r))};
};
}  // namespace

const std::unordered_map<std::string_view, BuiltinFnInfo> Intrinsic::ftable = {

    {"print", init_bi(make_fun(unit(), {float_t()}), "printdouble")},
    {"println", init_bi(make_fun(unit(), {float_t()}), "printlndouble")},
    {"printlnstr", init_bi(make_fun(unit(), {string_t()}), "printlnstr")},

    {"sin", init_bi(make_fun(float_t(), {float_t()}), "sin")},
    {"cos", init_bi(make_fun(float_t(), {float_t()}), "cos")},
    {"tan", init_bi(make_fun(float_t(), {float_t()}), "tan")},

    {"asin", init_bi(make_fun(float_t(), {float_t()}), "asin")},
    {"acos", init_bi(make_fun(float_t(), {float_t()}), "acos")},
    {"atan", init_bi(make_fun(float_t(), {float_t()}), "atan")},
    {"atan2", init_bi(make_fun(float_t(), {float_t(), float_t()}), "atan2")},

    {"sinh", init_bi(make_fun(float_t(), {float_t()}), "sinh")},
    {"cosh", init_bi(make_fun(float_t(), {float_t()}), "cosh")},
    {"tanh", init_bi(make_fun(float_t(), {float_t()}), "tanh")},
    {"exp", init_bi(make_fun(float_t(), {float_t()}), "exp")},
    {"pow", init_bi(make_fun(float_t(), {float_t(), float_t()}), "pow")},

    {"log", init_bi(make_fun(float_t(), {float_t()}), "log")},
    {"log10", init_bi(make_fun(float_t(), {float_t()}), "log10")},
    {"random", init_bi(make_fun(float_t(), {}), "mimiumrand")},

    {"sqrt", init_bi(make_fun(float_t(), {float_t()}), "sqrt")},
    {"abs", init_bi(make_fun(float_t(), {float_t()}), "fabs")},

    {"ceil", init_bi(make_fun(float_t(), {float_t()}), "ceil")},
    {"floor", init_bi(make_fun(float_t(), {float_t()}), "floor")},
    {"trunc", init_bi(make_fun(float_t(), {float_t()}), "trunc")},
    {"round", init_bi(make_fun(float_t(), {float_t()}), "round")},

    {"fmod", init_bi(make_fun(float_t(), {float_t(), float_t()}), "fmod")},
    {"remainder", init_bi(make_fun(float_t(), {float_t(), float_t()}), "remainder")},

    {"min", init_bi(make_fun(float_t(), {float_t(), float_t()}), "fmin")},
    {"max", init_bi(make_fun(float_t(), {float_t(), float_t()}), "fmax")},

    // These primitive operations are called from C library difined above if no intrinsic are find in code generator for each backends.
    // If they can, they will be replaced with primitive operations.

    {"add",init_bi(make_fun(float_t(), {float_t()}), "mimium_add") },
    {"sub",init_bi(make_fun(float_t(), {float_t()}), "mimium_sub") },
    {"mul",init_bi(make_fun(float_t(), {float_t()}), "mimium_mul") },
    {"div",init_bi(make_fun(float_t(), {float_t()}), "mimium_div") },

    {"ge", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_ge")},
    {"eq", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_eq")},
    {"noteq", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_noteq")},
    {"le", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_le")},
    {"gt", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_gt")},
    {"lt", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_lt")},
    {"and", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_and")},
    {"or", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_or")},
    {"not", init_bi(make_fun(float_t(), {float_t()}), "mimium_not")},

    {"lshift", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_lshift")},
    {"rshift", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_rshift")},

    {"mem", init_bi(make_fun(float_t(), {float_t()}), "mimium_memprim")},
    {"delay", init_bi(make_fun(float_t(), {float_t(), float_t()}), "mimium_delayprim")},

    {"loadwavsize", init_bi(make_fun(float_t(), {string_t()}), "libsndfile_loadwavsize")},
    {"loadwav", init_bi(make_fun(array_t(float_t(), 0), {string_t()}), "libsndfile_loadwav")},

    {"access_array_lin_interp",
     init_bi(make_fun(float_t(), {float_t(), float_t()}), "access_array_lin_interp")}

};

}  // namespace mimium