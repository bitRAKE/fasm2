; ui_main.inc - menu construction, command routing, and the window procedure.

proc BuildMainMenu
	invoke	CreateMenu
	mov	[hMenubar],rax
	test	rax,rax
	jz	.fail
	invoke	CreatePopupMenu
	mov	[hFileMenu],rax
	invoke	CreatePopupMenu
	mov	[hRecentMenu],rax
	invoke	CreatePopupMenu
	mov	[hEditMenu],rax
	invoke	CreatePopupMenu
	mov	[hHelpMenu],rax

	invoke	AppendMenu,[hFileMenu],MF_STRING,ID_FILE_NEW,'&New'
	invoke	AppendMenu,[hFileMenu],MF_STRING,ID_FILE_OPEN,'&Open...'
	invoke	AppendMenu,[hFileMenu],MF_STRING,ID_FILE_SAVE,'&Save'
	invoke	AppendMenu,[hFileMenu],MF_STRING,ID_FILE_SAVE_AS,'Save &As...'
	invoke	AppendMenu,[hFileMenu],MF_SEPARATOR,0,0
	invoke	AppendMenu,[hFileMenu],MF_POPUP,[hRecentMenu],'Recent &Files'
	invoke	AppendMenu,[hFileMenu],MF_SEPARATOR,0,0
	invoke	AppendMenu,[hFileMenu],MF_STRING,ID_FILE_EXIT,'E&xit'

	invoke	AppendMenu,[hEditMenu],MF_STRING,ID_EDIT_SELECT_ALL,'Select &All'
	invoke	AppendMenu,[hHelpMenu],MF_STRING,ID_HELP_ABOUT,'&About'

	invoke	AppendMenu,[hMenubar],MF_POPUP,[hFileMenu],'&File'
	invoke	AppendMenu,[hMenubar],MF_POPUP,[hEditMenu],'&Edit'
	invoke	AppendMenu,[hMenubar],MF_POPUP,[hHelpMenu],'&Help'

	fastcall PopulateRecentMenu
	mov	rax,[hMenubar]
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc UpdateTitle
    locals
	namep dq ?
	markp dq ?
    endl

	fastcall wcs_is_empty,current_path
	test	eax,eax
	jz	.have_path
	mov	qword [namep],untitled_name
	jmp	.mark
  .have_path:
	mov	qword [namep],current_path
  .mark:
	mov	qword [markp],0
	cmp	dword [dirty],0
	je	.format
	mov	qword [markp],dirty_suffix
  .format:
	invoke	wsprintf,title_buffer,title_format,[namep],[markp]
	cmp	qword [hMain],0
	je	.done
	invoke	SetWindowText,[hMain],title_buffer
  .done:
	xor	eax,eax
	ret
endp

proc MarkDirty
	cmp	dword [suppress_change],0
	jne	.done
	cmp	dword [dirty],0
	jne	.done
	mov	dword [dirty],1
	fastcall UpdateTitle
  .done:
	xor	eax,eax
	ret
endp

proc MaybeSaveDirty
	cmp	dword [dirty],0
	je	.ok
	invoke	MessageBox,[hMain],'Save changes first?',app_name,\
		MB_YESNOCANCEL or MB_ICONQUESTION
	cmp	eax,IDCANCEL
	je	.cancel
	cmp	eax,IDNO
	je	.ok
	fastcall CommandSave
	test	eax,eax
	jz	.cancel
  .ok:
	mov	eax,1
	ret
  .cancel:
	xor	eax,eax
	ret
endp

proc CommandNew
	fastcall MaybeSaveDirty
	test	eax,eax
	jz	.done
	mov	dword [suppress_change],1
	invoke	SetWindowText,[hEdit],''
	mov	dword [suppress_change],0
	fastcall ClearCurrentPath
	mov	dword [dirty],0
	fastcall UpdateTitle
  .done:
	ret
endp

proc CommandOpen
	fastcall MaybeSaveDirty
	test	eax,eax
	jz	.done
	fastcall ChooseOpenPath
	test	eax,eax
	jz	.done
	fastcall OpenPathIntoEditor,dialog_path
  .done:
	ret
endp

proc CommandSave
	fastcall wcs_is_empty,current_path
	test	eax,eax
	jnz	.save_as
	fastcall SaveEditorToPath,current_path
	ret
  .save_as:
	fastcall CommandSaveAs
	ret
