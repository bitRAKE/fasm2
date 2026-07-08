; addon/csd/caption.inc - shared caption-control table support.
;
; Examples keep their policy local, but the row split is reusable:
; descriptors are immutable, geometry is recomputed, and state is mutable.

CSD_DPI_BASE			= 96

CSD_CAPTION_ALIGN_LEADING	= 0001h
CSD_CAPTION_ALIGN_TRAILING	= 0002h
CSD_CAPTION_APP		= 0004h
CSD_CAPTION_DANGER		= 0008h
CSD_CAPTION_SYSTEM		= 0010h
CSD_CAPTION_SYSCMD		= 0020h
CSD_CAPTION_CHILD		= 0040h

CSD_STATE_NORMAL		= 0
CSD_STATE_HOVER		= 1
CSD_STATE_PRESSED		= 2
CSD_STATE_DISABLED		= 3
CSD_STATE_INACTIVE		= 4

CSD_CAPTION_NO_INDEX		= -1
CSD_CAPTION_ANIM_ACTIVE		= 0001h

struct CSD_CAPTION_DESCRIPTOR
  id		dd ?
  glyph		dd ?
  command	dd ?
  hitCode	dd ?
  flags		dd ?
  widthDip	dd ?
ends

struct CSD_CAPTION_GEOMETRY
  rect		RECT
ends

struct CSD_CAPTION_STATE
  value		db ?
  fadePhase	db ?
  fromValue	db ?
  animFlags	db ?
ends

proc CsdDipToPx dip,dpi
	invoke	MulDiv,ecx,edx,CSD_DPI_BASE
	ret
endp

proc CsdCaptionLayout uses rbx rsi rdi r12 r13 r14, descp,geomp,count,clientp,dpi,titleHeightDip,edgeDip,dragPadDip,dragRectp
    locals
	left_x		dd ?
	right_x		dd ?
	title_top	dd ?
	title_bottom	dd ?
	drag_pad	dd ?
	control_w	dd ?
    endl

	mov	rsi,rcx
	mov	rdi,rdx
	mov	ebx,r8d
	mov	r12,r9
	mov	r13d,dword [dpi]
	mov	r14,[dragRectp]

	fastcall CsdDipToPx,dword [edgeDip],r13d
	mov	[title_top],eax
	mov	edx,[r12+RECT.left]
	add	edx,eax
	mov	[left_x],edx
	mov	edx,[r12+RECT.right]
	sub	edx,eax
	mov	[right_x],edx

	fastcall CsdDipToPx,dword [titleHeightDip],r13d
	add	eax,[r12+RECT.top]
	mov	[title_bottom],eax

	fastcall CsdDipToPx,dword [dragPadDip],r13d
	mov	[drag_pad],eax

	test	ebx,ebx
	jz	layout_drag_rect

  layout_loop:
	fastcall CsdDipToPx,dword [rsi+CSD_CAPTION_DESCRIPTOR.widthDip],r13d
	mov	[control_w],eax

	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_ALIGN_TRAILING
	jz	layout_leading

	mov	eax,[right_x]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.right],eax
	sub	eax,[control_w]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.left],eax
	mov	[right_x],eax
	jmp	layout_vertical

  layout_leading:
	mov	eax,[left_x]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.left],eax
	add	eax,[control_w]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.right],eax
	mov	[left_x],eax

  layout_vertical:
	mov	eax,[title_top]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.top],eax
	mov	eax,[title_bottom]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.bottom],eax

	add	rsi,sizeof.CSD_CAPTION_DESCRIPTOR
	add	rdi,sizeof.CSD_CAPTION_GEOMETRY
	dec	ebx
	jnz	layout_loop

  layout_drag_rect:
	mov	eax,[left_x]
	add	eax,[drag_pad]
	mov	[r14+RECT.left],eax
	mov	eax,[right_x]
	sub	eax,[drag_pad]
	mov	[r14+RECT.right],eax
	mov	eax,[title_top]
	mov	[r14+RECT.top],eax
	mov	eax,[title_bottom]
	mov	[r14+RECT.bottom],eax
	ret
endp

proc CsdCaptionIndexFromPoint geomp,count,x,y
	xor	eax,eax
	test	edx,edx
	jz	find_none

  find_loop:
	cmp	r8d,[rcx+CSD_CAPTION_GEOMETRY.rect.left]
	jl	find_next
	cmp	r8d,[rcx+CSD_CAPTION_GEOMETRY.rect.right]
	jge	find_next
	cmp	r9d,[rcx+CSD_CAPTION_GEOMETRY.rect.top]
	jl	find_next
	cmp	r9d,[rcx+CSD_CAPTION_GEOMETRY.rect.bottom]
	jge	find_next
	ret

  find_next:
	add	rcx,sizeof.CSD_CAPTION_GEOMETRY
	inc	eax
	dec	edx
	jnz	find_loop

  find_none:
	mov	eax,CSD_CAPTION_NO_INDEX
	ret
endp

proc CsdUpdateSystemMenuState uses rbx rsi, menup,hwnd
	mov	rbx,rcx
	mov	rsi,rdx

	invoke	IsIconic,rsi
	test	eax,eax
	jnz	menu_state_minimized
	invoke	IsZoomed,rsi
	test	eax,eax
	jnz	menu_state_maximized

	iterate <command,state>,\
		SC_RESTORE,	MF_GRAYED,\
		SC_MOVE,	MF_ENABLED,\
		SC_SIZE,	MF_ENABLED,\
		SC_MINIMIZE,	MF_ENABLED,\
		SC_MAXIMIZE,	MF_ENABLED

		invoke	EnableMenuItem,rbx,command,MF_BYCOMMAND or state
	end iterate
	ret

  menu_state_maximized:
	iterate <command,state>,\
		SC_RESTORE,	MF_ENABLED,\
		SC_MOVE,	MF_GRAYED,\
		SC_SIZE,	MF_GRAYED,\
		SC_MINIMIZE,	MF_ENABLED,\
		SC_MAXIMIZE,	MF_GRAYED

		invoke	EnableMenuItem,rbx,command,MF_BYCOMMAND or state
	end iterate
	ret

  menu_state_minimized:
	iterate <command,state>,\
		SC_RESTORE,	MF_ENABLED,\
		SC_MOVE,	MF_GRAYED,\
		SC_SIZE,	MF_GRAYED,\
		SC_MINIMIZE,	MF_GRAYED,\
		SC_MAXIMIZE,	MF_ENABLED

		invoke	EnableMenuItem,rbx,command,MF_BYCOMMAND or state
	end iterate
	ret
endp

proc CsdRefreshSystemMenuState hwnd
	mov	[hwnd],rcx
	invoke	GetSystemMenu,[hwnd],0
	test	rax,rax
	jz	refresh_done
	fastcall CsdUpdateSystemMenuState,rax,[hwnd]

  refresh_done:
	ret
endp

proc CsdDispatchSystemCommand hwnd,command
	mov	[hwnd],rcx
	mov	dword [command],edx
	invoke	SendMessageW,[hwnd],WM_SYSCOMMAND,dword [command],0
	cmp	dword [command],SC_CLOSE
	je	dispatch_done
	fastcall CsdRefreshSystemMenuState,[hwnd]
	invoke	RedrawWindow,[hwnd],0,0,RDW_INVALIDATE or RDW_FRAME

  dispatch_done:
	ret
endp
