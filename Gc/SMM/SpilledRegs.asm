	.386
	.model flat

	.code
_spillRegs proc
	mov eax, [esp+4]
	mov [eax], ebx
	mov [eax+4], ebp
	mov [eax+8], esi
	mov [eax+12], edi
	ret
_spillRegs endp

end
