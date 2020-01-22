; ModuleID = '/Users/tomoya/codes/mimium/test/test_closure3.mmm'
source_filename = "/Users/tomoya/codes/mimium/test/test_closure3.mmm"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

%container_closurefun = type { void (double, %capture0*)* }
%capture0 = type { double* }

@ptr_to_closurefun = constant %container_closurefun { void (double, %capture0*)* @closurefun }

declare i8* @malloc(i64)

; Function Attrs: noinline nounwind optnone
declare void @addTask_cls(double, i8*, double, i8*) #0

; Function Attrs: noinline nounwind optnone
declare void @addTask(double, i8*, double) #0

; Function Attrs: noinline nounwind optnone
define i8* @mimium_main() #0 {
entry:
  %ptr_globalvar_raw = call i8* @malloc(i64 8)
  %ptr_globalvar = bitcast i8* %ptr_globalvar_raw to double*
  %ptr_ptr_closurefun_cap_raw = call i8* @malloc(i64 8)
  %ptr_ptr_closurefun_cap = bitcast i8* %ptr_ptr_closurefun_cap_raw to %capture0*
  %closurefun_cap = load %capture0, %capture0* %ptr_ptr_closurefun_cap
  %0 = insertvalue %capture0 %closurefun_cap, double* %ptr_globalvar, 0
  %ptr_closurefun = load %container_closurefun, %container_closurefun* @ptr_to_closurefun
  store double 2.000000e+00, double* %ptr_globalvar
  %globalvar = load double, double* %ptr_globalvar
  call void @printlndouble(double %globalvar)
  call void @closurefun(double 2.000000e+01, %capture0* %ptr_ptr_closurefun_cap)
  ret i8* null
}

define void @myprint(double %a0) {
entry:
  call void @printlndouble(double %a0)
  ret void
}

declare void @printlndouble(double)

define void @closurefun(double %time1, %capture0* %clsarg_closurefun) {
entry:
  %0 = load %capture0, %capture0* %clsarg_closurefun
  %fv = getelementptr inbounds %capture0, %capture0* %clsarg_closurefun, i32 0, i32 0
  %ptr_fv_globalvar = load double*, double** %fv
  %fv_globalvar = load double, double* %ptr_fv_globalvar
  call void @myprint(double 1.000000e+02)
  call void @myprint(double %time1)
  call void @myprint(double %fv_globalvar)
  ret void
}

attributes #0 = { noinline nounwind optnone }
