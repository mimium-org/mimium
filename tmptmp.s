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
	pushq	%r14
	pushq	%rbx
	pushq	%rax
	movl	$8, %edi
	callq	_malloc
	movq	%rax, %rbx
	movl	$8, %edi
	callq	_malloc
	movq	%rax, %r14
	movsd	LCPI0_1(%rip), %xmm0    ## xmm0 = mem[0],zero
	movsd	%xmm0, (%rbx)
	movsd	(%rbx), %xmm0           ## xmm0 = mem[0],zero
	callq	_printlndouble
	movsd	LCPI0_0(%rip), %xmm0    ## xmm0 = mem[0],zero
	movq	%r14, %rdi
	callq	_closurefun
	xorl	%eax, %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%r14
	retq
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function closurefun
LCPI1_0:
	.quad	4636737291354636288     ## double 100
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_closurefun
	.p2align	4, 0x90
_closurefun:                            ## @closurefun
	.cfi_startproc
## %bb.0:                               ## %entry
	subq	$24, %rsp
	.cfi_def_cfa_offset 32
	movsd	%xmm0, 8(%rsp)          ## 8-byte Spill
	movq	(%rdi), %rax
	movsd	(%rax), %xmm0           ## xmm0 = mem[0],zero
	movsd	%xmm0, 16(%rsp)         ## 8-byte Spill
	movsd	LCPI1_0(%rip), %xmm0    ## xmm0 = mem[0],zero
	callq	_printlndouble
	movsd	8(%rsp), %xmm0          ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	callq	_printlndouble
	movsd	16(%rsp), %xmm0         ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	callq	_printlndouble
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