endp

proc CommandSaveAs
	fastcall ChooseSavePath
	test	eax,eax
	jz	.cancel
	fastcall SaveEditorToPath,dialog_path
	ret
  .cancel:
	xor	eax,eax
	ret
endp

proc ResizeEditor
	invoke	GetClientRect,[hMain],addr client_rect
	invoke	MoveWindow,[hEdit],0,0,\
		dword [client_rect.right],dword [client_rect.bottom],1
	ret
endp

proc WindowProc uses rbx, hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_CREATE
	je	.wm_create
	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_SETFOCUS
	je	.wm_setfocus
	cmp	edx,WM_COMMAND
	je	.wm_command
	cmp	edx,WM_CLOSE
	je	.wm_close
	cmp	edx,WM_DESTROY
	je	.wm_destroy

	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_create:
	invoke	CreateWindowEx,WS_EX_CLIENTEDGE,'EDIT','',\
		WS_CHILD or WS_VISIBLE or WS_VSCROLL or WS_HSCROLL or\
		ES_MULTILINE or ES_AUTOVSCROLL or ES_AUTOHSCROLL or ES_WANTRETURN,\
		0,0,0,0,[hwnd],ID_EDIT_CONTROL,[hInstance],0
	mov	[hEdit],rax
	fastcall PopulateRecentMenu
	fastcall UpdateTitle
	xor	eax,eax
	ret

  .wm_size:
	fastcall ResizeEditor
	xor	eax,eax
	ret

  .wm_setfocus:
	invoke	SetFocus,[hEdit]
	xor	eax,eax
	ret

  .wm_command:
	mov	eax,dword [wparam]
	movzx	ecx,ax
	mov	edx,dword [wparam]
	shr	edx,16
	cmp	rcx,ID_EDIT_CONTROL
	jne	.menu_command
	cmp	edx,EN_CHANGE
	jne	.done_zero
	fastcall MarkDirty
	jmp	.done_zero

  .menu_command:
	cmp	ecx,ID_FILE_NEW
	je	.cmd_new
	cmp	ecx,ID_FILE_OPEN
	je	.cmd_open
	cmp	ecx,ID_FILE_SAVE
	je	.cmd_save
	cmp	ecx,ID_FILE_SAVE_AS
	je	.cmd_save_as
	cmp	ecx,ID_FILE_EXIT
	je	.cmd_exit
	cmp	ecx,ID_EDIT_SELECT_ALL
	je	.cmd_select_all
	cmp	ecx,ID_HELP_ABOUT
	je	.cmd_about
	cmp	ecx,ID_RECENT_BASE
	jb	.done_zero
	cmp	ecx,ID_RECENT_END
	ja	.done_zero
	mov	ebx,ecx
	fastcall MaybeSaveDirty
	test	eax,eax
	jz	.done_zero
	mov	ecx,ebx
	fastcall OpenRecentById,rcx
	jmp	.done_zero

  .cmd_new:
	fastcall CommandNew
	jmp	.done_zero
  .cmd_open:
	fastcall CommandOpen
	jmp	.done_zero
  .cmd_save:
	fastcall CommandSave
	jmp	.done_zero
  .cmd_save_as:
	fastcall CommandSaveAs
	jmp	.done_zero
  .cmd_exit:
	invoke	SendMessage,[hwnd],WM_CLOSE,0,0
	jmp	.done_zero
  .cmd_select_all:
	invoke	SendMessage,[hEdit],EM_SETSEL,0,-1
	jmp	.done_zero
  .cmd_about:
	invoke	MessageBox,[hwnd],'A modular fasm2 Win64 example with local includes, UTF-8 file I/O, and a comctl32 binary MRU.',app_name,MB_ICONINFORMATION
	jmp	.done_zero

  .wm_close:
	fastcall MaybeSaveDirty
	test	eax,eax
	jz	.done_zero
	fastcall SaveCurrentToRecent
	invoke	DestroyWindow,[hwnd]
	jmp	.done_zero

  .wm_destroy:
	fastcall SaveCurrentToRecent
	invoke	PostQuitMessage,0
  .done_zero:
	xor	eax,eax
	ret
endp
