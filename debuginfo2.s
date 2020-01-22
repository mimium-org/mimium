	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 15
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function mimium_main
LCPI0_0:
	.quad	4626322717216342016     ## double 20
LCPI0_1:
	.quad	4611686018427387904     ## double 2
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_mimium_main
	.p2align	4, 0x90
_mimium_main:                           ## @mimium_main
## %bb.0:                               ## %entry
	subq	$24, %rsp
	movl	$8, %edi
	callq	_malloc
	movl	$8, %edi
	movq	%rax, 16(%rsp)          ## 8-byte Spill
	callq	_malloc
	movsd	LCPI0_1(%rip), %xmm0    ## xmm0 = mem[0],zero
	movq	16(%rsp), %rcx          ## 8-byte Reload
	movsd	%xmm0, (%rcx)
	movsd	(%rcx), %xmm0           ## xmm0 = mem[0],zero
	movq	%rax, 8(%rsp)           ## 8-byte Spill
	callq	_printlndouble
	movsd	LCPI0_0(%rip), %xmm0    ## xmm0 = mem[0],zero
	movq	8(%rsp), %rdi           ## 8-byte Reload
	callq	_closurefun
	xorl	%edx, %edx
	movl	%edx, %eax
	addq	$24, %rsp
	retq
                                        ## -- End function
	.globl	_myprint                ## -- Begin function myprint
	.p2align	4, 0x90
_myprint:                               ## @myprint
	.cfi_startproc
## %bb.0:                               ## %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	callq	_printlndouble
	popq	%rax
	retq
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function closurefun
LCPI2_0:
	.quad	4636737291354636288     ## double 100
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_closurefun
	.p2align	4, 0x90
_closurefun:                            ## @closurefun
	.cfi_startproc
## %bb.0:                               ## %entry
	subq	$24, %rsp
	.cfi_def_cfa_offset 32
	movsd	LCPI2_0(%rip), %xmm1    ## xmm1 = mem[0],zero
	movsd	%xmm0, 16(%rsp)         ## 8-byte Spill
	movaps	%xmm1, %xmm0
	movq	%rdi, 8(%rsp)           ## 8-byte Spill
	callq	_myprint
	movsd	16(%rsp), %xmm0         ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	callq	_myprint
	addq	$24, %rsp
	retq
	.cfi_endproc
                                        ## -- End function
	.section	__DATA,__const
	.globl	_ptr_to_closurefun      ## @ptr_to_closurefun
	.p2align	3
_ptr_to_closurefun:
	.quad	_closurefun


.subsections_via_symbols
