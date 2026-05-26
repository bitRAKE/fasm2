; find_dialog.inc - modeless Find dialog and edit-control search.
;
; This module deliberately uses a resource-backed modeless dialog so the app's
; message loop has to route dialog keyboard navigation through IsDialogMessage.

EM_GETSEL	= 00B0h
EM_SETSEL	= 00B1h

define __GLOBAL_BSS__ find_dialog_bss

macro find_dialog_bss
	align 8
	find_buffer	rw FIND_TEXT_MAX
	find_text_ptr	dq ?
	find_needle_ptr dq ?
	find_text_len	dd ?
	find_needle_len dd ?
	find_match_case dd ?
	find_whole_word dd ?
purge find_dialog_bss
end macro

proc ShowFindDialog
	cmp	qword [hFindDlg],0
	jne	.focus_existing
	invoke	CreateDialogParam,[hInstance],IDD_FIND,[hMain],FindDlgProc,0
	mov	[hFindDlg],rax
	test	rax,rax
	jz	.done
	invoke	ShowWindow,rax,SW_SHOW
	ret
  .focus_existing:
	invoke	SetFocus,[hFindDlg]
  .done:
	ret
endp

proc FindNextFromCommand
	fastcall wcs_is_empty,find_buffer
	test	eax,eax
	jnz	.show_dialog
	fastcall FindNextInEditor,find_buffer
	ret
  .show_dialog:
	fastcall ShowFindDialog
	ret
endp

proc FindDlgProc hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_INITDIALOG
	je	.init
	cmp	edx,WM_COMMAND
	je	.command
	cmp	edx,WM_CLOSE
	je	.close
	cmp	edx,WM_DESTROY
	je	.destroy

	xor	eax,eax
	ret

  .init:
	mov	[hFindDlg],rcx
	invoke	SetDlgItemText,rcx,IDC_FIND_TEXT,find_buffer
	mov	eax,1
	ret

  .command:
	mov	eax,dword [wparam]
	movzx	ecx,ax
	cmp	ecx,ID_FIND_NEXT
	je	.find_next
	cmp	ecx,IDCANCEL
	je	.close
	xor	eax,eax
	ret

  .find_next:
	invoke	GetDlgItemText,[hwnd],IDC_FIND_TEXT,find_buffer,FIND_TEXT_MAX
	test	eax,eax
	jz	.handled
	invoke	IsDlgButtonChecked,[hwnd],IDC_MATCH_CASE
	mov	[find_match_case],eax
	invoke	IsDlgButtonChecked,[hwnd],IDC_WHOLE_WORD
	mov	[find_whole_word],eax
	fastcall FindNextInEditor,find_buffer
  .handled:
	mov	eax,1
	ret

  .close:
	invoke	DestroyWindow,[hwnd]
	mov	eax,1
	ret

  .destroy:
	mov	qword [hFindDlg],0
	mov	eax,1
	ret
endp

proc IsWordChar char_value
	mov	[char_value],rcx
	mov	eax,ecx
	cmp	eax,'0'
	jb	.no
	cmp	eax,'9'
	jbe	.yes
	cmp	eax,'A'
	jb	.underscore
	cmp	eax,'Z'
	jbe	.yes
	cmp	eax,'a'
	jb	.underscore
	cmp	eax,'z'
	jbe	.yes
  .underscore:
	cmp	eax,'_'
	je	.yes
  .no:
	xor	eax,eax
	ret
  .yes:
	mov	eax,1
	ret
endp

proc SearchRange uses rbx rsi rdi r12 r13 r14, start_index,end_index
	mov	[start_index],rcx
	mov	[end_index],rdx
	mov	ebx,ecx
	mov	r12d,edx
	mov	rdi,[find_text_ptr]
	mov	rsi,[find_needle_ptr]
	mov	r13d,[find_needle_len]
	mov	r14d,[find_match_case]

  .candidate:
	cmp	ebx,r12d
	jae	.not_found
	xor	edx,edx
  .match_loop:
	cmp	edx,r13d
	jae	.matched
	mov	r10d,ebx
	add	r10d,edx
	movzx	eax,word [rdi+r10*2]
	movzx	r11d,word [rsi+rdx*2]
	cmp	r14d,0
	jne	.compare
	cmp	eax,'A'
	jb	.fold_needle
	cmp	eax,'Z'
	ja	.fold_needle
	add	eax,32
  .fold_needle:
	cmp	r11d,'A'
	jb	.compare
	cmp	r11d,'Z'
	ja	.compare
	add	r11d,32
  .compare:
	cmp	ax,r11w
	jne	.next_candidate
	inc	edx
	jmp	.match_loop

  .matched:
	cmp	dword [find_whole_word],0
	je	.found
	test	ebx,ebx
	jz	.check_after
	mov	r10d,ebx
	dec	r10d
	movzx	eax,word [rdi+r10*2]
	fastcall IsWordChar,rax
	test	eax,eax
	jnz	.next_candidate
  .check_after:
	mov	r10d,ebx
	add	r10d,r13d
	cmp	r10d,[find_text_len]
	jae	.found
	movzx	eax,word [rdi+r10*2]
	fastcall IsWordChar,rax
	test	eax,eax
	jnz	.next_candidate
  .found:
	mov	eax,ebx
	ret

  .next_candidate:
	inc	ebx
	jmp	.candidate

  .not_found:
	mov	eax,-1
	ret
endp

proc FindNextInEditor uses rbx, needlep
    locals
	textp dq ?
	sel_start dd ?
	sel_end dd ?
	sel_to dd ?
	limit dd ?
	found dd ?
    endl

	mov	[needlep],rcx
	fastcall wcs_len,[needlep]
	test	eax,eax
	jz	.not_found
	mov	[find_needle_len],eax
	mov	rax,[needlep]
	mov	[find_needle_ptr],rax

	invoke	GetWindowTextLength,[hEdit]
	mov	[find_text_len],eax
	cmp	eax,[find_needle_len]
	jb	.not_found

	inc	eax
	shl	eax,1
	invoke	LocalAlloc,LPTR,rax
	test	rax,rax
	jz	.not_found
	mov	[textp],rax
	mov	[find_text_ptr],rax
	mov	eax,[find_text_len]
	inc	eax
	invoke	GetWindowText,[hEdit],[textp],eax

	mov	eax,[find_text_len]
	sub	eax,[find_needle_len]
	inc	eax
	mov	[limit],eax

	invoke	SendMessage,[hEdit],EM_GETSEL,addr sel_start,addr sel_end
	mov	ebx,[sel_end]
	cmp	ebx,[limit]
	jae	.wrap
	fastcall SearchRange,rbx,dword [limit]
	cmp	eax,-1
	jne	.select

  .wrap:
	cmp	ebx,0
	je	.fail_with_alloc
	fastcall SearchRange,0,rbx
	cmp	eax,-1
	je	.fail_with_alloc

  .select:
	mov	[found],eax
	mov	ecx,eax
	add	ecx,[find_needle_len]
	mov	[sel_to],ecx
	invoke	SendMessage,[hEdit],EM_SETSEL,dword [found],dword [sel_to]
	invoke	SetFocus,[hEdit]
	invoke	LocalFree,[textp]
	mov	eax,1
	ret

  .fail_with_alloc:
	invoke	LocalFree,[textp]
  .not_found:
	invoke	MessageBox,[hMain],'Text not found.',app_name,MB_ICONINFORMATION
	xor	eax,eax
	ret
endp
