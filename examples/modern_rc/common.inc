; common.inc - small helpers intentionally independent of the app feature set.

FNV_OFFSET	:= 0xcbf29ce484222325
FNV_PRIME	:= 0x100000001b3

proc fnv1a_w uses rbx, textp
	mov	[textp],rcx
	mov	rbx,rcx
	mov	rax,FNV_OFFSET
	mov	r8,FNV_PRIME
  .loop:
	movzx	edx,word [rbx]
	test	edx,edx
	jz	.done
	cmp	edx,'A'
	jb	.no_case
	cmp	edx,'Z'
	ja	.no_case
	add	edx,32
  .no_case:
	xor	rax,rdx
	imul	rax,r8
	add	rbx,2
	jmp	.loop
  .done:
	ret
endp

proc wcscpy_n uses rsi rdi, dstp,srcp,maxchars
	mov	[dstp],rcx
	mov	[srcp],rdx
	mov	[maxchars],r8
	mov	rdi,[dstp]
	mov	rsi,[srcp]
	mov	r8,[maxchars]
	test	r8,r8
	jz	.done
	dec	r8
	xor	ecx,ecx
  .copy:
	cmp	rcx,r8
	jae	.terminate
	movzx	eax,word [rsi+rcx*2]
	mov	word [rdi+rcx*2],ax
	test	eax,eax
	jz	.done
	inc	rcx
	jmp	.copy
  .terminate:
	mov	word [rdi+r8*2],0
  .done:
	xor	eax,eax
	ret
endp

proc wcs_is_empty textp
	mov	[textp],rcx
	movzx	eax,word [rcx]
	test	eax,eax
	setz	al
	movzx	eax,al
	ret
endp

proc wcs_len textp
	mov	[textp],rcx
	xor	eax,eax
  .loop:
	cmp	word [rcx+rax*2],0
	je	.done
	inc	eax
	jmp	.loop
  .done:
	ret
endp

proc ClearMenuItems menup,start_pos
	mov	[menup],rcx
	mov	[start_pos],rdx
  .loop:
	invoke	GetMenuItemCount,[menup]
	cmp	eax,dword [start_pos]
	jle	.done
	invoke	RemoveMenu,[menup],dword [start_pos],MF_BYPOSITION
	test	eax,eax
	jz	.done
	jmp	.loop
  .done:
	xor	eax,eax
	ret
endp

proc SetCurrentPath pathp
	mov	[pathp],rcx
	fastcall wcscpy_n,current_path,[pathp],MAX_PATH
	ret
endp

proc ClearCurrentPath
	mov	word [current_path],0
	xor	eax,eax
	ret
endp
