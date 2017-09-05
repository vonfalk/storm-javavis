	.text

	# os::doSwitch(...) is _ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_
	.globl _ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_
	.type _ZN2os8doSwitchEPPNS_12UThreadStack4DescES3_, @function
	.align 4
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

	# doEndDetour()
	.globl doEndDetour
	.type doEndDetour, @function
	.align 4 # To avoid being identified as a member function.
doEndDetour:
	# Reserve space for the additional return value.
	pushq $0
	mov %rsp, %rax

	# NOTE: Since the last 'parameter' to a function is known to be aligned at a 16 byte boundary, we know
	# that %rsp is now aligned on a 16 byte boundary as well. This alignment holds up all the way through this
	# function since we push exactly 6 words on the stack, followed by a couple of 16 byte XMM registers.

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

	# Let 'ret' handle the other one, which should contain an address now!
	retq

	# doEndDetourMember()
	# Like doEndDetour, but checks if a vtable lookup is required before issuing a 'ret' instruction.
	.globl doEndDetourMember
	.type doEndDetourMember, @function
	.align 4 # To avoid being identified as a member function.
doEndDetourMember:
	# Reserve space for the additional return value.
	pushq $0
	mov %rsp, %rax

	# NOTE: Since the last 'parameter' to a function is known to be aligned at a 16 byte boundary, we know
	# that %rsp is now aligned on a 16 byte boundary as well. This alignment holds up all the way through this
	# function since we push exactly 6 words on the stack, followed by a couple of 16 byte XMM registers.

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

	# See if we are about to return to a member function. We can only trash %rax now!
	movq (%rsp), %rax

	# If it is odd, we need to do a vtable lookup!
	andq $1, %rax
	testq %rax, %rax
	je NO_MEMBER

	# Perform a vtable lookup and store it on the stack, ready for 'retq' to consume.
	movq (%rdi), %rax  # load the vtable pointer
	addq (%rsp), %rax  # add the offset
	subq $1, %rax      # remove the marker
	movq (%rax), %rax  # read the function pointer
	movq %rax, (%rsp)  # and save it to the stack for 'retq'

NO_MEMBER:
	# Let 'ret' handle the other one, which should contain an address now!
	retq
