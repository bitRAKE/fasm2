; log_edit.inc - insert formatted log lines at the top of the edit control.

define __GLOBAL_BSS__ log_edit_bss

macro log_edit_bss
	align 8
	message_index	dd ?
	filter_noise	dd ?
	coalesce_adjacent dd ?
	noise_show_mask dd ?
	log_busy	dd ?
	last_message_valid dd ?
	last_msgid	dd ?
	last_repeat_count dd ?
	last_first_index dd ?
	last_line_chars dd ?
	dd ?
	last_wparam	dq ?
	last_lparam	dq ?
	log_line	du LOG_LINE_CHARS dup (?)
purge log_edit_bss
end macro

proc InitLogEdit
	mov	[filter_noise],1
	mov	[coalesce_adjacent],1
	mov	[noise_show_mask],0
	mov	[last_message_valid],0
	invoke	SendMessage,[hEdit],EM_SETLIMITTEXT,LOG_TEXT_LIMIT,0
	ret
endp

proc ResetLastMessage
	mov	[last_message_valid],0
	mov	[last_repeat_count],0
	mov	[last_line_chars],0
	ret
endp

proc ClearLog
	cmp	qword [hEdit],0
	je	.done
	cmp	[log_busy],0
	jne	.done
	mov	[log_busy],1
	invoke	SetWindowText,[hEdit],''
	mov	[log_busy],0
	fastcall ResetLastMessage
  .done:
	ret
endp

proc InsertLogLine linep
	mov	[linep],rcx
	cmp	qword [hEdit],0
	je	.done
	cmp	[log_busy],0
	jne	.done

	mov	[log_busy],1
	invoke	SendMessage,[hEdit],WM_SETREDRAW,0,0

	invoke	GetWindowTextLength,[hEdit]
	cmp	eax,LOG_TRIM_TRIGGER
	jbe	.insert
	invoke	SendMessage,[hEdit],EM_SETSEL,LOG_TRIM_KEEP,-1
	invoke	SendMessage,[hEdit],EM_REPLACESEL,0,''

  .insert:
	invoke	SendMessage,[hEdit],EM_SETSEL,0,0
	invoke	SendMessage,[hEdit],EM_REPLACESEL,0,[linep]
	invoke	SendMessage,[hEdit],EM_SETSEL,0,0
	invoke	SendMessage,[hEdit],EM_SCROLLCARET,0,0
	invoke	SendMessage,[hEdit],WM_SETREDRAW,1,0
	invoke	InvalidateRect,[hEdit],0,1
	mov	[log_busy],0
  .done:
	ret
endp

proc ReplaceTopLogLine linep,new_chars
	mov	[linep],rcx
	mov	dword [new_chars],edx
	cmp	qword [hEdit],0
	je	.done
	cmp	[log_busy],0
	jne	.done

	mov	[log_busy],1
	invoke	SendMessage,[hEdit],WM_SETREDRAW,0,0
	invoke	SendMessage,[hEdit],EM_SETSEL,0,dword [last_line_chars]
	invoke	SendMessage,[hEdit],EM_REPLACESEL,0,[linep]
	invoke	SendMessage,[hEdit],EM_SETSEL,0,0
	invoke	SendMessage,[hEdit],EM_SCROLLCARET,0,0
	invoke	SendMessage,[hEdit],WM_SETREDRAW,1,0
	invoke	InvalidateRect,[hEdit],0,1
	mov	eax,dword [new_chars]
	mov	[last_line_chars],eax
	mov	[log_busy],0
  .done:
	ret
endp

proc LogText linep
	mov	[linep],rcx
	cmp	[log_busy],0
	jne	.done
	fastcall ResetLastMessage
	fastcall InsertLogLine,[linep]
  .done:
	ret
endp

proc LogDropCount count
	mov	dword [count],ecx
	cmp	[log_busy],0
	jne	.done
	fastcall ResetLastMessage
	invoke	wsprintf,log_line,<'    -> %u file(s) dropped',13,10>,dword [count]
	fastcall InsertLogLine,log_line
  .done:
	ret
