; clear_edit.inc - EDIT subclass with a right-edge font-icon clear adornment.
;
; Host requirements:
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
  hover      dd ?
  tracking   dd ?
  has_text   dd ?
	     align 8
  font	     dq ?
ends

macro clear_edit_bss
	align 8
	clear_edit_arrow_cursor dq ?
	clear_edit_empty_text dw 0
purge clear_edit_bss
end macro
define __GLOBAL_BSS__  clear_edit_bss

proc ClearEdit_Register
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
    endl
	mov	[hwnd],rcx
	mov	dword [has_text],edx
	invoke	GetProcessHeap
	mov	[heap_handle],rax
	invoke	HeapAlloc,[heap_handle],8,sizeof.ECLEAR
	test	rax,rax
	jz	fail
	mov	rbx,rax
	mov	[rbx+ECLEAR.pad],CLEAR_EDIT_PAD
	mov	[rbx+ECLEAR.hit],CLEAR_EDIT_HIT
	mov	[rbx+ECLEAR.icon_px],CLEAR_EDIT_ICON_PX
	mov	eax,dword [has_text]
	mov	[rbx+ECLEAR.has_text],eax

	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,[rc.bottom]
	sub	eax,[rc.top]
	sub	eax,2
	cmp	eax,CLEAR_EDIT_ICON_PX
	jge	icon_max_ready
	mov	[rbx+ECLEAR.icon_px],eax
  icon_max_ready:
	cmp	[rbx+ECLEAR.icon_px],8
	jge	icon_min_ready
	mov	[rbx+ECLEAR.icon_px],8
  icon_min_ready:
	invoke	GetDpiForWindow,[hwnd]
	test	eax,eax
	jnz	have_dpi
	mov	eax,CLEAR_EDIT_DPI_BASE
  have_dpi:
	invoke	MulDiv,[rbx+ECLEAR.icon_px],eax,CLEAR_EDIT_DPI_BASE
	neg	eax
	invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		CLEAR_EDIT_QUALITY,DEFAULT_PITCH or FF_DONTCARE,'Segoe MDL2 Assets'
	mov	[rbx+ECLEAR.font],rax

	lea	rax,[ClearEditProc]
	mov	[procp],rax
	invoke	SetWindowSubclass,[hwnd],[procp],CLEAR_EDIT_SUBCLASS_ID,rbx
	test	eax,eax
	jz	cleanup
	fastcall ClearEdit_Invalidate,[hwnd],rbx
	mov	eax,1
	ret

  cleanup:
	mov	rcx,[rbx+ECLEAR.font]
	jrcxz	no_font
	invoke	DeleteObject,rcx
  no_font:
	invoke	HeapFree,[heap_handle],0,rbx
  fail:
	xor	eax,eax
	ret
endp

proc ClearEdit_GlyphRect hwnd,statep,rectp
    locals
	cr RECT
    endl
	mov	[statep],rdx
	mov	[rectp],r8
	invoke	GetClientRect,rcx,addr cr
	mov	rcx,[statep]
	mov	rdx,[rectp]
	mov	eax,[cr.right]
	sub	eax,[rcx+ECLEAR.pad]
	mov	[rdx+RECT.right],eax
	sub	eax,[rcx+ECLEAR.hit]
	mov	[rdx+RECT.left],eax
	mov	eax,[cr.top]
	mov	[rdx+RECT.top],eax
	mov	eax,[cr.bottom]
	mov	[rdx+RECT.bottom],eax
	ret
endp

proc ClearEdit_Invalidate hwnd,statep
    locals
	rc RECT
    endl
	mov	[hwnd],rcx
	fastcall ClearEdit_GlyphRect,rcx,rdx,addr rc
	invoke	InvalidateRect,[hwnd],addr rc,1
	ret
endp

proc ClearEdit_HitTest hwnd,statep,x,y
    locals
	rc RECT
    endl
	cmp	[rdx+ECLEAR.has_text],0
	je	no
	mov	dword [x],r8d
	mov	dword [y],r9d
	fastcall ClearEdit_GlyphRect,rcx,rdx,addr rc
	mov	eax,dword [x]
	cmp	eax,[rc.left]
	jl	no
	cmp	eax,[rc.right]
	jge	no
	mov	eax,dword [y]
	cmp	eax,[rc.top]
	jl	no
	cmp	eax,[rc.bottom]
	jge	no
	mov	eax,1
	ret
  no:
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
	jz	no
	invoke	ScreenToClient,[hwnd],addr pt
	test	eax,eax
	jz	no
	fastcall ClearEdit_HitTest,[hwnd],[statep],[pt.x],[pt.y]
	ret
  no:
	xor	eax,eax
	ret
endp

proc ClearEdit_TrackLeave hwnd,statep
    locals
	tme TRACKMOUSEEVENT
    endl
	cmp	[rdx+ECLEAR.tracking],0
	jne	done
	mov	[statep],rdx
	mov	[tme.cbSize],sizeof.TRACKMOUSEEVENT
	mov	[tme.dwFlags],TME_LEAVE
	mov	[tme.hwndTrack],rcx
	mov	[tme.dwHoverTime],0
	invoke	TrackMouseEvent,addr tme
	test	eax,eax
	jz	done
	mov	rax,[statep]
	mov	[rax+ECLEAR.tracking],1
  done:
	ret
endp

