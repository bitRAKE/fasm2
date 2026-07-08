; addon/csd/state.inc - mutable caption-control state helpers.

struct CSD_CAPTION_INPUT
  hotIndex	dd ?
  pressedIndex	dd ?
  tracking	dd ?
  active	dd ?
ends

proc CsdCaptionSetAllStates statep,count,value
	test	edx,edx
	jz	set_all_done

  set_all_loop:
	mov	[rcx+CSD_CAPTION_STATE.value],r8b
	mov	byte [rcx+CSD_CAPTION_STATE.fadePhase],255
	mov	[rcx+CSD_CAPTION_STATE.fromValue],r8b
	mov	byte [rcx+CSD_CAPTION_STATE.animFlags],0
	add	rcx,sizeof.CSD_CAPTION_STATE
	dec	edx
	jnz	set_all_loop

  set_all_done:
	ret
endp

proc CsdCaptionStateApply inputp,statep,count
	mov	r9d,[rcx+CSD_CAPTION_INPUT.hotIndex]
	mov	r10d,[rcx+CSD_CAPTION_INPUT.pressedIndex]
	xor	eax,eax
	test	r8d,r8d
	jz	apply_done
	cmp	dword [rcx+CSD_CAPTION_INPUT.active],0
	je	apply_inactive_loop
	xor	ecx,ecx

  apply_loop:
	mov	r11b,CSD_STATE_NORMAL
	cmp	ecx,r9d
	jne	apply_compare
	cmp	ecx,r10d
	jne	apply_hover
	mov	r11b,CSD_STATE_PRESSED
	jmp	apply_compare

  apply_hover:
	mov	r11b,CSD_STATE_HOVER

  apply_compare:
	cmp	[rdx+CSD_CAPTION_STATE.value],r11b
	je	apply_next
	mov	al,[rdx+CSD_CAPTION_STATE.value]
	mov	[rdx+CSD_CAPTION_STATE.fromValue],al
	mov	[rdx+CSD_CAPTION_STATE.value],r11b
	mov	byte [rdx+CSD_CAPTION_STATE.fadePhase],0
	mov	byte [rdx+CSD_CAPTION_STATE.animFlags],CSD_CAPTION_ANIM_ACTIVE
	mov	eax,1

  apply_next:
	add	rdx,sizeof.CSD_CAPTION_STATE
	inc	ecx
	dec	r8d
	jnz	apply_loop
	ret

  apply_inactive_loop:
	cmp	byte [rdx+CSD_CAPTION_STATE.value],CSD_STATE_INACTIVE
	je	apply_inactive_next
	mov	al,[rdx+CSD_CAPTION_STATE.value]
	mov	[rdx+CSD_CAPTION_STATE.fromValue],al
	mov	byte [rdx+CSD_CAPTION_STATE.value],CSD_STATE_INACTIVE
	mov	byte [rdx+CSD_CAPTION_STATE.fadePhase],0
	mov	byte [rdx+CSD_CAPTION_STATE.animFlags],CSD_CAPTION_ANIM_ACTIVE
	mov	eax,1

  apply_inactive_next:
	add	rdx,sizeof.CSD_CAPTION_STATE
	dec	r8d
	jnz	apply_inactive_loop

  apply_done:
	ret
endp

proc CsdCaptionAnimAnyActive statep,count
	test	edx,edx
	jz	anim_any_done

  anim_any_loop:
	test	byte [rcx+CSD_CAPTION_STATE.animFlags],CSD_CAPTION_ANIM_ACTIVE
	jnz	anim_any_yes
	add	rcx,sizeof.CSD_CAPTION_STATE
	dec	edx
	jnz	anim_any_loop

  anim_any_done:
	xor	eax,eax
	ret

  anim_any_yes:
	mov	eax,1
	ret
endp

