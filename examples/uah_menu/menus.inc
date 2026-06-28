proc UpdatePopupMenuState menup
    locals
	flags dd ?
    endl
	mov	[menup],rcx
	mov	eax,MF_BYCOMMAND or MF_UNCHECKED
	cmp	dword [dark_mode],0
	je	.have_theme_flags
	mov	eax,MF_BYCOMMAND or MF_CHECKED
  .have_theme_flags:
	mov	[flags],eax
	invoke	CheckMenuItem,[menup],ID_THEME_TOGGLE,dword [flags]
	ret
endp

proc EnableMenuByBool menup,item_id,enabled
    locals
	flags dd ?
    endl
	mov	[menup],rcx
	mov	dword [item_id],edx
	mov	dword [enabled],r8d
	mov	eax,MF_BYCOMMAND or MF_GRAYED
	cmp	dword [enabled],0
	je	.ready
	mov	eax,MF_BYCOMMAND or MF_ENABLED
  .ready:
	mov	[flags],eax
	invoke	EnableMenuItem,[menup],dword [item_id],dword [flags]
	ret
endp

proc UpdateEditMenuState menup
    locals
	sel CHARRANGE
	selection dd ?
    endl
	mov	[menup],rcx
	fastcall UpdatePopupMenuState,[menup]
	cmp	qword [hRich],0
	je	.disabled

	mov	[sel.cpMin],0
	mov	[sel.cpMax],0
	invoke	SendMessage,[hRich],EM_EXGETSEL,0,addr sel
	mov	eax,[sel.cpMin]
	cmp	eax,[sel.cpMax]
	setne	al
	movzx	eax,al
	mov	[selection],eax

	invoke	SendMessage,[hRich],EM_CANUNDO,0,0
	fastcall EnableMenuByBool,[menup],ID_EDIT_UNDO,eax
	invoke	SendMessage,[hRich],EM_CANREDO,0,0
	fastcall EnableMenuByBool,[menup],ID_EDIT_REDO,eax
	fastcall EnableMenuByBool,[menup],ID_EDIT_CUT,dword [selection]
	fastcall EnableMenuByBool,[menup],ID_EDIT_COPY,dword [selection]
	fastcall EnableMenuByBool,[menup],ID_EDIT_DELETE,dword [selection]
	invoke	SendMessage,[hRich],EM_CANPASTE,0,0
	fastcall EnableMenuByBool,[menup],ID_EDIT_PASTE,eax
	ret

  .disabled:
	iterate id,ID_EDIT_UNDO,ID_EDIT_REDO,ID_EDIT_CUT,ID_EDIT_COPY,\
		ID_EDIT_PASTE,ID_EDIT_DELETE,ID_EDIT_SELECT_ALL,ID_EDIT_INSERT_LINK,\
		ID_EDIT_CLEAR
		fastcall EnableMenuByBool,[menup],id,0
	end iterate
	ret
endp

proc LoadPopupMenuResource resource_id,bar_out
	mov	dword [resource_id],ecx
	mov	[bar_out],rdx
	invoke	LoadMenu,[hInstance],dword [resource_id]
	mov	rcx,[bar_out]
	mov	[rcx],rax
	test	rax,rax
	jz	.fail
	invoke	GetSubMenu,rax,0
	test	rax,rax
	jnz	.done
	mov	rcx,[bar_out]
	invoke	DestroyMenu,[rcx]
	mov	rcx,[bar_out]
	mov	qword [rcx],0
  .fail:
	xor	eax,eax
  .done:
	ret
endp

proc ShowTrayMenu x,y
    locals
	bar dq ?
	popup dq ?
    endl
	mov	dword [x],ecx
	mov	dword [y],edx
	fastcall LoadPopupMenuResource,IDR_TRAY_MENU,addr bar
	mov	[popup],rax
	test	rax,rax
	jz	.done
	fastcall UpdatePopupMenuState,[popup]
	invoke	SetForegroundWindow,[hMain]
	invoke	TrackPopupMenuEx,[popup],TPM_RIGHTBUTTON or TPM_RIGHTALIGN or TPM_BOTTOMALIGN,\
		dword [x],dword [y],[hMain],0
	invoke	PostMessage,[hMain],WM_NULL,0,0
	invoke	DestroyMenu,[bar]
  .done:
	ret
endp

proc ShowEditContextMenu x,y
    locals
	bar dq ?
	popup dq ?
	pt POINT
    endl
	mov	dword [x],ecx
	mov	dword [y],edx
	cmp	dword [x],-1
	jne	.load
	cmp	dword [y],-1
	jne	.load
	invoke	GetCursorPos,addr pt
	mov	eax,[pt.x]
	mov	dword [x],eax
	mov	eax,[pt.y]
	mov	dword [y],eax

  .load:
	fastcall LoadPopupMenuResource,IDR_EDIT_MENU,addr bar
	mov	[popup],rax
	test	rax,rax
	jz	.done
	fastcall UpdateEditMenuState,[popup]
	invoke	SetForegroundWindow,[hMain]
	invoke	TrackPopupMenuEx,[popup],TPM_RIGHTBUTTON,dword [x],dword [y],[hMain],0
	invoke	DestroyMenu,[bar]
  .done:
	ret
endp