endp

proc LogMessage msgid,wparam,lparam
    locals
	namep	dq ?
	wp_hi	dd ?
	wp_lo	dd ?
	lp_hi	dd ?
	lp_lo	dd ?
	line_chars dd ?
    endl

	mov	dword [msgid],ecx
	mov	[wparam],rdx
	mov	[lparam],r8

	cmp	qword [hEdit],0
	je	.done
	cmp	[log_busy],0
	jne	.done
	cmp	[filter_noise],0
	je	.format
	fastcall IsNoiseFiltered,dword [msgid]
	test	eax,eax
	jnz	.done

  .format:
	inc	[message_index]
	cmp	[coalesce_adjacent],0
	je	.new_line
	cmp	[last_message_valid],0
	je	.new_line
	mov	eax,dword [msgid]
	cmp	eax,[last_msgid]
	jne	.new_line
	mov	rax,[wparam]
	cmp	rax,[last_wparam]
	jne	.new_line
	mov	rax,[lparam]
	cmp	rax,[last_lparam]
	jne	.new_line
	inc	[last_repeat_count]
	jmp	.repeat_line

  .new_line:
	mov	[last_message_valid],1
	mov	eax,dword [msgid]
	mov	[last_msgid],eax
	mov	rax,[wparam]
	mov	[last_wparam],rax
	mov	rax,[lparam]
	mov	[last_lparam],rax
	mov	eax,[message_index]
	mov	[last_first_index],eax
	mov	[last_repeat_count],1
	fastcall MsgName,dword [msgid]
	mov	[namep],rax

	mov	rax,[wparam]
	mov	[wp_lo],eax
	shr	rax,32
	mov	[wp_hi],eax
	mov	rax,[lparam]
	mov	[lp_lo],eax
	shr	rax,32
	mov	[lp_hi],eax

	invoke	wsprintf,log_line,<'[%6u] 0x%04X %-30ls w=0x%08X%08X l=0x%08X%08X',13,10>,\
		dword [message_index],dword [msgid],[namep],\
		dword [wp_hi],dword [wp_lo],dword [lp_hi],dword [lp_lo]
	mov	[line_chars],eax
	fastcall InsertLogLine,log_line
	mov	eax,[line_chars]
	mov	[last_line_chars],eax
	jmp	.done

  .repeat_line:
	fastcall MsgName,dword [msgid]
	mov	[namep],rax

	mov	rax,[wparam]
	mov	[wp_lo],eax
	shr	rax,32
	mov	[wp_hi],eax
	mov	rax,[lparam]
	mov	[lp_lo],eax
	shr	rax,32
	mov	[lp_hi],eax

	invoke	wsprintf,log_line,<'[%6u-%6u x%u] 0x%04X %-30ls w=0x%08X%08X l=0x%08X%08X',13,10>,\
		dword [last_first_index],dword [message_index],dword [last_repeat_count],\
		dword [msgid],[namep],\
		dword [wp_hi],dword [wp_lo],dword [lp_hi],dword [lp_lo]
	mov	[line_chars],eax
	fastcall ReplaceTopLogLine,log_line,dword [line_chars]

  .done:
	ret
endp

proc ToggleNoiseFilter
	xor	[filter_noise],1
	cmp	[filter_noise],0
	je	.off
	fastcall LogText,<'--- noise filter ON ---',13,10>
	ret
  .off:
	fastcall LogText,<'--- noise filter OFF ---',13,10>
	ret
endp

proc ToggleNoiseGroup mask
	xor	[noise_show_mask],ecx
	ret
endp

proc ToggleAdjacentCoalesce
	xor	[coalesce_adjacent],1
	ret
endp

proc ShowAllNoiseGroups
	mov	[noise_show_mask],NOISE_ALL
	ret
endp

proc HideAllNoiseGroups
	mov	[noise_show_mask],0
	ret
endp
