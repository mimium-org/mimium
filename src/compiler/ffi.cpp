/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/ffi.hpp"


extern "C" {
void dumpaddress(void* a) { std::cerr << a << "\n"; }

void printdouble(double d) { std::cout << d; }
void printlndouble(double d) { std::cout << d << "\n"; }

void printlnstr(char* str) { std::cout << str << "\n"; }

double mimiumrand() { return ((double)rand() / RAND_MAX) * 2 - 1; }

bool mimium_dtob(double d) { return d > 0; }
int64_t mimium_dtoi(double d) { return static_cast<int64_t>(d); }
double mimium_gt(double d1, double d2) { return static_cast<double>(d1 > d2); }
double mimium_lt(double d1, double d2) { return static_cast<double>(d1 < d2); }
double mimium_ge(double d1, double d2) { return static_cast<double>(d1 >= d2); }
double mimium_le(double d1, double d2) { return static_cast<double>(d1 <= d2); }
double mimium_and(double d1, double d2) {
  return static_cast<double>(mimium_dtob(d1) && mimium_dtob(d2));
}
double mimium_or(double d1, double d2) {
  return static_cast<double>(mimium_dtob(d1) || mimium_dtob(d2));
}
double mimium_not(double d1) { return static_cast<double>(!mimium_dtob(d1)); }

double mimium_lshift(double d1, double d2) {
  return static_cast<double>(mimium_dtoi(d1) << mimium_dtoi(d2));
}
double mimium_rshift(double d1, double d2) {
  return static_cast<double>(mimium_dtoi(d1) >> mimium_dtoi(d2));
}




double access_array_lin_interp(double* array, double index_d) {
  double fract = fmod(index_d, 1.000);
  size_t index = floor(index_d);
  return array[index] * fract + array[index + 1] * (1 - fract);
}
struct MmmRingBuf{
  // int64_t size=5000;
  int64_t readi=0;
  int64_t writei=0;
  double buf[mimium::types::fixed_delaysize];
};
double mimium_delayprim(double in, double time,MmmRingBuf* rbuf) {
  auto size = sizeof(rbuf->buf)/sizeof(double);
  rbuf->writei = (rbuf->writei+1)%size;
  double readi = fmod((size+rbuf->writei-time),size);
  rbuf->readi = (int64_t)readi;
  rbuf->buf[rbuf->writei]=in;
  return access_array_lin_interp(rbuf->buf,readi);
}


double libsndfile_loadwavsize(char* filename) {
  SF_INFO sfinfo;
  auto* sfile = sf_open(filename, SFM_READ, &sfinfo);
  if (sfile == nullptr) { std::cerr << sf_strerror(sfile) << "\n"; }
  auto res = sfinfo.frames;
  sf_close(sfile);
  return res;
}

double* libsndfile_loadwav(char* filename) {
  SF_INFO sfinfo;
  auto sfile = sf_open(filename, SFM_READ, &sfinfo);
  if (sfile == nullptr) { std::cerr << sf_strerror(sfile) << "\n"; }

  const int bufsize = sfinfo.frames * sfinfo.channels;
  double* buffer =  new double[bufsize];
  auto size = sf_readf_double(sfile, buffer, bufsize);
  // sf_close(sfile);
  // std::cerr<< filename << "(" << size << ") is succecfully loaded";
  return buffer;
}
}

namespace mimium {
using namespace types;
using FI = BuiltinFnInfo;
std::unordered_map<std::string, BuiltinFnInfo> LLVMBuiltin::ftable = {
    {"print", initBI(Function{Void{}, {Float{}}}, "printdouble")},
    {"println", initBI(Function{Void{}, {Float{}}}, "printlndouble")},
    {"printlnstr", initBI(Function{Void{}, {String{}}}, "printlnstr")},

    {"sin", initBI(Function{Float{}, {Float{}}}, "sin")},
    {"cos", initBI(Function{Float{}, {Float{}}}, "cos")},
    {"tan", initBI(Function{Float{}, {Float{}}}, "tan")},

    {"asin", initBI(Function{Float{}, {Float{}}}, "asin")},
    {"acos", initBI(Function{Float{}, {Float{}}}, "acos")},
    {"atan", initBI(Function{Float{}, {Float{}}}, "atan")},
    {"atan2", initBI(Function{Float{}, {Float{}, Float{}}}, "atan2")},

    {"sinh", initBI(Function{Float{}, {Float{}}}, "sinh")},
    {"cosh", initBI(Function{Float{}, {Float{}}}, "cosh")},
    {"tanh", initBI(Function{Float{}, {Float{}}}, "tanh")},
    {"exp", initBI(Function{Float{}, {Float{}}}, "exp")},
    {"pow", initBI(Function{Float{}, {Float{}, Float{}}}, "pow")},

    {"log", initBI(Function{Float{}, {Float{}}}, "log")},
    {"log10", initBI(Function{Float{}, {Float{}}}, "log10")},
    {"random", initBI(Function{Float{}, {}}, "mimiumrand")},

    {"sqrt", initBI(Function{Float{}, {Float{}}}, "sqrt")},
    {"abs", initBI(Function{Float{}, {Float{}}}, "fabs")},

    {"ceil", initBI(Function{Float{}, {Float{}}}, "ceil")},
    {"floor", initBI(Function{Float{}, {Float{}}}, "floor")},
    {"trunc", initBI(Function{Float{}, {Float{}}}, "trunc")},
    {"round", initBI(Function{Float{}, {Float{}}}, "round")},

    {"fmod", initBI(Function{Float{}, {Float{}, Float{}}}, "fmod")},
    {"remainder", initBI(Function{Float{}, {Float{}, Float{}}}, "remainder")},

    {"min", initBI(Function{Float{}, {Float{}, Float{}}}, "fmin")},
    {"max", initBI(Function{Float{}, {Float{}, Float{}}}, "fmax")},

    {"ge", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_ge")},
    {"le", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_le")},
    {"gt", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_gt")},
    {"lt", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_lt")},
    {"and", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_and")},
    {"or", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_or")},
    {"not", initBI(Function{Float{}, {Float{}}}, "mimium_not")},

    {"lshift", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_lshift")},
    {"rshift", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_rshift")},

    {"mem", initBI(Function{Float{}, {Float{}}}, "mimium_memprim")},
    {"delay", initBI(Function{Float{}, {Float{}, Float{}}}, "mimium_delayprim")},

    {"loadwavsize", initBI(Function{Float{}, {String{}}}, "libsndfile_loadwavsize")},
    {"loadwav", initBI(Function{Array{Float{}}, {String{}}}, "libsndfile_loadwav")},


    {"access_array_lin_interp",
     initBI(Function{Float{}, {Float{}, Float{}}}, "access_array_lin_interp")}

};

}  // namespace mimium