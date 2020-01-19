; ModuleID = '/Users/tomoya/codes/mimium/test/dsptest_closure.mmm'
source_filename = "/Users/tomoya/codes/mimium/test/dsptest_closure.mmm"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

%fvtype.0 = type { double*, double* }
%fvtype.1 = type { double*, double*, double*, double* }
%Time_of_Float = type { double, double }

declare i8* @malloc(i64 %0)

; Function Attrs: noinline nounwind optnone
declare void @addTask_cls(double %0, i8* %1, double %2, i8* %3) #0

; Function Attrs: noinline nounwind optnone
declare void @addTask(double %0, i8* %1, double %2) #0

; Function Attrs: noinline nounwind optnone
define i8* @mimium_main() #0 {
entry:
  %ptr_SR_raw = call i8* @malloc(i64 8)
  %ptr_SR = bitcast i8* %ptr_SR_raw to double*
  store double 4.800000e+04, double* %ptr_SR
  %SR = load double, double* %ptr_SR
  %ptr_freq_raw = call i8* @malloc(i64 8)
  %ptr_freq = bitcast i8* %ptr_freq_raw to double*
  store double 1.000000e+03, double* %ptr_freq
  %freq = load double, double* %ptr_freq
  %ptr_phase_raw = call i8* @malloc(i64 8)
  %ptr_phase = bitcast i8* %ptr_phase_raw to double*
  store double 0.000000e+00, double* %ptr_phase
  %phase = load double, double* %ptr_phase
  %ptr_PI_raw = call i8* @malloc(i64 8)
  %ptr_PI = bitcast i8* %ptr_PI_raw to double*
  store double 0x400921FCDFBA0F5B, double* %ptr_PI
  %PI = load double, double* %ptr_PI
  %ptr_changeFreq_cls_raw = call i8* @malloc(i64 16)
  %ptr_changeFreq_cls = bitcast i8* %ptr_changeFreq_cls_raw to %fvtype.0*
  %0 = getelementptr inbounds %fvtype.0, %fvtype.0* %ptr_changeFreq_cls, i32 0, i32 0
  store double* %ptr_freq, double** %0
  %1 = getelementptr inbounds %fvtype.0, %fvtype.0* %ptr_changeFreq_cls, i32 0, i32 1
  store double* %ptr_SR, double** %1
  %ptr_dsp_cls_raw = call i8* @malloc(i64 32)
  %ptr_dsp_cls = bitcast i8* %ptr_dsp_cls_raw to %fvtype.1*
  %2 = getelementptr inbounds %fvtype.1, %fvtype.1* %ptr_dsp_cls, i32 0, i32 0
  store double* %ptr_PI, double** %2
  %3 = getelementptr inbounds %fvtype.1, %fvtype.1* %ptr_dsp_cls, i32 0, i32 1
  store double* %ptr_freq, double** %3
  %4 = getelementptr inbounds %fvtype.1, %fvtype.1* %ptr_dsp_cls, i32 0, i32 2
  store double* %ptr_SR, double** %4
  %5 = getelementptr inbounds %fvtype.1, %fvtype.1* %ptr_dsp_cls, i32 0, i32 3
  store double* %ptr_phase, double** %5
  %6 = bitcast %fvtype.0* %ptr_changeFreq_cls to i8*
  call void @addTask_cls(double 0.000000e+00, i8* bitcast (void (double, %fvtype.0*)* @changeFreq to i8*), double 0.000000e+00, i8* %6)
  ret %fvtype.1* %ptr_dsp_cls
}

define void @changeFreq(double %time0, %fvtype.0* %clsarg_changeFreq) {
entry:
  %fv = getelementptr inbounds %fvtype.0, %fvtype.0* %clsarg_changeFreq, i32 0, i32 0
  %ptr_fv_freq_changeFreq = load double*, double** %fv
  %fv_freq_changeFreq = load double, double* %ptr_fv_freq_changeFreq
  %fv1 = getelementptr inbounds %fvtype.0, %fvtype.0* %clsarg_changeFreq, i32 0, i32 1
  %ptr_fv_SR_changeFreq = load double*, double** %fv1
  %fv_SR_changeFreq = load double, double* %ptr_fv_SR_changeFreq
  %k2 = fadd double %fv_freq_changeFreq, 1.258700e+04
  %k1 = call double @fmod(double %k2, double 4.500000e+03)
  store double %k1, double* %ptr_fv_freq_changeFreq
  %fv_freq_changeFreq2 = load double, double* %ptr_fv_freq_changeFreq
  %ptr_nt1 = alloca double
  %nt1 = fadd double %time0, %fv_SR_changeFreq
  %0 = insertvalue %Time_of_Float zeroinitializer, double %nt1, 0
  %1 = insertvalue %Time_of_Float %0, double %nt1, 1
  %2 = extractvalue %Time_of_Float %1, 0
  %3 = extractvalue %Time_of_Float %1, 1
  %4 = bitcast %fvtype.0* %clsarg_changeFreq to i8*
  call void @addTask_cls(double %2, i8* bitcast (void (double, %fvtype.0*)* @changeFreq to i8*), double %3, i8* %4)
  ret void
}

declare double @fmod(double %0, double %1)

define double @dsp(double %time2, %fvtype.1* %clsarg_dsp) {
entry:
  %fv = getelementptr inbounds %fvtype.1, %fvtype.1* %clsarg_dsp, i32 0, i32 0
  %ptr_fv_PI_dsp = load double*, double** %fv
  %fv_PI_dsp = load double, double* %ptr_fv_PI_dsp
  %fv1 = getelementptr inbounds %fvtype.1, %fvtype.1* %clsarg_dsp, i32 0, i32 1
  %ptr_fv_freq_dsp = load double*, double** %fv1
  %fv_freq_dsp = load double, double* %ptr_fv_freq_dsp
  %fv2 = getelementptr inbounds %fvtype.1, %fvtype.1* %clsarg_dsp, i32 0, i32 2
  %ptr_fv_SR_dsp = load double*, double** %fv2
  %fv_SR_dsp = load double, double* %ptr_fv_SR_dsp
  %fv3 = getelementptr inbounds %fvtype.1, %fvtype.1* %clsarg_dsp, i32 0, i32 3
  %ptr_fv_phase_dsp = load double*, double** %fv3
  %fv_phase_dsp = load double, double* %ptr_fv_phase_dsp
  %k10 = fmul double 2.000000e+00, %fv_PI_dsp
  %k9 = fmul double %k10, %fv_freq_dsp
  %k8 = fdiv double %k9, %fv_SR_dsp
  %k7 = fadd double %fv_phase_dsp, %k8
  store double %k7, double* %ptr_fv_phase_dsp
  %fv_phase_dsp4 = load double, double* %ptr_fv_phase_dsp
  %k12 = call double @sin(double %fv_phase_dsp4)
  ret double %k12
}

declare double @sin(double %0)

attributes #0 = { noinline nounwind optnone }
