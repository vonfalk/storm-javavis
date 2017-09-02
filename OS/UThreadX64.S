	.text

	# os::doSwitch(...) is _ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_
	.globl _ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_
	.type _ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_, @function
_ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_:
	# NOTE: newDesc is in RDI
	# NOTE: oldDesc is in RSI

	# Prolog. Bonus: saves RBP!
	push %rbp
	movq %rsp, %rbp

	# Save state. We're saving 'rdi' even though it is not strictly neccessary in order to call simple functions.
	pushq %rdi
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	# Save stack information. Creating a StackDesc structure on the stack.
	# We do not know anything about the stack on X86-64...
	pushq $0
	pushq $0
	# Current stack pointer, overlaps with 'high' in StackDesc.
	leaq -8(%rsp), %rbx
	pushq %rbx

	# Report the state of the old thread. This makes it possible for the GC to scan that properly.
	movq %rsp, (%rsi)

	# Switch to the new stack.
	movq (%rdi), %rsp

	# Skip the ThreadDesc struct.
	addq $24, %rsp

	# Store newDesc in RAX so that we do not overwrite it.
	movq %rdi, %rax

	# Restore state.
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rdi

	# Clear the new thread's description. This means that the GC will start scanning the
	# stack based on RSP rather than what is in the StackDesc.
	movq $0, (%rax)

	# Epilog. Bonus: restores RBP!
	pop %rbp
	retq

	# os::doEndDetour() is _ZN2os11doEndDetourEv
	.globl _ZN2os11doEndDetourEv
	.type _ZN2os11doEndDetourEv, @function
_ZN2os11doEndDetourEv:
	# Reserve space for the additional return value.
	pushq $0
	mov %rsp, %rax

	# Align the stack to 16 bytes. Otherwise our 'movaps' instructions will not work. We know that %rsp will
	# be aligned to 16 bytes when the function is called, so this will keep the alignment to 16 bytes.
	pushq $0

	# Save registers required for calling another function.
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	pushq %r9

	# Floating-point registers as well.
	subq $16, %rsp
	movaps %xmm0, (%rsp)
	subq $16, %rsp
	movaps %xmm1, (%rsp)
	subq $16, %rsp
	movaps %xmm2, (%rsp)
	subq $16, %rsp
	movaps %xmm3, (%rsp)
	subq $16, %rsp
	movaps %xmm4, (%rsp)
	subq $16, %rsp
	movaps %xmm5, (%rsp)
	subq $16, %rsp
	movaps %xmm6, (%rsp)
	subq $16, %rsp
	movaps %xmm7, (%rsp)

	# Now, we have saved everything. Call the 'doEndDetour2' function in C to perform thread switches properly.
	movq %rax, %rdi
	call doEndDetour2

	# Restore the state we saved on the stack!
	movaps (%rsp), %xmm7
	addq $16, %rsp
	movaps (%rsp), %xmm6
	addq $16, %rsp
	movaps (%rsp), %xmm5
	addq $16, %rsp
	movaps (%rsp), %xmm4
	addq $16, %rsp
	movaps (%rsp), %xmm3
	addq $16, %rsp
	movaps (%rsp), %xmm2
	addq $16, %rsp
	movaps (%rsp), %xmm1
	addq $16, %rsp
	movaps (%rsp), %xmm0
	addq $16, %rsp

	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi

	# Remove the alignment value.
	addq $8, %rsp

	# Let 'ret' handle the other one, which should contain an address now!
	retq
