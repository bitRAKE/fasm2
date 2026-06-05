; clear_edit.inc - EDIT subclass with a right-edge font-icon clear adornment.
;
; Host requirements:
;   define __GLOBAL_DATA__ clear_edit_data
;   define __GLOBAL_BSS__  clear_edit_bss
;   call ClearEdit_Register during startup
;   call ClearEdit_Attach for each single-line EDIT
;   send ECM_TEXTCHANGED to the edit with wParam = text-present flag

ECM_TEXTCHANGED		= WM_APP + 1
CLEAR_EDIT_SUBCLASS_ID	= 1
CLEAR_EDIT_PAD		= 2
CLEAR_EDIT_HIT		= 18
CLEAR_EDIT_ICON_PX	= 14
CLEAR_EDIT_MASK		= 00808080h
CLEAR_EDIT_MASK_HOVER	= 000000FFh
CLEAR_EDIT_BADGE	= 00000000h
CLEAR_EDIT_RING		= 00F7F2EEh
CLEAR_EDIT_DPI_BASE	= 96
CLEAR_EDIT_QUALITY	= 5
ECLEAR_STATUSERRORFULL	= 0EB90h
TME_LEAVE		= 00000002h
HTCLIENT		= 1

struct TRACKMOUSEEVENT
  cbSize      dd ?
  dwFlags     dd ?
  hwndTrack   dq ?
  dwHoverTime dd ?
	      dd ?
ends

struct ECLEAR
  pad	     dd ?
  hit	     dd ?
  icon_px    dd ?
  mask_color dd ?
  hover      dd ?
  tracking   dd ?
  has_text   dd ?
	     dd ?
  font	     dq ?
ends

macro clear_edit_data
	clear_edit_face GLOBWSTR 'Segoe MDL2 Assets',0
	clear_edit_empty_text dw 0
purge clear_edit_data
end macro

macro clear_edit_bss
	align 8
	clear_edit_arrow_cursor dq ?
purge clear_edit_bss
end macro

proc ClearEdit_Register hinst
	mov	[hinst],rcx
	invoke	LoadCursorW,0,IDC_ARROW
	mov	[clear_edit_arrow_cursor],rax
	test	eax,eax
	jz	.fail
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc ClearEdit_Attach uses rbx, hwnd,has_text
    locals
	rc RECT
	procp dq ?
	heap_handle dq ?
	state dq ?
    endl

	mov	[hwnd],rcx
	mov	dword [has_text],edx
	invoke	GetProcessHeap
	mov	[heap_handle],rax
	invoke	HeapAlloc,[heap_handle],8,sizeof.ECLEAR
	mov	[state],rax
	test	rax,rax
	jz	.fail
	mov	rbx,rax
	mov	dword [rbx+ECLEAR.pad],CLEAR_EDIT_PAD
	mov	dword [rbx+ECLEAR.hit],CLEAR_EDIT_HIT
	mov	dword [rbx+ECLEAR.icon_px],CLEAR_EDIT_ICON_PX
	mov	dword [rbx+ECLEAR.mask_color],CLEAR_EDIT_MASK
	mov	eax,dword [has_text]
	mov	dword [rbx+ECLEAR.has_text],eax

	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,dword [rc.bottom]
	sub	eax,dword [rc.top]
	sub	eax,2
	cmp	eax,CLEAR_EDIT_ICON_PX
	jge	.icon_max_ready
	mov	dword [rbx+ECLEAR.icon_px],eax
  .icon_max_ready:
	cmp	dword [rbx+ECLEAR.icon_px],8
	jge	.icon_min_ready
	mov	dword [rbx+ECLEAR.icon_px],8
  .icon_min_ready:
	invoke	GetDpiForWindow,[hwnd]
	test	eax,eax
	jnz	.have_dpi
	mov	eax,CLEAR_EDIT_DPI_BASE
  .have_dpi:
	invoke	MulDiv,dword [rbx+ECLEAR.icon_px],eax,CLEAR_EDIT_DPI_BASE
	neg	eax
	invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		CLEAR_EDIT_QUALITY,DEFAULT_PITCH or FF_DONTCARE,addr clear_edit_face
	mov	[rbx+ECLEAR.font],rax

	lea	rax,[ClearEditProc]
	mov	[procp],rax
	invoke	SetWindowSubclass,[hwnd],[procp],CLEAR_EDIT_SUBCLASS_ID,[state]
	test	eax,eax
	jz	.cleanup
	fastcall ClearEdit_Invalidate,[hwnd],[state]
	mov	eax,1
	ret

  .cleanup:
	cmp	qword [rbx+ECLEAR.font],0
	je	.no_font
	invoke	DeleteObject,[rbx+ECLEAR.font]
  .no_font:
	invoke	HeapFree,[heap_handle],0,[state]
  .fail:
	xor	eax,eax
	ret
endp