proc ClearEdit_DrawGlyph uses rbx rsi, hwnd,statep,hdc
    locals
	rc RECT
	old_font dq ?
	old_bk dd ?
	glyph_text dw ECLEAR_STATUSERRORFULL,0
    endl
	cmp	[rdx+ECLEAR.has_text],0
	je	done
	cmp	[rdx+ECLEAR.font],0
	je	done
	mov	rsi,r8
	mov	rbx,rdx
	fastcall ClearEdit_GlyphRect,rcx,rdx,addr rc
	invoke	SelectObject,rsi,[rbx+ECLEAR.font]
	mov	[old_font],rax
	invoke	SetBkMode,rsi,TRANSPARENT
	mov	[old_bk],eax
	mov	edx,CLEAR_EDIT_MASK
	cmp	[rbx+ECLEAR.hover],0
	je	mask_ready
	mov	edx,CLEAR_EDIT_MASK_HOVER
  mask_ready:
	invoke	SetTextColor,rsi,edx
	invoke	DrawTextW,rsi,addr glyph_text,1,addr rc,\
		DT_SINGLELINE or DT_CENTER or DT_VCENTER or DT_NOPREFIX or DT_NOCLIP
	invoke	SetBkMode,rsi,[old_bk]
	invoke	SelectObject,rsi,[old_font]
  done:
	ret
endp

proc ClearEdit_Clear hwnd,statep
    locals
	parent dq ?
    endl
	mov	[hwnd],rcx
	mov	[statep],rdx
	mov	rax,[statep]
	mov	[rax+ECLEAR.has_text],0
	mov	[rax+ECLEAR.hover],0
	invoke	SetWindowTextW,rcx,addr clear_edit_empty_text
	invoke	GetParent,[hwnd]
	mov	[parent],rax
	test	rax,rax
	jz	redraw
	invoke	GetDlgCtrlID,[hwnd]
	mov	r8d,EN_CHANGE
	shl	r8d,16
	or	r8d,eax
	mov	r9,[hwnd]
	invoke	SendMessageW,[parent],WM_COMMAND,r8,r9
  redraw:
	fastcall ClearEdit_Invalidate,[hwnd],[statep]
	ret
endp

proc ClearEditProc uses rbx, hwnd,wmsg,wparam,lparam,subclass_id,refdata
    locals
	result dq ?
	hdc dq ?
	procp dq ?
	heap_handle dq ?
	x dd ?
	y dd ?
	over dd ?
    endl
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9
	mov	rbx,[refdata]

	iterate <message,branch>,\
		WM_PAINT,	wm_paint,\
		ECM_TEXTCHANGED,ecm_textchanged,\
		WM_MOUSEMOVE,	wm_mousemove,\
		WM_MOUSELEAVE,	wm_mouseleave,\
		WM_SETCURSOR,	wm_setcursor,\
		WM_LBUTTONDOWN,	wm_lbuttondown,\
		WM_NCDESTROY,	wm_ncdestroy

		cmp edx, message
		jz branch
	end iterate
  default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  wm_paint:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	mov	[result],rax
	invoke	GetDC,[hwnd]
	mov	[hdc],rax
	test	rax,rax
	jz	paint_done
	fastcall ClearEdit_DrawGlyph,[hwnd],rbx,[hdc]
	invoke	ReleaseDC,[hwnd],[hdc]
  paint_done:
	mov	rax,[result]
	ret

  ecm_textchanged:
	xor	eax,eax
	cmp	qword [wparam],0
	setne	al
	mov	dword [rbx+ECLEAR.has_text],eax
	test	eax,eax
	jnz	text_state_ready
	mov	dword [rbx+ECLEAR.hover],0
  text_state_ready:
	fastcall ClearEdit_Invalidate,[hwnd],rbx
	xor	eax,eax
	ret

  wm_mousemove:
	movsx	eax,word [lparam]
	mov	[x],eax
	movsx	eax,word [lparam+2]
	mov	[y],eax
	fastcall ClearEdit_HitTest,[hwnd],rbx,[x],[y]
	mov	[over],eax
	cmp	eax,[rbx+ECLEAR.hover]
	je	track
	mov	[rbx+ECLEAR.hover],eax
	fastcall ClearEdit_Invalidate,[hwnd],rbx
  track:
	cmp	[over],0
	je	default
	fastcall ClearEdit_TrackLeave,[hwnd],rbx
	jmp	default

  wm_mouseleave:
	mov	[rbx+ECLEAR.tracking],0
	cmp	[rbx+ECLEAR.hover],0
	je	default
	mov	[rbx+ECLEAR.hover],0
	fastcall ClearEdit_Invalidate,[hwnd],rbx
	jmp	default

  wm_setcursor:
	mov	eax,dword [lparam]
	and	eax,0FFFFh
	cmp	eax,HTCLIENT
	jne	default
	fastcall ClearEdit_CursorOver,[hwnd],rbx
	test	eax,eax
	jz	default
	invoke	SetCursor,[clear_edit_arrow_cursor]
	mov	eax,1
	ret

  wm_lbuttondown:
	movsx	eax,word [lparam]
	mov	[x],eax
	movsx	eax,word [lparam+2]
	mov	[y],eax
	fastcall ClearEdit_HitTest,[hwnd],rbx,[x],[y]
	test	eax,eax
	jz	default
	fastcall ClearEdit_Clear,[hwnd],rbx
	xor	eax,eax
	ret

  wm_ncdestroy:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	mov	[result],rax
	lea	rax,[ClearEditProc]
	mov	[procp],rax
	invoke	RemoveWindowSubclass,[hwnd],[procp],[subclass_id]
	mov	rcx, [rbx+ECLEAR.font]
	jrcxz	no_font
	invoke	DeleteObject,rcx
  no_font:
	invoke	GetProcessHeap
	mov	[heap_handle],rax
	invoke	HeapFree,[heap_handle],0,rbx
	mov	rax,[result]
	ret
endp
