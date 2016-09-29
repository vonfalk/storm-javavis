.386
.model flat, stdcall

extern x86SEH:near

x86SafeSEH proto
.SAFESEH x86SafeSEH

.code
x86SafeSeh proc
	jmp x86SEH
x86SafeSEH endp
	
end