proc ClearEdit_TextChanged hwnd,has_text
	mov	[hwnd],rcx
	mov	dword [has_text],edx
	invoke	SendMessageW,[hwnd],ECM_TEXTCHANGED,dword [has_text],0
	ret
endp

proc ClearEdit_GlyphRect uses rbx, hwnd,statep,rectp
    locals
	cr RECT
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	[rectp],r8
	invoke	GetClientRect,[hwnd],addr cr
	mov	rbx,[statep]
	mov	rdx,[rectp]
	mov	eax,dword [cr.right]
	sub	eax,dword [rbx+ECLEAR.pad]
	mov	dword [rdx+RECT.right],eax
	sub	eax,dword [rbx+ECLEAR.hit]
	mov	dword [rdx+RECT.left],eax
	mov	eax,dword [cr.top]
	mov	dword [rdx+RECT.top],eax
	mov	eax,dword [cr.bottom]
	mov	dword [rdx+RECT.bottom],eax
	ret
endp

proc ClearEdit_Invalidate hwnd,statep
    locals
	rc RECT
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	fastcall ClearEdit_GlyphRect,[hwnd],[statep],addr rc
	invoke	InvalidateRect,[hwnd],addr rc,1
	ret
endp

proc ClearEdit_HitTest hwnd,statep,x,y
    locals
	rc RECT
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	dword [x],r8d
	mov	dword [y],r9d
	mov	rax,[statep]
	cmp	dword [rax+ECLEAR.has_text],0
	je	.no
	fastcall ClearEdit_GlyphRect,[hwnd],[statep],addr rc
	mov	eax,dword [x]
	cmp	eax,dword [rc.left]
	jl	.no
	cmp	eax,dword [rc.right]
	jge	.no
	mov	eax,dword [y]
	cmp	eax,dword [rc.top]
	jl	.no
	cmp	eax,dword [rc.bottom]
	jge	.no
	mov	eax,1
	ret
  .no:
	xor	eax,eax
	ret
endp

proc ClearEdit_CursorOver hwnd,statep
    locals
	pt POINT
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	invoke	GetCursorPos,addr pt
	test	eax,eax
	jz	.no
	invoke	ScreenToClient,[hwnd],addr pt
	test	eax,eax
	jz	.no
	fastcall ClearEdit_HitTest,[hwnd],[statep],dword [pt.x],dword [pt.y]
	ret
  .no:
	xor	eax,eax
	ret
endp

proc ClearEdit_TrackLeave hwnd,statep
    locals
	tme TRACKMOUSEEVENT
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	rax,[statep]
	cmp	dword [rax+ECLEAR.tracking],0
	jne	.done
	mov	dword [tme.cbSize],sizeof.TRACKMOUSEEVENT
	mov	dword [tme.dwFlags],TME_LEAVE
	mov	rax,[hwnd]
	mov	[tme.hwndTrack],rax
	mov	dword [tme.dwHoverTime],0
	invoke	TrackMouseEvent,addr tme
	mov	rax,[statep]
	mov	dword [rax+ECLEAR.tracking],1
  .done:
	ret
endp

proc ClearEdit_DrawLayer hwnd,statep,hdc,rectp,glyph,color
    locals
	ch dw ?,0
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	[hdc],r8
	mov	[rectp],r9
	movzx	eax,word [glyph]
	mov	word [ch],ax
	invoke	SetTextColor,[hdc],dword [color]
	invoke	DrawTextW,[hdc],addr ch,1,[rectp],\
		DT_SINGLELINE or DT_CENTER or DT_VCENTER or DT_NOPREFIX or DT_NOCLIP
	ret
endp

proc ClearEdit_DrawGlyph uses rbx, hwnd,statep,hdc
    locals
	rc RECT
	old_font dq ?
	old_bk dd ?
	mask_color dd ?
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	[hdc],r8
	mov	rbx,[statep]
	cmp	dword [rbx+ECLEAR.has_text],0
	je	.done
	cmp	qword [rbx+ECLEAR.font],0
	je	.done
	fastcall ClearEdit_GlyphRect,[hwnd],[statep],addr rc
	invoke	SelectObject,[hdc],[rbx+ECLEAR.font]
	mov	[old_font],rax
	invoke	SetBkMode,[hdc],TRANSPARENT
	mov	dword [old_bk],eax
	mov	eax,dword [rbx+ECLEAR.mask_color]
	cmp	dword [rbx+ECLEAR.hover],0
	je	.mask_ready
	mov	eax,CLEAR_EDIT_MASK_HOVER
  .mask_ready:
	mov	[mask_color],eax
	fastcall ClearEdit_DrawLayer,[hwnd],[statep],[hdc],addr rc,ECLEAR_STATUSERRORFULL,[mask_color]
	invoke	SetBkMode,[hdc],dword [old_bk]
	invoke	SelectObject,[hdc],[old_font]
  .done:
	ret
endp

