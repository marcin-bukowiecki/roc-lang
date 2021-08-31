	.text
	.def	 @feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"Test1"
	.def	 Int32.typeId.0;
	.scl	2;
	.type	32;
	.endef
	.globl	Int32.typeId.0
	.p2align	4, 0x90
Int32.typeId.0:
	movl	$3, %eax
	retq

	.def	 Int32.toString.0;
	.scl	2;
	.type	32;
	.endef
	.globl	Int32.toString.0
	.p2align	4, 0x90
Int32.toString.0:
.seh_proc Int32.toString.0
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$32, %rsp
	.seh_stackalloc 32
	.seh_endprologue
	movq	%rcx, %rsi
	movl	$12, %ecx
	callq	malloc
	movq	%rax, %rdi
	movl	$32, %ecx
	callq	malloc
	movq	%rax, %rbx
	movl	8(%rsi), %r8d
	leaq	.L__unnamed_1(%rip), %rdx
	movq	%rdi, %rcx
	callq	myIntToString
	movq	%rdi, 16(%rbx)
	movl	%eax, 24(%rbx)
	movq	%rbx, %rax
	addq	$32, %rsp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	retq
	.seh_handlerdata
	.text
	.seh_endproc

	.def	 FunctionDispatcher;
	.scl	2;
	.type	32;
	.endef
	.globl	FunctionDispatcher
	.p2align	4, 0x90
FunctionDispatcher:
.seh_proc FunctionDispatcher
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	movq	$0, 32(%rsp)
	.p2align	4, 0x90
.LBB2_1:
	movq	32(%rsp), %rax
	movq	(%rcx,%rax,8), %r9
	movl	(%r9), %r10d
	cmpl	$-1, %r10d
	je	.LBB2_2
	addq	$1, %rax
	movq	%rax, 32(%rsp)
	cmpl	%edx, %r10d
	jne	.LBB2_1
	movq	8(%r9), %rax
	movq	(%rax,%r8,8), %rax
	jmp	.LBB2_5
.LBB2_2:
	leaq	.L__unnamed_2(%rip), %rcx
	callq	puts
	movq	$-1, %rax
.LBB2_5:
	addq	$40, %rsp
	retq
	.seh_handlerdata
	.text
	.seh_endproc

	.def	 AnyFunctionDispatcher;
	.scl	2;
	.type	32;
	.endef
	.globl	AnyFunctionDispatcher
	.p2align	4, 0x90
AnyFunctionDispatcher:
.seh_proc AnyFunctionDispatcher
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	movq	%rdx, %r8
	movq	(%rcx), %rcx
	movl	$1, %edx
	callq	FunctionDispatcher
	nop
	addq	$40, %rsp
	retq
	.seh_handlerdata
	.text
	.seh_endproc

	.def	 Int32.vtable.init;
	.scl	2;
	.type	32;
	.endef
	.globl	Int32.vtable.init
	.p2align	4, 0x90
Int32.vtable.init:
.seh_proc Int32.vtable.init
	pushq	%rsi
	.seh_pushreg %rsi
	subq	$32, %rsp
	.seh_stackalloc 32
	.seh_endprologue
	movl	$8, %ecx
	callq	malloc
	movq	%rax, Int32.vtable.traits(%rip)
	movl	$12, %ecx
	callq	malloc
	movq	%rax, %rsi
	movl	$1, (%rax)
	movq	Int32.vtable.traits(%rip), %rax
	movq	%rsi, (%rax)
	xorl	%ecx, %ecx
	callq	malloc
	leaq	Int32.typeId.0(%rip), %rdx
	movq	%rdx, (%rax)
	leaq	Int32.toString.0(%rip), %r8
	movq	%r8, 8(%rax)
	movq	%rax, 8(%rsi)
	movl	$2, %ecx
	callq	myVTableFactory
	movq	Int32.vtable.traits(%rip), %rax
	addq	$32, %rsp
	popq	%rsi
	retq
	.seh_handlerdata
	.text
	.seh_endproc

	.def	 main;
	.scl	2;
	.type	32;
	.endef
	.globl	main
	.p2align	4, 0x90
main:
.seh_proc main
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	callq	test
	xorl	%eax, %eax
	addq	$40, %rsp
	retq
	.seh_handlerdata
	.text
	.seh_endproc

	.def	 test;
	.scl	2;
	.type	32;
	.endef
	.globl	test
	.p2align	4, 0x90
test:
.seh_proc test
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	leaq	.Ls1(%rip), %rcx
	callq	puts
	nop
	addq	$40, %rsp
	retq
	.seh_handlerdata
	.text
	.seh_endproc

	.section	.rdata,"dr"
.L__unnamed_1:
	.asciz	"%d"

.L__unnamed_2:
	.asciz	"Could not find function"

	.bss
	.globl	Int32.vtable.traits
	.p2align	3
Int32.vtable.traits:
	.quad	0

	.section	.rdata,"dr"
.Ls1:
	.asciz	"Hello world\n"

