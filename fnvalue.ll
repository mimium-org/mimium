; ModuleID = 'build/fnvalue.cpp'
source_filename = "build/fnvalue.cpp"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

%class.anon = type { i32* }

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32* @_Z3muli(i32) #0 {
  %2 = alloca %class.anon, align 8
  %3 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  %4 = getelementptr inbounds %class.anon, %class.anon* %2, i32 0, i32 0
  store i32* %3, i32** %4, align 8
  %5 = getelementptr inbounds %class.anon, %class.anon* %2, i32 0, i32 0
  %6 = load i32*, i32** %5, align 8
  ret i32* %6
}

; Function Attrs: noinline norecurse optnone ssp uwtable
define i32 @main() #1 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca %class.anon, align 8
  %4 = alloca %class.anon, align 8
  store i32 0, i32* %1, align 4
  %5 = call i32* @_Z3muli(i32 1)
  %6 = getelementptr inbounds %class.anon, %class.anon* %3, i32 0, i32 0
  store i32* %5, i32** %6, align 8
  %7 = call i32 @"_ZZ3muliENK3$_0clEi"(%class.anon* %3, i32 4)
  store i32 %7, i32* %2, align 4
  %8 = call i32* @_Z3muli(i32 1)
  %9 = getelementptr inbounds %class.anon, %class.anon* %4, i32 0, i32 0
  store i32* %8, i32** %9, align 8
  ret i32 0
}

; Function Attrs: noinline nounwind optnone ssp uwtable
define internal i32 @"_ZZ3muliENK3$_0clEi"(%class.anon*, i32) #0 align 2 {
  %3 = alloca %class.anon*, align 8
  %4 = alloca i32, align 4
  store %class.anon* %0, %class.anon** %3, align 8
  store i32 %1, i32* %4, align 4
  %5 = load %class.anon*, %class.anon** %3, align 8
  %6 = getelementptr inbounds %class.anon, %class.anon* %5, i32 0, i32 0
  %7 = load i32*, i32** %6, align 8
  %8 = load i32, i32* %7, align 4
  %9 = load i32, i32* %4, align 4
  %10 = mul nsw i32 %8, %9
  ret i32 %10
}

attributes #0 = { noinline nounwind optnone ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline norecurse optnone ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 10, i32 15]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"PIC Level", i32 2}
!3 = !{!"clang version 9.0.0 (tags/RELEASE_900/final)"}
