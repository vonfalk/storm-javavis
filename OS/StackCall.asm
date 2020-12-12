.386
.model flat, c

doStackCall PROTO C
fnCallRaw PROTO C

assume fs:nothing

.code

doStackCall proc
	;; "current" is at [ebp+8]
	;; "callOn"  is at [ebp+12]
	;; "fn"      is at [ebp+16]
	;; "fnCall"  is at [ebp+20]
	;; "member"  is at [ebp+24]
	;; "result"  is at [ebp+28]

	;; We can trash eax, ecx and edx

	;; Prolog. Saves ebp
	push ebp
	mov ebp, esp

	;; Store the stack description in the thread. Note: we don't save fs:[0], as we want
	;; exceptions to be able to pass through the thread call.
	push DWORD PTR fs:[4]	; Stack base (high address)
	push DWORD PTR fs:[8] 	; Stack limit (low address)
	lea eax, [esp-4]
	push eax 		; Current stack pointer. Allows the GC to scan this stack.

	;; Store the state in the old stack description. This tells the GC to scan it using that
	;; description rather than our esp.
	mov eax, [ebp+8]
	mov [eax+8], esp 	; current.desc = esp
	mov ecx, [ebp+12]
	mov [eax+16], ecx 	; current.detourTo = callOn

	;; Switch to the other stack.
	mov eax, [ecx+8] 	; eax = callOn.desc
	mov esp, [eax]		; esp = callOn.desc->low
	mov DWORD PTR [ecx+8], 0; callOn.desc = 0

	;; We need to fix fs:[4] and fs:[8], otherwise exceptions won't work.
	;; We want them to be the maximum of our limits and the limits of the new stack.
	mov edx, [eax+8] 	; Load high address of the new stack
	cmp edx, fs:[4] 	; Compare to current high address
	jb no_update_high
	mov fs:[4], edx 	; If [eax+8] >= fs:[4] : update fs:[4]
	
no_update_high:
	mov edx, [eax] 		; Load low address of the new stack
	cmp edx, fs:[8] 	; Compare to current low addres
	ja no_update_low
	mov fs:[8], edx 	; If [eax] <= fs:[8] : update fs:[8]
no_update_low:

	;; Now we're done with the updates of the stack, so now we can go on to call 'callRaw':
	push [ebp+28]		; result
	push 0			; first
	push [ebp+24]		; member
	push [ebp+16]		; fn
	push [ebp+20]		; me
	call fnCallRaw
	add esp, 20 		; clean up the stack

	;; Now we need to switch back to the old stack. We need to be a bit careful with the
	;; order here as well. Otherwise, the GC might scan the two stack in an inconsistent
	;; state with relation to our esp.
	mov ecx, [ebp+8] 	; Load "current"
	mov DWORD PTR [ecx+16], 0; current.detourTo = 0
	mov eax, [ecx+8] 	; load current.desc.
	mov esp, [eax] 		; restore our stack pointer from current.desc->low

	;; Now, we just need to restore the fs:[4] and fs:[8], then we're ready to go!
	add esp, 4 		; "pop" the esp
	pop DWORD PTR fs:[8] 	; Stack limit
	pop DWORD PTR fs:[4] 	; Stack base

	;; Epilog. Restores ebp
	mov esp, ebp
	pop ebp
	ret

doStackCall endp

end
