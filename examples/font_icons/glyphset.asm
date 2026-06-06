; glyphset.asm - browse a drop-in collection of layered glyph configurations.

include 'windows.inc'
include 'font_icons.inc'

ID_GLYPHSET_LIST	= 3001
ID_GLYPHSET_VIEW	= 3002
ID_GLYPHSET_STATUS	= 3003
ID_GLYPHSET_FONT_LABEL	= 3004
ID_GLYPHSET_FONT_COMBO	= 3005

GSET_BG			= 0141212h
GSET_TEXT		= 0F7F2EEh
GSET_MUTED		= 0B2A49Ah
GSET_MIN_WINDOW_W	= 620
GSET_MIN_WINDOW_H	= 360
GSET_FONT_FLUENT	= 0
GSET_FONT_MDL2		= 1
GSET_FONT_SYMBOL	= 2

struct GLYPHSET_ITEM
  title  dq ?
  layers dq ?
  count  dd ?
	 align 8
ends

include 'glyphset_layers.inc'

define __GLOBAL_DATA__ glyphset_data
define __GLOBAL_BSS__ glyphset_bss

macro glyphset_data
	main_class GLOBWSTR 'Fasm2GlyphSetMain',0
	view_class GLOBWSTR 'Fasm2GlyphSetView',0
	font_mdl2 GLOBWSTR 'Segoe MDL2 Assets',0
	font_fluent GLOBWSTR 'Segoe Fluent Icons',0
	font_symbol GLOBWSTR 'Segoe UI Symbol',0

	glyphset_user_strings

	align 8
	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszClassName: main_class

	view_wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: ViewProc,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszClassName: view_class

	glyphset_user_layers

	align 8
	glyphset_items:
		glyphset_user_items
	glyphset_item_count = ($ - glyphset_items) / sizeof.GLYPHSET_ITEM
purge glyphset_data
end macro

macro glyphset_bss
	align 8
	hInstance dq ?
	hMain dq ?
	hLabelFont dq ?
	hComboFont dq ?
	hList dq ?
	hView dq ?
	hStatus dq ?
	hUIFont dq ?
	hBackBrush dq ?
	current_index dd ?
	current_font dd ?
	msg MSG
purge glyphset_bss
end macro

proc CurrentItem
	mov	eax,dword [current_index]
	cmp	eax,glyphset_item_count
	jae	.none
	lea	rdx,[glyphset_items]
	imul	eax,sizeof.GLYPHSET_ITEM
	lea	rax,[rdx+rax]
	ret
  .none:
	xor	eax,eax
	ret
endp

proc CurrentFace
	cmp	dword [current_font],GSET_FONT_FLUENT
	je	.fluent
	cmp	dword [current_font],GSET_FONT_SYMBOL
	je	.symbol
	lea	rax,[font_mdl2]
	ret
  .fluent:
	lea	rax,[font_fluent]
	ret
  .symbol:
	lea	rax,[font_symbol]
	ret
endp

proc UpdateStatus uses rbx
    locals
	text rw 256
    endl

	fastcall CurrentItem
	mov	rbx,rax
	test	rax,rax
	jz	.empty
	mov	eax,dword [current_index]
	inc	eax
	invoke	wsprintfW,addr text,'%s  |  layers: %u  |  %u/%u',[rbx+GLYPHSET_ITEM.title],\
		dword [rbx+GLYPHSET_ITEM.count],eax,glyphset_item_count
	invoke	SetWindowTextW,[hStatus],addr text
	ret
  .empty:
	invoke	SetWindowTextW,[hStatus],'Drop layered glyph blocks into glyphset_layers.inc.'
	ret
endp

proc SelectIndex index
	mov	dword [index],ecx
	mov	eax,dword [index]
	cmp	eax,glyphset_item_count
	jae	.done
	mov	dword [current_index],eax
	invoke	SendMessageW,[hList],LB_SETCURSEL,dword [index],0
	fastcall UpdateStatus
	invoke	InvalidateRect,[hView],0,1
  .done:
	ret
endp

proc SelectDelta delta
	mov	dword [delta],ecx
	mov	eax,glyphset_item_count
	test	eax,eax
	jz	.done
	mov	eax,dword [current_index]
	add	eax,dword [delta]
	cmp	eax,0
	jge	.check_high
	mov	eax,glyphset_item_count - 1
	jmp	.select
  .check_high:
	cmp	eax,glyphset_item_count
	jb	.select
	xor	eax,eax
  .select:
	fastcall SelectIndex,eax
  .done:
	ret
endp