proc CsdCaptionAnimAdvance statep,count,step
	xor	eax,eax
	test	edx,edx
	jz	anim_advance_done

  anim_advance_loop:
	test	byte [rcx+CSD_CAPTION_STATE.animFlags],CSD_CAPTION_ANIM_ACTIVE
	jz	anim_advance_next
	movzx	r9d,byte [rcx+CSD_CAPTION_STATE.fadePhase]
	add	r9d,r8d
	cmp	r9d,255
	jb	anim_advance_store
	mov	r9d,255
	mov	byte [rcx+CSD_CAPTION_STATE.animFlags],0

  anim_advance_store:
	mov	[rcx+CSD_CAPTION_STATE.fadePhase],r9b
	mov	eax,1

  anim_advance_next:
	add	rcx,sizeof.CSD_CAPTION_STATE
	dec	edx
	jnz	anim_advance_loop

  anim_advance_done:
	ret
endp

proc CsdCaptionInputInit inputp,statep,count
	mov	dword [rcx+CSD_CAPTION_INPUT.hotIndex],CSD_CAPTION_NO_INDEX
	mov	dword [rcx+CSD_CAPTION_INPUT.pressedIndex],CSD_CAPTION_NO_INDEX
	mov	dword [rcx+CSD_CAPTION_INPUT.tracking],0
	mov	dword [rcx+CSD_CAPTION_INPUT.active],1
	fastcall CsdCaptionSetAllStates,rdx,r8d,CSD_STATE_NORMAL
	xor	eax,eax
	ret
endp

proc CsdCaptionSetHot inputp,statep,count,index
	cmp	[rcx+CSD_CAPTION_INPUT.hotIndex],r9d
	jne	set_hot_assign
	xor	eax,eax
	ret

  set_hot_assign:
	mov	[rcx+CSD_CAPTION_INPUT.hotIndex],r9d
	fastcall CsdCaptionStateApply,rcx,rdx,r8d
	ret
endp

proc CsdCaptionSetPressed inputp,statep,count,index
	cmp	[rcx+CSD_CAPTION_INPUT.pressedIndex],r9d
	jne	set_pressed_assign
	xor	eax,eax
	ret

  set_pressed_assign:
	mov	[rcx+CSD_CAPTION_INPUT.pressedIndex],r9d
	fastcall CsdCaptionStateApply,rcx,rdx,r8d
	ret
endp

proc CsdCaptionSetActive inputp,statep,count,active
	mov	[rcx+CSD_CAPTION_INPUT.active],r9d
	mov	dword [rcx+CSD_CAPTION_INPUT.hotIndex],CSD_CAPTION_NO_INDEX
	mov	dword [rcx+CSD_CAPTION_INPUT.pressedIndex],CSD_CAPTION_NO_INDEX
	test	r9d,r9d
	jnz	set_active_apply
	mov	dword [rcx+CSD_CAPTION_INPUT.tracking],0

  set_active_apply:
	fastcall CsdCaptionStateApply,rcx,rdx,r8d
	ret
endp

proc CsdCaptionMouseLeave inputp,statep,count
	mov	dword [rcx+CSD_CAPTION_INPUT.tracking],0
	mov	dword [rcx+CSD_CAPTION_INPUT.hotIndex],CSD_CAPTION_NO_INDEX
	fastcall CsdCaptionStateApply,rcx,rdx,r8d
	ret
endp

proc CsdCaptionTrackMouseLeaveWithFlags hwnd,inputp,flags
    locals
	tme TRACKMOUSEEVENT
    endl

	mov	[inputp],rdx
	cmp	dword [rdx+CSD_CAPTION_INPUT.tracking],0
	jne	track_done
	mov	[tme.cbSize],sizeof.TRACKMOUSEEVENT
	mov	[tme.dwFlags],r8d
	mov	[tme.hwndTrack],rcx
	mov	[tme.dwHoverTime],0
	invoke	TrackMouseEvent,addr tme
	test	eax,eax
	jz	track_done
	mov	rdx,[inputp]
	mov	dword [rdx+CSD_CAPTION_INPUT.tracking],1

  track_done:
	ret
endp

proc CsdCaptionTrackMouseLeave hwnd,inputp
	fastcall CsdCaptionTrackMouseLeaveWithFlags,rcx,rdx,TME_LEAVE
	ret
endp
