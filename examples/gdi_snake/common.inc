; common.inc - tiny helpers used by both simulation and UI modules.

proc RandomU32
	mov	eax,[rng_state]
	test	eax,eax
	jnz	.have_seed
	mov	eax,2463534242
  .have_seed:
	mov	edx,eax
	shl	edx,13
	xor	eax,edx
	mov	edx,eax
	shr	edx,17
	xor	eax,edx
	mov	edx,eax
	shl	edx,5
	xor	eax,edx
	mov	[rng_state],eax
	ret
endp

proc RandomBelow max_value
	test	ecx,ecx
	jnz	.valid
	xor	eax,eax
	ret
  .valid:
	fastcall RandomU32
	xor	edx,edx
	div	ecx
	mov	eax,edx
	ret
endp

proc SetGameMessage textp,ticks
	mov	[message_text],rcx
	mov	[message_ticks],edx
	ret
endp

proc ClampBoardValue value,min_value,max_value
	mov	eax,ecx
	cmp	eax,edx
	jae	.check_high
	mov	eax,edx
	ret
  .check_high:
	cmp	eax,r8d
	jbe	.done
	mov	eax,r8d
  .done:
	ret
endp