proc PopulateList uses rbx rsi
	lea	rbx,[glyphset_items]
	mov	esi,glyphset_item_count
	test	esi,esi
	jz	.empty
  .loop:
	invoke	SendMessageW,[hList],LB_ADDSTRING,0,[rbx+GLYPHSET_ITEM.title]
	add	rbx,sizeof.GLYPHSET_ITEM
	dec	esi
	jnz	.loop
	fastcall SelectIndex,0
	ret
  .empty:
	mov	dword [current_index],-1
	fastcall UpdateStatus
	ret
endp

proc ApplyFonts
	cmp	[hUIFont],0
	jne	.have_ui
	invoke	CreateFontW,-13,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,'Segoe UI'
	mov	[hUIFont],rax
  .have_ui:
	invoke	SendMessageW,[hLabelFont],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hComboFont],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hList],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hStatus],WM_SETFONT,[hUIFont],1
	ret
endp

proc CreateControls hwnd
	mov	[hwnd],rcx
	invoke	CreateWindowExW,0,'STATIC','Font:',\
		WS_CHILD or WS_VISIBLE or SS_LEFT,\
		0,0,0,0,[hwnd],ID_GLYPHSET_FONT_LABEL,[hInstance],0
	mov	[hLabelFont],rax
	invoke	CreateWindowExW,0,'COMBOBOX',0,\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP or WS_VSCROLL or CBS_DROPDOWNLIST,\
		0,0,0,0,[hwnd],ID_GLYPHSET_FONT_COMBO,[hInstance],0
	mov	[hComboFont],rax
	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,'Segoe Fluent Icons'
	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,'Segoe MDL2 Assets'
	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,'Segoe UI Symbol'
	invoke	SendMessageW,[hComboFont],CB_SETCURSEL,0,0
	invoke	CreateWindowExW,WS_EX_CLIENTEDGE,'LISTBOX',0,\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP or WS_VSCROLL or LBS_NOTIFY or LBS_NOINTEGRALHEIGHT,\
		0,0,0,0,[hwnd],ID_GLYPHSET_LIST,[hInstance],0
	mov	[hList],rax
	invoke	CreateWindowExW,WS_EX_CLIENTEDGE,view_class,0,\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP,\
		0,0,0,0,[hwnd],ID_GLYPHSET_VIEW,[hInstance],0
	mov	[hView],rax
	invoke	CreateWindowExW,0,'STATIC',0,\
		WS_CHILD or WS_VISIBLE or SS_LEFT,\
		0,0,0,0,[hwnd],ID_GLYPHSET_STATUS,[hInstance],0
	mov	[hStatus],rax
	fastcall ApplyFonts
	fastcall PopulateList
	ret
endp

proc Layout hwnd
    locals
	rc RECT
	width dd ?
	height dd ?
	list_w dd ?
	status_h dd ?
	view_x dd ?
	view_w dd ?
	content_y dd ?
	content_h dd ?
    endl

	mov	[hwnd],rcx
	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	mov	dword [width],eax
	mov	eax,dword [rc.bottom]
	sub	eax,dword [rc.top]
	mov	dword [height],eax
	mov	dword [status_h],28
	mov	dword [list_w],240
	mov	dword [content_y],44
	mov	eax,dword [width]
	cmp	eax,520
	jae	.have_list_w
	mov	dword [list_w],200
  .have_list_w:
	mov	eax,dword [height]
	sub	eax,dword [content_y]
	sub	eax,dword [status_h]
	sub	eax,16
	cmp	eax,120
	jge	.content_ready
	mov	eax,120
  .content_ready:
	mov	dword [content_h],eax
	invoke	MoveWindow,[hLabelFont],8,12,48,22,1
	invoke	MoveWindow,[hComboFont],64,8,300,180,1
	invoke	MoveWindow,[hList],8,dword [content_y],dword [list_w],dword [content_h],1
	mov	eax,dword [list_w]
	add	eax,16
	mov	dword [view_x],eax
	mov	eax,dword [width]
	sub	eax,dword [view_x]
	sub	eax,8
	cmp	eax,160
	jge	.view_w_ready
	mov	eax,160
  .view_w_ready:
	mov	dword [view_w],eax
	invoke	MoveWindow,[hView],dword [view_x],dword [content_y],dword [view_w],dword [content_h],1
	mov	eax,dword [height]
	sub	eax,dword [status_h]
	add	eax,4
	invoke	MoveWindow,[hStatus],8,eax,dword [width],24,1
	ret
endp