proc ClearEdit_Clear hwnd,statep
    locals
	parent dq ?
	cmd dd ?
    endl

	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	rax,[statep]
	mov	dword [rax+ECLEAR.has_text],0
	mov	dword [rax+ECLEAR.hover],0
	invoke	SetWindowTextW,[hwnd],addr clear_edit_empty_text
	invoke	GetParent,[hwnd]
	mov	[parent],rax
	test	rax,rax
	jz	.redraw
	invoke	GetDlgCtrlID,[hwnd]
	mov	ecx,eax
	mov	eax,EN_CHANGE
	shl	eax,16
	or	eax,ecx
	mov	dword [cmd],eax
	invoke	SendMessageW,[parent],WM_COMMAND,dword [cmd],[hwnd]
  .redraw:
	fastcall ClearEdit_Invalidate,[hwnd],[statep]
	ret
endp

proc ClearEditProc uses rbx, hwnd,wmsg,wparam,lparam,subclass_id,refdata
    locals
	result dq ?
	hdc dq ?
	x dd ?
	y dd ?
	over dd ?
	procp dq ?
	heap_handle dq ?
    endl

	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9
	mov	rbx,[refdata]

	cmp	edx,WM_PAINT
	je	.wm_paint
	cmp	edx,ECM_TEXTCHANGED
	je	.ecm_textchanged
	cmp	edx,WM_MOUSEMOVE
	je	.wm_mousemove
	cmp	edx,WM_MOUSELEAVE
	je	.wm_mouseleave
	cmp	edx,WM_SETCURSOR
	je	.wm_setcursor
	cmp	edx,WM_LBUTTONDOWN
	je	.wm_lbuttondown
	cmp	edx,WM_NCDESTROY
	je	.wm_ncdestroy
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_paint:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	mov	[result],rax
	invoke	GetDC,[hwnd]
	mov	[hdc],rax
	test	rax,rax
	jz	.paint_done
	fastcall ClearEdit_DrawGlyph,[hwnd],rbx,[hdc]
	invoke	ReleaseDC,[hwnd],[hdc]
  .paint_done:
	mov	rax,[result]
	ret

  .ecm_textchanged:
	xor	eax,eax
	cmp	qword [wparam],0
	setne	al
	mov	dword [rbx+ECLEAR.has_text],eax
	test	eax,eax
	jnz	.text_state_ready
	mov	dword [rbx+ECLEAR.hover],0
  .text_state_ready:
	fastcall ClearEdit_Invalidate,[hwnd],rbx
	xor	eax,eax
	ret

  .wm_mousemove:
	movsx	eax,word [lparam]
	mov	dword [x],eax
	movsx	eax,word [lparam+2]
	mov	dword [y],eax
	fastcall ClearEdit_HitTest,[hwnd],rbx,dword [x],dword [y]
	mov	dword [over],eax
	cmp	eax,dword [rbx+ECLEAR.hover]
	je	.track
	mov	dword [rbx+ECLEAR.hover],eax
	fastcall ClearEdit_Invalidate,[hwnd],rbx
  .track:
	cmp	dword [over],0
	je	.mouse_default
	fastcall ClearEdit_TrackLeave,[hwnd],rbx
  .mouse_default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_mouseleave:
	mov	dword [rbx+ECLEAR.tracking],0
	cmp	dword [rbx+ECLEAR.hover],0
	je	.leave_default
	mov	dword [rbx+ECLEAR.hover],0
	fastcall ClearEdit_Invalidate,[hwnd],rbx
  .leave_default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_setcursor:
	mov	eax,dword [lparam]
	and	eax,0FFFFh
	cmp	eax,HTCLIENT
	jne	.cursor_default
	fastcall ClearEdit_CursorOver,[hwnd],rbx
	test	eax,eax
	jz	.cursor_default
	invoke	SetCursor,[clear_edit_arrow_cursor]
	mov	eax,1
	ret
  .cursor_default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_lbuttondown:
	movsx	eax,word [lparam]
	mov	dword [x],eax
	movsx	eax,word [lparam+2]
	mov	dword [y],eax
	fastcall ClearEdit_HitTest,[hwnd],rbx,dword [x],dword [y]
	test	eax,eax
	jz	.button_default
	fastcall ClearEdit_Clear,[hwnd],rbx
	xor	eax,eax
	ret
  .button_default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_ncdestroy:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	mov	[result],rax
	lea	rax,[ClearEditProc]
	mov	[procp],rax
	invoke	RemoveWindowSubclass,[hwnd],[procp],[subclass_id]
	cmp	qword [rbx+ECLEAR.font],0
	je	.no_font
	invoke	DeleteObject,[rbx+ECLEAR.font]
  .no_font:
	invoke	GetProcessHeap
	mov	[heap_handle],rax
	invoke	HeapFree,[heap_handle],0,rbx
	mov	rax,[result]
	ret
endp
