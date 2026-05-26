; file_io.inc - UTF-8 text load/save for the edit control.

proc OpenPathIntoEditor uses rbx rsi rdi, pathp
    locals
	hfile dq ?
	rawp dq ?
	widep dq ?
	size dd ?
	chars dd ?
    endl

	mov	[pathp],rcx
	invoke	CreateFileW,[pathp],GENERIC_READ,FILE_SHARE_READ,0,\
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0
	cmp	rax,-1
	je	.fail
	mov	[hfile],rax

	mov	dword [file_high],0
	invoke	GetFileSize,[hfile],addr file_high
	cmp	dword [file_high],0
	jne	.too_large
	cmp	eax,MAX_LOAD_BYTES
	ja	.too_large
	mov	[size],eax
	inc	eax
	invoke	LocalAlloc,LPTR,rax
	test	rax,rax
	jz	.close_fail
	mov	[rawp],rax

	invoke	ReadFile,[hfile],[rawp],dword [size],addr bytes_done,0
	test	eax,eax
	jz	.free_raw_fail
	invoke	CloseHandle,[hfile]
	mov	qword [hfile],0

	cmp	dword [bytes_done],0
	jne	.convert_text
	mov	dword [suppress_change],1
	invoke	SetWindowText,[hEdit],''
	mov	dword [suppress_change],0
	fastcall SetCurrentPath,[pathp]
	mov	dword [dirty],0
	fastcall UpdateTitle
	fastcall SaveCurrentToRecent
	fastcall PopulateRecentMenu
	invoke	LocalFree,[rawp]
	mov	eax,1
	ret

  .convert_text:
	invoke	MultiByteToWideChar,CP_UTF8,0,[rawp],dword [bytes_done],0,0
	test	eax,eax
	jnz	.have_count
	invoke	MultiByteToWideChar,0,0,[rawp],dword [bytes_done],0,0
	test	eax,eax
	jz	.free_raw_fail
  .have_count:
	mov	[chars],eax
	inc	eax
	shl	eax,1
	invoke	LocalAlloc,LPTR,rax
	test	rax,rax
	jz	.free_raw_fail
	mov	[widep],rax

	invoke	MultiByteToWideChar,CP_UTF8,0,[rawp],dword [bytes_done],\
		[widep],dword [chars]
	test	eax,eax
	jnz	.set_text
	invoke	MultiByteToWideChar,0,0,[rawp],dword [bytes_done],\
		[widep],dword [chars]
	test	eax,eax
	jz	.free_both_fail

  .set_text:
	mov	dword [suppress_change],1
	invoke	SetWindowText,[hEdit],[widep]
	mov	dword [suppress_change],0
	fastcall SetCurrentPath,[pathp]
	mov	dword [dirty],0
	fastcall UpdateTitle
	fastcall SaveCurrentToRecent
	fastcall PopulateRecentMenu
	invoke	LocalFree,[widep]
	invoke	LocalFree,[rawp]
	mov	eax,1
	ret

  .too_large:
	invoke	MessageBox,[hMain],'File is too large for this teaching example.',app_name,MB_ICONWARNING
  .close_fail:
	cmp	qword [hfile],0
	je	.fail
	invoke	CloseHandle,[hfile]
	jmp	.fail
  .free_both_fail:
	invoke	LocalFree,[widep]
  .free_raw_fail:
	invoke	LocalFree,[rawp]
  .fail:
	invoke	MessageBox,[hMain],'Could not open the selected file.',app_name,MB_ICONERROR
	xor	eax,eax
	ret
endp

proc SaveEditorToPath uses rbx, pathp
    locals
	hfile dq ?
	widep dq ?
	utf8p dq ?
	wchars dd ?
	bytes dd ?
	ok dd ?
    endl

	mov	[pathp],rcx
	invoke	GetWindowTextLength,[hEdit]
	mov	[wchars],eax
	inc	eax
	shl	eax,1
	invoke	LocalAlloc,LPTR,rax
	test	rax,rax
	jz	.fail
	mov	[widep],rax
	mov	eax,[wchars]
	inc	eax
	invoke	GetWindowText,[hEdit],[widep],eax

	invoke	WideCharToMultiByte,CP_UTF8,0,[widep],-1,0,0,0,0
	test	eax,eax
	jz	.free_wide_fail
	mov	[bytes],eax
	invoke	LocalAlloc,LPTR,rax
	test	rax,rax
	jz	.free_wide_fail
	mov	[utf8p],rax
	invoke	WideCharToMultiByte,CP_UTF8,0,[widep],-1,[utf8p],dword [bytes],0,0
	test	eax,eax
	jz	.free_both_fail

	invoke	CreateFileW,[pathp],GENERIC_WRITE,0,0,CREATE_ALWAYS,\
		FILE_ATTRIBUTE_NORMAL,0
	cmp	rax,-1
	je	.free_both_fail
	mov	[hfile],rax
	mov	eax,[bytes]
	dec	eax
	invoke	WriteFile,[hfile],[utf8p],eax,addr bytes_done,0
	mov	[ok],eax
	invoke	CloseHandle,[hfile]
	cmp	dword [ok],0
	jz	.free_both_fail

	fastcall SetCurrentPath,[pathp]
	mov	dword [dirty],0
	fastcall UpdateTitle
	fastcall SaveCurrentToRecent
	fastcall PopulateRecentMenu
	invoke	LocalFree,[utf8p]
	invoke	LocalFree,[widep]
	mov	eax,1
	ret

  .free_both_fail:
	invoke	LocalFree,[utf8p]
  .free_wide_fail:
	invoke	LocalFree,[widep]
  .fail:
	invoke	MessageBox,[hMain],'Could not save the file.',app_name,MB_ICONERROR
	xor	eax,eax
	ret
endp