proc OnFontComboChange
	invoke	SendMessageW,[hComboFont],CB_GETCURSEL,0,0
	cmp	eax,GSET_FONT_FLUENT
	je	.fluent
	cmp	eax,GSET_FONT_SYMBOL
	je	.symbol
	mov	dword [current_font],GSET_FONT_MDL2
	jmp	.update
  .fluent:
	mov	dword [current_font],GSET_FONT_FLUENT
	jmp	.update
  .symbol:
	mov	dword [current_font],GSET_FONT_SYMBOL
  .update:
	invoke	InvalidateRect,[hView],0,1
	ret
endp

proc DrawEmptyMessage uses rbx rsi, hdc,rectp
    locals
	old_font dq ?
    endl

	mov	rbx,rcx
	mov	rsi,rdx
	invoke	SetBkMode,rbx,TRANSPARENT
	invoke	SetTextColor,rbx,GSET_MUTED
	invoke	SelectObject,rbx,[hUIFont]
	mov	[old_font],rax
	invoke	DrawTextW,rbx,'Drop layered glyph blocks into glyphset_layers.inc.',-1,rsi,\
		DT_CENTER or DT_VCENTER or DT_SINGLELINE
	invoke	SelectObject,rbx,[old_font]
	ret
endp

proc ViewOnPaint uses rbx rdi, hwnd
    locals
	ps PAINTSTRUCT
	rc RECT
	draw_rect RECT
	width dd ?
	height dd ?
	glyph_px dd ?
	facep dq ?
	hIconFont dq ?
    endl

	mov	[hwnd],rcx
	invoke	BeginPaint,[hwnd],addr ps
	mov	rdi,rax
	invoke	GetClientRect,[hwnd],addr rc
	invoke	FillRect,rdi,addr rc,[hBackBrush]

	fastcall CurrentItem
	mov	rbx,rax
	test	rax,rax
	jz	.empty
	mov	eax,dword [rbx+GLYPHSET_ITEM.count]
	test	eax,eax
	jz	.empty
	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	mov	dword [width],eax
	mov	eax,dword [rc.bottom]
	sub	eax,dword [rc.top]
	mov	dword [height],eax
	mov	eax,dword [width]
	cmp	eax,dword [height]
	jle	.size_ready
	mov	eax,dword [height]
  .size_ready:
	sub	eax,48
	cmp	eax,48
	jge	.have_glyph_px
	mov	eax,48
  .have_glyph_px:
	mov	dword [glyph_px],eax
	mov	eax,dword [width]
	sub	eax,dword [glyph_px]
	cdq
	mov	ecx,2
	idiv	ecx
	mov	dword [draw_rect.left],eax
	mov	eax,dword [height]
	sub	eax,dword [glyph_px]
	cdq
	idiv	ecx
	mov	dword [draw_rect.top],eax
	mov	eax,dword [draw_rect.left]
	add	eax,dword [glyph_px]
	mov	dword [draw_rect.right],eax
	mov	eax,dword [draw_rect.top]
	add	eax,dword [glyph_px]
	mov	dword [draw_rect.bottom],eax
	fastcall CurrentFace
	mov	[facep],rax
	fastcall FontIcon_CreateFontForWindow,[hMain],dword [glyph_px],[facep]
	mov	[hIconFont],rax
	test	rax,rax
	jz	.paint_done
	fastcall FontIcon_DrawLayeredGlyphArray,rdi,[rbx+GLYPHSET_ITEM.layers],\
		dword [rbx+GLYPHSET_ITEM.count],addr draw_rect,[hIconFont]
	invoke	DeleteObject,[hIconFont]
	jmp	.paint_done

  .empty:
	fastcall DrawEmptyMessage,rdi,addr rc
  .paint_done:
	invoke	EndPaint,[hwnd],addr ps
	ret
endp

proc NavigateKey key
	iterate <vk_,		branch>,\
		VK_LEFT,	prev,\
		VK_UP,		prev,\
		VK_RIGHT,	next,\
		VK_DOWN,	next,\
		VK_SPACE,	next,\
		VK_HOME,	first,\
		VK_END,		last

		cmp ecx, vk_
		jz branch
	end iterate
	xor	eax,eax
	ret
  prev:
	fastcall SelectDelta,-1
	jmp 	handled
  next:
	fastcall SelectDelta,1
	jmp 	handled
  first:
	xor	ecx, ecx
	jmp	select
  last:
	mov	ecx,glyphset_item_count
	jrcxz	handled
	dec	ecx
  select:
	fastcall SelectIndex,ecx
  handled:
	mov	eax,1
	ret
endp

