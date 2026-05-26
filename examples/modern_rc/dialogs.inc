; dialogs.inc - common file dialogs, isolated from file loading/saving.

define __GLOBAL_BSS__ dialogs_bss

macro dialogs_bss
	align 8
	ofn OPENFILENAME
purge dialogs_bss
end macro

proc PrepareOpenFileName titlep, flags
	mov	[titlep],rcx
	mov	[flags],rdx
	mov	qword [ofn.lStructSize],sizeof.OPENFILENAME
	mov	rax,[hMain]
	mov	[ofn.hwndOwner],rax
	mov	qword [ofn.hInstance],0
	mov	qword [ofn.lpstrFilter],file_filter
	mov	qword [ofn.lpstrCustomFilter],0
	mov	dword [ofn.nMaxCustFilter],0
	mov	dword [ofn.nFilterIndex],1
	mov	qword [ofn.lpstrFile],dialog_path
	mov	dword [ofn.nMaxFile],MAX_PATH
	mov	qword [ofn.lpstrFileTitle],0
	mov	dword [ofn.nMaxFileTitle],0
	mov	qword [ofn.lpstrInitialDir],0
	mov	rax,[titlep]
	mov	[ofn.lpstrTitle],rax
	mov	dword [ofn.Flags],edx
	mov	word [ofn.nFileOffset],0
	mov	word [ofn.nFileExtension],0
	mov	qword [ofn.lpstrDefExt],default_ext
	mov	qword [ofn.lCustData],0
	mov	qword [ofn.lpfnHook],0
	mov	qword [ofn.lpTemplateName],0
	ret
endp

proc ChooseOpenPath
	mov	word [dialog_path],0
	fastcall PrepareOpenFileName,'Open text file',\
		OFN_EXPLORER or OFN_FILEMUSTEXIST or OFN_PATHMUSTEXIST
	invoke	GetOpenFileName,addr ofn
	ret
endp

proc ChooseSavePath
	fastcall wcs_is_empty,current_path
	test	eax,eax
	jnz	.blank
	fastcall wcscpy_n,dialog_path,current_path,MAX_PATH
	jmp	.show
  .blank:
	mov	word [dialog_path],0
  .show:
	fastcall PrepareOpenFileName,'Save text file',\
		OFN_EXPLORER or OFN_OVERWRITEPROMPT or OFN_PATHMUSTEXIST
	invoke	GetSaveFileName,addr ofn
	ret
endp