proc ViewProc hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	iterate <message,branch>,\
		WM_PAINT,	.wm_paint,\
		WM_ERASEBKGND,	.wm_erasebkgnd,\
		WM_LBUTTONDOWN,	.wm_lbuttondown,\
		WM_KEYDOWN,	.wm_keydown,\
		WM_MOUSEWHEEL,	.wm_mousewheel

		cmp	edx,message
		je	branch
	end iterate
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .wm_paint:
	fastcall ViewOnPaint,[hwnd]
	xor	eax,eax
	ret
  .wm_erasebkgnd:
	mov	eax,1
	ret
  .wm_lbuttondown:
	invoke	SetFocus,[hwnd]
	xor	eax,eax
	ret
  .wm_keydown:
	fastcall NavigateKey,dword [wparam]
	test	eax,eax
	jnz	.handled
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .wm_mousewheel:
	movsx	eax,word [wparam+2]
	test	eax,eax
	jg	.wheel_prev
	fastcall SelectDelta,1
	jmp	.handled
  .wheel_prev:
	fastcall SelectDelta,-1
  .handled:
	xor	eax,eax
	ret
endp

proc WindowProc uses rbx, hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	iterate <message,branch>,\
		WM_CREATE,		.wm_create,\
		WM_SIZE,		.wm_size,\
		WM_COMMAND,		.wm_command,\
		WM_KEYDOWN,		.wm_keydown,\
		WM_GETMINMAXINFO,	.wm_getminmaxinfo,\
		WM_DESTROY,		.wm_destroy

		cmp	edx,message
		je	branch
	end iterate
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_create:
	mov	dword [current_font],GSET_FONT_FLUENT
	invoke	CreateSolidBrush,GSET_BG
	mov	[hBackBrush],rax
	fastcall CreateControls,[hwnd]
	fastcall Layout,[hwnd]
	xor	eax,eax
	ret

  .wm_size:
	fastcall Layout,[hwnd]
	invoke	InvalidateRect,[hView],0,1
	xor	eax,eax
	ret

  .wm_command:
	mov	eax,dword [wparam]
	mov	ecx,eax
	and	ecx,0FFFFh
	shr	eax,16
	cmp	ecx,ID_GLYPHSET_FONT_COMBO
	je	.font_cmd
	cmp	ecx,ID_GLYPHSET_LIST
	jne	.default
	cmp	eax,LBN_SELCHANGE
	jne	.default
	invoke	SendMessageW,[hList],LB_GETCURSEL,0,0
	cmp	eax,LB_ERR
	je	.done_zero
	fastcall SelectIndex,eax
  .done_zero:
	xor	eax,eax
	ret

  .font_cmd:
	cmp	eax,CBN_SELCHANGE
	jne	.done_zero
	fastcall OnFontComboChange
	jmp	.done_zero

  .wm_keydown:
	fastcall NavigateKey,dword [wparam]
	test	eax,eax
	jnz	.done_zero
	jmp	.default

  .wm_getminmaxinfo:
	mov	rbx,[lparam]
	mov	dword [rbx+MINMAXINFO.ptMinTrackSize.x],GSET_MIN_WINDOW_W
	mov	dword [rbx+MINMAXINFO.ptMinTrackSize.y],GSET_MIN_WINDOW_H
	xor	eax,eax
	ret

  .wm_destroy:
	mov	rcx,[hUIFont]
	jrcxz	.no_font
	invoke	DeleteObject,rcx
  .no_font:
	mov	rcx,[hBackBrush]
	jrcxz	.no_brush
	invoke	DeleteObject,rcx
  .no_brush:
	invoke	PostQuitMessage,0
	xor	eax,eax
	ret

  .default:
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
endp

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[wc.hInstance],rax
	mov	[view_wc.hInstance],rax

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[wc.hCursor],rax
	mov	[view_wc.hCursor],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[wc.hIcon],rax
	mov	[wc.hIconSm],rax

	invoke	RegisterClassExW,addr wc
	test	rax,rax
	jz	.fatal
	invoke	RegisterClassExW,addr view_wc
	test	rax,rax
	jz	.fatal

	invoke	CreateWindowExW,0,main_class,'glyphset - layered icon collection',\
		WS_OVERLAPPEDWINDOW,\
		CW_USEDEFAULT,CW_USEDEFAULT,760,460,\
		0,0,[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	.fatal

	invoke	ShowWindow,[hMain],SW_SHOWDEFAULT
	invoke	UpdateWindow,[hMain]

  .message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jz	.exit
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	.message_loop

  .exit:
	invoke	ExitProcess,dword [msg.wParam]

  .fatal:
	invoke	MessageBoxW,0,'glyphset.asm failed to start.','glyphset - layered icon collection',MB_ICONERROR
	invoke	ExitProcess,1
endp
