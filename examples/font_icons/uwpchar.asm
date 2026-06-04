; uwpchar.asm - assembly icon-font browser/export sample.

include 'windows.inc'
include 'font_icons.inc'
include 'uwpchar_data.inc'

ID_FONT_COMBO	= 1001
ID_SIZE_COMBO	= 1002
ID_COPY_BUTTON	= 1003
ID_STATUS	= 1004
ID_VIEW		= 1005
ID_EXPORT_EDIT	= 1006
ID_COLOR_FORE	= 1007
ID_COLOR_BACK	= 1008
ID_COLOR_PAIR	= 1009
ID_MERGED_CHECK	= 1010

HEAP_ZERO_MEMORY		= 0008h
GGI_MARK_NONEXISTING_GLYPHS	= 0001h
WHEEL_DELTA			= 120

UWP_VIEW_BG		= 0141212h
UWP_PANEL_BG		= 026201Ch
UWP_TEXT		= 0F7F2EEh
UWP_MUTED		= 0B2A49Ah
UWP_PAIR_FILL		= 0FFD27Eh

define __GLOBAL_DATA__ uwpchar_data
define __GLOBAL_BSS__ uwpchar_bss

macro uwpchar_data
	app_name	du 'uwpchar.asm - icon font browser',0
	main_class	du 'UwpCharAsmMain',0
	view_class	du 'UwpCharAsmView',0
	static_class	du 'STATIC',0
	combo_class	du 'COMBOBOX',0
	button_class	du 'BUTTON',0
	edit_class	du 'EDIT',0

	font_mdl2	du 'Segoe MDL2 Assets',0
	font_fluent	du 'Segoe Fluent Icons',0
	font_mdl2_tag	du 'MDL2',0
	font_fluent_tag	du 'Fluent',0
	font_ui		du 'Segoe UI',0
	font_mono	du 'Consolas',0

	label_font	du 'Font:',0
	label_size	du 'Size:',0
	label_copy	du 'Copy',0
	label_fore	du 'Fore',0
	label_back	du 'Back',0
	label_pair	du 'Pair',0
	label_merged	du 'Merged pairs',0
	status_fmt	du 'Glyphs: %u | Names: %u | Pairs: %u',0
	code_fmt	du 'U+%04X',0
	fallback_icon_fmt du 'ICON_%04X',0
	fallback_u_fmt	du 'U_%04X',0
	insert_single_fmt du 13,10,'%s = 0%04Xh ; %s',0
	insert_pair_fmt du 13,10,'%s = 0%04Xh ; %s',13,10,\
		'%s = 0%04Xh ; %s layered pair',0
	edit_seed	du '; Click glyphs to append fasm constants.',13,10
			du '; MDL2 Fill/Solid companions export as two-layer pairs.',0
	fatal_text	du 'uwpchar.asm failed to start.',0

	size_16		du '16',0
	size_20		du '20',0
	size_24		du '24',0
	size_32		du '32',0
	size_40		du '40',0
	size_48		du '48',0
	size_64		du '64',0

	align 8
	size_texts	dq size_16,size_20,size_24,size_32,size_40,size_48,size_64
	size_values	dd 16,20,24,32,40,48,64

	main_wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		cbClsExtra: 0,\
		cbWndExtra: 0,\
		hInstance: 0,\
		hIcon: 0,\
		hCursor: 0,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszMenuName: 0,\
		lpszClassName: main_class,\
		hIconSm: 0

	view_wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: ViewProc,\
		cbClsExtra: 0,\
		cbWndExtra: 0,\
		hInstance: 0,\
		hIcon: 0,\
		hCursor: 0,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszMenuName: 0,\
		lpszClassName: view_class,\
		hIconSm: 0

	uwpchar_name_data
purge uwpchar_data
end macro

macro uwpchar_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	hView		dq ?
	hComboFont	dq ?
	hComboSize	dq ?
	hButtonCopy	dq ?
	hStatus	dq ?
	hEdit		dq ?
	hLabelFont	dq ?
	hLabelSize	dq ?
	hButtonFore	dq ?
	hButtonBack	dq ?
	hButtonPair	dq ?
	hCheckMerged	dq ?
	hGlyphFont	dq ?
	hUIFont	dq ?
	hMonoFont	dq ?
	hPanelBrush	dq ?
	hViewBrush	dq ?
	glyph_codes	dq ?
	glyph_count	dd ?
	glyph_cap	dd ?
	current_font	dd ?
	font_size	dd ?
	cell_w		dd ?
	cell_h		dd ?
	label_h		dd ?
	scroll_y	dd ?
	show_merged_pairs dd ?
	color_fore	dd ?
	color_back	dd ?
	color_pair	dd ?
	custom_colors	dd 16 dup ?
	msg		MSG
purge uwpchar_bss
end macro

proc CurrentFace
	cmp	dword [current_font],UWPCHAR_FONT_FLUENT
	je	.fluent
	lea	rax,[font_mdl2]
	ret
  .fluent:
	lea	rax,[font_fluent]
	ret
endp

proc CurrentFontTag
	cmp	dword [current_font],UWPCHAR_FONT_FLUENT
	je	.fluent
	lea	rax,[font_mdl2_tag]
	ret
  .fluent:
	lea	rax,[font_fluent_tag]
	ret
endp

proc CopyAsciiToWide uses rsi rdi, destp,srcp,cap
	mov	[destp],rcx
	mov	[srcp],rdx
	mov	dword [cap],r8d
	mov	rdi,[destp]
	mov	rsi,[srcp]
	mov	ecx,dword [cap]
	test	ecx,ecx
	jz	.done
	dec	ecx
  .copy:
	test	ecx,ecx
	jz	.terminate
	movzx	eax,byte [rsi]
	test	al,al
	jz	.terminate
	mov	word [rdi],ax
	inc	rsi
	add	rdi,2
	dec	ecx
	jmp	.copy
  .terminate:
	mov	word [rdi],0
  .done:
	mov	rax,[destp]
	ret
endp

proc RebuildColorBrush
	cmp	qword [hViewBrush],0
	je	.no_old_view
	invoke	DeleteObject,[hViewBrush]
	mov	qword [hViewBrush],0
  .no_old_view:
	invoke	CreateSolidBrush,dword [color_back]
	mov	[hViewBrush],rax
	cmp	qword [hView],0
	je	.no_view
	invoke	InvalidateRect,[hView],0,1
  .no_view:
	cmp	qword [hEdit],0
	je	.done
	invoke	InvalidateRect,[hEdit],0,1
  .done:
	ret
endp

proc PickColor uses rdi, colorp
    locals
	cc CHOOSECOLOR
    endl

	mov	[colorp],rcx
	lea	rdi,[cc]
	xor	eax,eax
	mov	ecx,sizeof.CHOOSECOLOR/8
	rep	stosq
	mov	qword [cc.lStructSize],sizeof.CHOOSECOLOR
	mov	rax,[hMain]
	mov	[cc.hwndOwner],rax
	mov	rax,[colorp]
	mov	eax,dword [rax]
	mov	dword [cc.rgbResult],eax
	mov	qword [cc.lpCustColors],custom_colors
	mov	dword [cc.Flags],CC_RGBINIT or CC_FULLOPEN
	invoke	ChooseColorW,addr cc
	test	eax,eax
	jz	.done
	mov	rax,[colorp]
	mov	ecx,dword [cc.rgbResult]
	mov	dword [rax],ecx
	fastcall RebuildColorBrush
	invoke	InvalidateRect,[hView],0,1
  .done:
	ret
endp

proc GlyphList_Clear
	cmp	qword [glyph_codes],0
	je	.done
	invoke	GetProcessHeap
	invoke	HeapFree,rax,0,[glyph_codes]
	mov	qword [glyph_codes],0
  .done:
	mov	dword [glyph_count],0
	mov	dword [glyph_cap],0
	ret
endp

proc GlyphList_Push uses rbx, code
    locals
	new_cap dd ?
	bytes	dq ?
	heap_handle dq ?
    endl

	mov	dword [code],ecx
	mov	eax,dword [glyph_count]
	cmp	eax,dword [glyph_cap]
	jne	.have_space

	mov	eax,dword [glyph_cap]
	test	eax,eax
	jnz	.grow
	mov	eax,4096
	jmp	.cap_ready
  .grow:
	shl	eax,1
  .cap_ready:
	mov	dword [new_cap],eax
	shl	rax,2
	mov	[bytes],rax
	invoke	GetProcessHeap
	mov	[heap_handle],rax
	cmp	qword [glyph_codes],0
	je	.alloc
	invoke	HeapReAlloc,[heap_handle],HEAP_ZERO_MEMORY,[glyph_codes],[bytes]
	jmp	.alloc_done
  .alloc:
	invoke	HeapAlloc,[heap_handle],HEAP_ZERO_MEMORY,[bytes]
  .alloc_done:
	test	rax,rax
	jz	.fail
	mov	[glyph_codes],rax
	mov	eax,dword [new_cap]
	mov	dword [glyph_cap],eax

  .have_space:
	mov	rbx,[glyph_codes]
	mov	ecx,dword [glyph_count]
	mov	edx,dword [code]
	mov	dword [rbx+rcx*4],edx
	inc	dword [glyph_count]
	mov	eax,1
	ret

  .fail:
	xor	eax,eax
	ret
endp

proc UpdateGlyphFont
	cmp	qword [hGlyphFont],0
	je	.no_old
	invoke	DeleteObject,[hGlyphFont]
	mov	qword [hGlyphFont],0
  .no_old:
	fastcall CurrentFace
	fastcall FontIcon_CreateFontForWindow,[hMain],dword [font_size],rax
	mov	[hGlyphFont],rax
	ret
endp

proc UpdateCellMetrics
	mov	dword [label_h],18
	mov	eax,dword [font_size]
	add	eax,36
	cmp	eax,64
	jge	.width_ready
	mov	eax,64
  .width_ready:
	mov	dword [cell_w],eax
	mov	eax,dword [font_size]
	add	eax,54
	cmp	eax,78
	jge	.height_ready
	mov	eax,78
  .height_ready:
	mov	dword [cell_h],eax
	ret
endp

proc BuildGlyphList uses rbx rsi rdi
    locals
	hdc	dq ?
	old_font dq ?
	code	dd ?
	candidate dd ?
	count	dd ?
	chars	rw 512
	glyphs	rw 512
    endl

	fastcall GlyphList_Clear
	fastcall UpdateGlyphFont
	fastcall UpdateCellMetrics

	invoke	GetDC,0
	mov	[hdc],rax
	test	rax,rax
	jz	.done
	invoke	SelectObject,[hdc],[hGlyphFont]
	mov	[old_font],rax

	mov	dword [code],0020h
  .chunk:
	cmp	dword [code],0FFFDh
	ja	.scan_done
	mov	dword [count],0

  .fill_chunk:
	cmp	dword [count],512
	jae	.have_chunk
	mov	eax,dword [code]
	cmp	eax,0FFFDh
	ja	.have_chunk
	inc	dword [code]
	cmp	eax,0D800h
	jb	.store_char
	cmp	eax,0DFFFh
	jbe	.fill_chunk
  .store_char:
	mov	ecx,dword [count]
	mov	word [chars+rcx*2],ax
	inc	dword [count]
	jmp	.fill_chunk

  .have_chunk:
	cmp	dword [count],0
	je	.scan_done
	invoke	GetGlyphIndicesW,[hdc],addr chars,dword [count],addr glyphs,GGI_MARK_NONEXISTING_GLYPHS
	cmp	eax,GDI_ERROR
	je	.chunk
	xor	edi,edi
 .push_loop:
	cmp	edi,dword [count]
	jae	.chunk
	cmp	word [glyphs+rdi*2],0FFFFh
	je	.next_glyph
	movzx	ecx,word [chars+rdi*2]
	mov	dword [candidate],ecx
	cmp	dword [show_merged_pairs],0
	je	.push_candidate
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.push_candidate
	fastcall FindNameEntry,UWPCHAR_FONT_MDL2,dword [candidate]
	test	rax,rax
	jz	.push_candidate
	test	dword [rax+UWPCHAR_NAME_ENTRY.flags],UWPCHAR_NAME_PAIR_FILL
	jnz	.next_glyph
  .push_candidate:
	mov	ecx,dword [candidate]
	fastcall GlyphList_Push,ecx
  .next_glyph:
	inc	edi
	jmp	.push_loop

  .scan_done:
	invoke	SelectObject,[hdc],[old_font]
	invoke	ReleaseDC,0,[hdc]
  .done:
	ret
endp

proc FindNameEntry uses rbx rsi, font,code
	mov	dword [font],ecx
	mov	dword [code],edx
	cmp	dword [font],UWPCHAR_FONT_FLUENT
	je	.fluent
	lea	rsi,[uwpchar_mdl2_names]
	mov	ebx,uwpchar_mdl2_name_count
	jmp	.loop
  .fluent:
	lea	rsi,[uwpchar_fluent_names]
	mov	ebx,uwpchar_fluent_name_count
  .loop:
	test	ebx,ebx
	jz	.not_found
	mov	eax,dword [rsi+UWPCHAR_NAME_ENTRY.code]
	cmp	eax,dword [code]
	je	.found
	ja	.not_found
	add	rsi,sizeof.UWPCHAR_NAME_ENTRY
	dec	ebx
	jmp	.loop
  .found:
	mov	rax,rsi
	ret
  .not_found:
	xor	eax,eax
	ret
endp

proc UpdateStatus
    locals
	text rw 160
	names dd ?
    endl

	cmp	dword [current_font],UWPCHAR_FONT_FLUENT
	je	.fluent
	mov	dword [names],uwpchar_mdl2_name_count
	jmp	.have_names
  .fluent:
	mov	dword [names],uwpchar_fluent_name_count
  .have_names:
	invoke	wsprintfW,addr text,status_fmt,dword [glyph_count],dword [names],uwpchar_mdl2_pair_count
	invoke	SetWindowTextW,[hStatus],addr text
	ret
endp

proc UpdateViewScroll hwnd
    locals
	rc RECT
	si SCROLLINFO
	cols dd ?
	rows dd ?
	total dd ?
    endl

	mov	[hwnd],rcx
	cmp	qword [hwnd],0
	je	.done
	cmp	dword [cell_w],0
	je	.done
	cmp	dword [cell_h],0
	je	.done
	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	cdq
	idiv	dword [cell_w]
	test	eax,eax
	jg	.cols_ready
	mov	eax,1
  .cols_ready:
	mov	dword [cols],eax
	mov	eax,dword [glyph_count]
	add	eax,dword [cols]
	dec	eax
	xor	edx,edx
	div	dword [cols]
	mov	dword [rows],eax
	imul	eax,dword [cell_h]
	mov	dword [total],eax

	mov	dword [si.cbSize],sizeof.SCROLLINFO
	mov	dword [si.fMask],SIF_RANGE or SIF_PAGE or SIF_POS
	mov	dword [si.nMin],0
	mov	eax,dword [total]
	test	eax,eax
	jg	.have_total
	xor	eax,eax
	jmp	.max_ready
  .have_total:
	dec	eax
  .max_ready:
	mov	dword [si.nMax],eax
	mov	eax,dword [rc.bottom]
	sub	eax,dword [rc.top]
	cmp	eax,1
	jge	.page_ready
	mov	eax,1
  .page_ready:
	mov	dword [si.nPage],eax
	mov	eax,dword [scroll_y]
	mov	dword [si.nPos],eax
	mov	dword [si.nTrackPos],0
	invoke	SetScrollInfo,[hwnd],SB_VERT,addr si,1
  .done:
	ret
endp

proc ClampScroll hwnd
    locals
	si SCROLLINFO
	max_pos dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [si.cbSize],sizeof.SCROLLINFO
	mov	dword [si.fMask],SIF_RANGE or SIF_PAGE or SIF_POS
	invoke	GetScrollInfo,[hwnd],SB_VERT,addr si
	mov	eax,dword [si.nMax]
	sub	eax,dword [si.nPage]
	inc	eax
	jge	.max_ready
	xor	eax,eax
  .max_ready:
	mov	dword [max_pos],eax
	cmp	dword [scroll_y],0
	jge	.min_ready
	mov	dword [scroll_y],0
  .min_ready:
	mov	eax,dword [max_pos]
	cmp	dword [scroll_y],eax
	jle	.pos_ready
	mov	dword [scroll_y],eax
  .pos_ready:
	mov	eax,dword [scroll_y]
	mov	dword [si.nPos],eax
	invoke	SetScrollInfo,[hwnd],SB_VERT,addr si,1
	ret
endp

proc ViewScrollBy hwnd,delta
	mov	[hwnd],rcx
	mov	dword [delta],edx
	mov	eax,dword [delta]
	add	dword [scroll_y],eax
	fastcall ClampScroll,[hwnd]
	invoke	InvalidateRect,[hwnd],0,1
	ret
endp

proc DrawOneCell uses rbx rsi, hdc,code,rectp
    locals
	name_entry dq ?
	base_code dd ?
	fill_code dd ?
    endl

	mov	[hdc],rcx
	mov	dword [code],edx
	mov	[rectp],r8
	fastcall FindNameEntry,dword [current_font],dword [code]
	mov	[name_entry],rax
	test	rax,rax
	jz	.single
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.single
	cmp	dword [show_merged_pairs],0
	je	.single
	mov	rbx,rax
	test	dword [rbx+UWPCHAR_NAME_ENTRY.flags],UWPCHAR_NAME_PAIR_BASE or UWPCHAR_NAME_PAIR_FILL
	jz	.single
	mov	eax,dword [code]
	mov	dword [base_code],eax
	mov	eax,dword [rbx+UWPCHAR_NAME_ENTRY.pair]
	mov	dword [fill_code],eax
	test	dword [rbx+UWPCHAR_NAME_ENTRY.flags],UWPCHAR_NAME_PAIR_FILL
	jz	.layered
	mov	eax,dword [code]
	mov	dword [fill_code],eax
	mov	eax,dword [rbx+UWPCHAR_NAME_ENTRY.pair]
	mov	dword [base_code],eax
  .layered:
	fastcall FontIcon_DrawLayeredGlyph,[hdc],dword [base_code],dword [fill_code],\
		[rectp],[hGlyphFont],dword [color_fore],dword [color_pair]
	ret
  .single:
	fastcall FontIcon_DrawGlyph,[hdc],dword [code],[rectp],[hGlyphFont],dword [color_fore]
	ret
endp

proc ViewOnPaint uses rbx rsi rdi r12 r13 r14 r15, hwnd
    locals
	ps PAINTSTRUCT
	rc RECT
	glyph_rect RECT
	label_rect RECT
	label_text rw 32
	cols dd ?
	start_row dd ?
	end_row dd ?
	row dd ?
	col dd ?
	x dd ?
	y dd ?
	idx dq ?
	code dd ?
	old_font dq ?
    endl

	mov	[hwnd],rcx
	invoke	BeginPaint,[hwnd],addr ps
	mov	r15,rax
	invoke	GetClientRect,[hwnd],addr rc
	invoke	FillRect,r15,addr rc,[hViewBrush]
	invoke	SetBkMode,r15,TRANSPARENT
	cmp	dword [cell_w],0
	je	.paint_done
	cmp	dword [cell_h],0
	je	.paint_done

	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	cdq
	idiv	dword [cell_w]
	test	eax,eax
	jg	.have_cols
	mov	eax,1
  .have_cols:
	mov	dword [cols],eax

	mov	eax,dword [scroll_y]
	cdq
	idiv	dword [cell_h]
	mov	dword [start_row],eax
	mov	eax,dword [scroll_y]
	mov	ecx,dword [rc.bottom]
	sub	ecx,dword [rc.top]
	add	eax,ecx
	cdq
	idiv	dword [cell_h]
	inc	eax
	mov	dword [end_row],eax

	mov	eax,dword [start_row]
	mov	dword [row],eax
  .row_loop:
	mov	eax,dword [row]
	cmp	eax,dword [end_row]
	jg	.paint_done
	mov	dword [col],0
  .col_loop:
	mov	eax,dword [col]
	cmp	eax,dword [cols]
	jge	.next_row

	mov	eax,dword [row]
	imul	eax,dword [cols]
	add	eax,dword [col]
	mov	dword [idx],eax
	mov	dword [idx+4],0
	cmp	eax,dword [glyph_count]
	jae	.next_row
	mov	rbx,[glyph_codes]
	mov	edx,dword [rbx+rax*4]
	mov	dword [code],edx

	mov	eax,dword [col]
	imul	eax,dword [cell_w]
	mov	dword [x],eax
	mov	eax,dword [row]
	imul	eax,dword [cell_h]
	sub	eax,dword [scroll_y]
	mov	dword [y],eax

	mov	eax,dword [x]
	add	eax,6
	mov	dword [glyph_rect.left],eax
	mov	eax,dword [y]
	add	eax,6
	mov	dword [glyph_rect.top],eax
	mov	eax,dword [x]
	add	eax,dword [cell_w]
	sub	eax,6
	mov	dword [glyph_rect.right],eax
	mov	eax,dword [y]
	add	eax,dword [font_size]
	add	eax,14
	mov	dword [glyph_rect.bottom],eax
	fastcall DrawOneCell,r15,dword [code],addr glyph_rect

	invoke	wsprintfW,addr label_text,code_fmt,dword [code]
	invoke	SelectObject,r15,[hUIFont]
	mov	[old_font],rax
	invoke	SetTextColor,r15,UWP_MUTED
	mov	eax,dword [x]
	add	eax,4
	mov	dword [label_rect.left],eax
	mov	eax,dword [y]
	add	eax,dword [font_size]
	add	eax,22
	mov	dword [label_rect.top],eax
	mov	eax,dword [x]
	add	eax,dword [cell_w]
	sub	eax,4
	mov	dword [label_rect.right],eax
	mov	eax,dword [label_rect.top]
	add	eax,dword [label_h]
	mov	dword [label_rect.bottom],eax
	invoke	DrawTextW,r15,addr label_text,-1,addr label_rect,\
		DT_CENTER or DT_VCENTER or DT_SINGLELINE or DT_END_ELLIPSIS
	invoke	SelectObject,r15,[old_font]

	inc	dword [col]
	jmp	.col_loop

  .next_row:
	inc	dword [row]
	jmp	.row_loop

  .paint_done:
	invoke	EndPaint,[hwnd],addr ps
	ret
endp

proc SetExportSeed
	invoke	SetWindowTextW,[hEdit],edit_seed
	ret
endp

proc AppendToEdit textp
    locals
	len dd ?
    endl

	mov	[textp],rcx
	invoke	GetWindowTextLengthW,[hEdit]
	mov	dword [len],eax
	invoke	SendMessageW,[hEdit],EM_SETSEL,dword [len],dword [len]
	invoke	SendMessageW,[hEdit],EM_REPLACESEL,1,[textp]
	ret
endp

proc InsertSelection uses rbx rsi, code
    locals
	name_entry dq ?
	pair_entry dq ?
	namep dq ?
	pair_namep dq ?
	font_tag dq ?
	base_code dd ?
	fill_code dd ?
	fallback rw 32
	name_w rw 256
	pair_name_w rw 256
	line rw 512
    endl

	mov	dword [code],ecx
	fastcall CurrentFontTag
	mov	[font_tag],rax
	fastcall FindNameEntry,dword [current_font],dword [code]
	mov	[name_entry],rax
	test	rax,rax
	jz	.fallback_name
	mov	rbx,rax
	mov	rax,[rbx+UWPCHAR_NAME_ENTRY.name]
	fastcall CopyAsciiToWide,addr name_w,rax,256
	mov	[namep],rax
	mov	qword [pair_namep],0
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.single
	cmp	dword [show_merged_pairs],0
	je	.single
	test	dword [rbx+UWPCHAR_NAME_ENTRY.flags],UWPCHAR_NAME_PAIR_BASE or UWPCHAR_NAME_PAIR_FILL
	jz	.single
	mov	eax,dword [code]
	mov	dword [base_code],eax
	mov	eax,dword [rbx+UWPCHAR_NAME_ENTRY.pair]
	mov	dword [fill_code],eax
	fastcall FindNameEntry,dword [current_font],dword [fill_code]
	mov	[pair_entry],rax
	test	rax,rax
	jz	.have_pair_name
	mov	rax,[rax+UWPCHAR_NAME_ENTRY.name]
	fastcall CopyAsciiToWide,addr pair_name_w,rax,256
	mov	[pair_namep],rax
	jmp	.have_pair_name
  .have_pair_name:
	cmp	qword [pair_namep],0
	jne	.have_pair_namep
	mov	rax,[namep]
	mov	[pair_namep],rax
  .have_pair_namep:
	test	dword [rbx+UWPCHAR_NAME_ENTRY.flags],UWPCHAR_NAME_PAIR_FILL
	jz	.format_pair
	mov	eax,dword [code]
	mov	dword [fill_code],eax
	mov	eax,dword [rbx+UWPCHAR_NAME_ENTRY.pair]
	mov	dword [base_code],eax
	mov	rax,[namep]
	mov	[pair_namep],rax
	fastcall FindNameEntry,dword [current_font],dword [base_code]
	test	rax,rax
	jz	.format_pair
	mov	rax,[rax+UWPCHAR_NAME_ENTRY.name]
	fastcall CopyAsciiToWide,addr pair_name_w,rax,256
	mov	[namep],rax
  .format_pair:
	invoke	wsprintfW,addr line,insert_pair_fmt,[namep],dword [base_code],[font_tag],\
		[pair_namep],dword [fill_code],[font_tag]
	fastcall AppendToEdit,addr line
	ret

  .fallback_name:
	cmp	dword [code],0E000h
	jb	.fallback_u
	cmp	dword [code],0F8FFh
	ja	.fallback_u
	invoke	wsprintfW,addr fallback,fallback_icon_fmt,dword [code]
	jmp	.fallback_ready
  .fallback_u:
	invoke	wsprintfW,addr fallback,fallback_u_fmt,dword [code]
  .fallback_ready:
	lea	rax,[fallback]
	mov	[namep],rax
  .single:
	invoke	wsprintfW,addr line,insert_single_fmt,[namep],dword [code],[font_tag]
	fastcall AppendToEdit,addr line
	ret
endp

proc CopyExportText
    locals
	len dd ?
	bytes dq ?
	handle dq ?
	buffer dq ?
    endl

	invoke	GetWindowTextLengthW,[hEdit]
	test	eax,eax
	jle	.done
	mov	dword [len],eax
	inc	eax
	shl	rax,1
	mov	[bytes],rax
	invoke	GlobalAlloc,GMEM_MOVEABLE,[bytes]
	mov	[handle],rax
	test	rax,rax
	jz	.done
	invoke	GlobalLock,[handle]
	mov	[buffer],rax
	test	rax,rax
	jz	.free_handle
	mov	eax,dword [len]
	inc	eax
	invoke	GetWindowTextW,[hEdit],[buffer],eax
	invoke	GlobalUnlock,[handle]
	invoke	OpenClipboard,[hMain]
	test	eax,eax
	jz	.free_handle
	invoke	EmptyClipboard
	invoke	SetClipboardData,CF_UNICODETEXT,[handle]
	invoke	CloseClipboard
	ret

  .free_handle:
	invoke	GlobalFree,[handle]
  .done:
	ret
endp

proc UpdateAll
	fastcall BuildGlyphList
	fastcall UpdateStatus
	fastcall UpdateViewScroll,[hView]
	invoke	InvalidateRect,[hView],0,1
	ret
endp

proc ApplyFonts
	cmp	qword [hUIFont],0
	jne	.have_ui
	invoke	CreateFontW,-13,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,font_ui
	mov	[hUIFont],rax
  .have_ui:
	cmp	qword [hMonoFont],0
	jne	.have_mono
	invoke	CreateFontW,-13,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,FIXED_PITCH or FF_DONTCARE,font_mono
	mov	[hMonoFont],rax
  .have_mono:
	invoke	SendMessageW,[hComboFont],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hComboSize],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hButtonCopy],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hStatus],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hLabelFont],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hLabelSize],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hButtonFore],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hButtonBack],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hButtonPair],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hCheckMerged],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hEdit],WM_SETFONT,[hMonoFont],1
	ret
endp

proc CreateControls hwnd
	mov	[hwnd],rcx
	mov	dword [current_font],UWPCHAR_FONT_MDL2
	mov	dword [font_size],24
	mov	dword [show_merged_pairs],1
	mov	dword [color_fore],UWP_TEXT
	mov	dword [color_back],UWP_VIEW_BG
	mov	dword [color_pair],UWP_PAIR_FILL
	fastcall UpdateCellMetrics
	invoke	CreateWindowExW,0,static_class,label_font,WS_CHILD or WS_VISIBLE,\
		0,0,0,0,[hwnd],0,[hInstance],0
	mov	[hLabelFont],rax
	invoke	CreateWindowExW,0,combo_class,0,WS_CHILD or WS_VISIBLE or CBS_DROPDOWNLIST,\
		0,0,0,0,[hwnd],ID_FONT_COMBO,[hInstance],0
	mov	[hComboFont],rax
	invoke	CreateWindowExW,0,static_class,label_size,WS_CHILD or WS_VISIBLE,\
		0,0,0,0,[hwnd],0,[hInstance],0
	mov	[hLabelSize],rax
	invoke	CreateWindowExW,0,combo_class,0,WS_CHILD or WS_VISIBLE or CBS_DROPDOWNLIST,\
		0,0,0,0,[hwnd],ID_SIZE_COMBO,[hInstance],0
	mov	[hComboSize],rax
	invoke	CreateWindowExW,0,button_class,label_copy,WS_CHILD or WS_VISIBLE or BS_PUSHBUTTON,\
		0,0,0,0,[hwnd],ID_COPY_BUTTON,[hInstance],0
	mov	[hButtonCopy],rax
	invoke	CreateWindowExW,0,static_class,0,WS_CHILD or WS_VISIBLE,\
		0,0,0,0,[hwnd],ID_STATUS,[hInstance],0
	mov	[hStatus],rax
	invoke	CreateWindowExW,0,button_class,label_fore,WS_CHILD or WS_VISIBLE or BS_PUSHBUTTON,\
		0,0,0,0,[hwnd],ID_COLOR_FORE,[hInstance],0
	mov	[hButtonFore],rax
	invoke	CreateWindowExW,0,button_class,label_back,WS_CHILD or WS_VISIBLE or BS_PUSHBUTTON,\
		0,0,0,0,[hwnd],ID_COLOR_BACK,[hInstance],0
	mov	[hButtonBack],rax
	invoke	CreateWindowExW,0,button_class,label_pair,WS_CHILD or WS_VISIBLE or BS_PUSHBUTTON,\
		0,0,0,0,[hwnd],ID_COLOR_PAIR,[hInstance],0
	mov	[hButtonPair],rax
	invoke	CreateWindowExW,0,button_class,label_merged,\
		WS_CHILD or WS_VISIBLE or BS_AUTOCHECKBOX,\
		0,0,0,0,[hwnd],ID_MERGED_CHECK,[hInstance],0
	mov	[hCheckMerged],rax
	invoke	SendMessageW,[hCheckMerged],BM_SETCHECK,BST_CHECKED,0
	invoke	CreateWindowExW,WS_EX_CLIENTEDGE,edit_class,0,\
		WS_CHILD or WS_VISIBLE or ES_MULTILINE or ES_AUTOVSCROLL or ES_NOHIDESEL or ES_WANTRETURN or WS_VSCROLL,\
		0,0,0,0,[hwnd],ID_EXPORT_EDIT,[hInstance],0
	mov	[hEdit],rax
	invoke	SendMessageW,[hEdit],EM_SETLIMITTEXT,0,0
	fastcall SetExportSeed
	invoke	CreateWindowExW,WS_EX_CLIENTEDGE,view_class,0,WS_CHILD or WS_VISIBLE or WS_VSCROLL,\
		0,0,0,0,[hwnd],ID_VIEW,[hInstance],0
	mov	[hView],rax

	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,font_mdl2
	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,font_fluent
	invoke	SendMessageW,[hComboFont],CB_SETCURSEL,0,0

	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_16
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_20
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_24
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_32
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_40
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_48
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,size_64
	invoke	SendMessageW,[hComboSize],CB_SETCURSEL,2,0
	invoke	CreateSolidBrush,UWP_PANEL_BG
	mov	[hPanelBrush],rax
	fastcall RebuildColorBrush
	fastcall ApplyFonts
	ret
endp

proc Layout hwnd
    locals
	rc RECT
	width dd ?
	height dd ?
	view_x dd ?
	view_w dd ?
	edit_h dd ?
    endl

	mov	[hwnd],rcx
	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	mov	dword [width],eax
	mov	eax,dword [rc.bottom]
	sub	eax,dword [rc.top]
	mov	dword [height],eax

	invoke	MoveWindow,[hLabelFont],8,12,48,22,1
	invoke	MoveWindow,[hComboFont],64,8,300,180,1
	invoke	MoveWindow,[hLabelSize],8,48,48,22,1
	invoke	MoveWindow,[hComboSize],64,44,82,180,1
	invoke	MoveWindow,[hCheckMerged],160,44,160,24,1
	invoke	MoveWindow,[hButtonFore],8,80,64,28,1
	invoke	MoveWindow,[hButtonBack],80,80,64,28,1
	invoke	MoveWindow,[hButtonPair],152,80,64,28,1
	invoke	MoveWindow,[hButtonCopy],232,80,76,28,1
	invoke	MoveWindow,[hStatus],8,116,360,22,1

	mov	eax,dword [height]
	sub	eax,156
	cmp	eax,120
	jge	.edit_h_ready
	mov	eax,120
  .edit_h_ready:
	mov	dword [edit_h],eax
	invoke	MoveWindow,[hEdit],8,148,376,dword [edit_h],1

	mov	dword [view_x],400
	mov	eax,dword [width]
	sub	eax,dword [view_x]
	sub	eax,8
	cmp	eax,160
	jge	.view_w_ready
	mov	eax,160
  .view_w_ready:
	mov	dword [view_w],eax
	mov	eax,dword [height]
	sub	eax,16
	cmp	eax,160
	jge	.view_h_ready
	mov	eax,160
  .view_h_ready:
	invoke	MoveWindow,[hView],dword [view_x],8,dword [view_w],eax,1
	ret
endp

proc OnFontComboChange
	invoke	SendMessageW,[hComboFont],CB_GETCURSEL,0,0
	cmp	eax,1
	je	.fluent
	mov	dword [current_font],UWPCHAR_FONT_MDL2
	jmp	.update
  .fluent:
	mov	dword [current_font],UWPCHAR_FONT_FLUENT
  .update:
	mov	dword [scroll_y],0
	fastcall UpdateAll
	ret
endp

proc OnSizeComboChange
	invoke	SendMessageW,[hComboSize],CB_GETCURSEL,0,0
	cmp	eax,0
	jl	.done
	cmp	eax,6
	jg	.done
	mov	eax,dword [size_values+rax*4]
	mov	dword [font_size],eax
	mov	dword [scroll_y],0
	fastcall UpdateAll
  .done:
	ret
endp

proc ViewProc uses rbx, hwnd,wmsg,wparam,lparam
    locals
	si SCROLLINFO
	rc RECT
	cols dd ?
	x dd ?
	y dd ?
	col dd ?
	row dd ?
	idx dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_VSCROLL
	je	.wm_vscroll
	cmp	edx,WM_MOUSEWHEEL
	je	.wm_mousewheel
	cmp	edx,WM_LBUTTONDOWN
	je	.wm_lbuttondown
	cmp	edx,WM_PAINT
	je	.wm_paint
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_size:
	fastcall UpdateViewScroll,[hwnd]
	xor	eax,eax
	ret

  .wm_vscroll:
	mov	eax,dword [wparam]
	and	eax,0FFFFh
	cmp	eax,SB_LINEUP
	je	.scroll_line_up
	cmp	eax,SB_LINEDOWN
	je	.scroll_line_down
	cmp	eax,SB_PAGEUP
	je	.scroll_page_up
	cmp	eax,SB_PAGEDOWN
	je	.scroll_page_down
	cmp	eax,SB_THUMBTRACK
	je	.scroll_thumb
	cmp	eax,SB_THUMBPOSITION
	je	.scroll_thumb
	xor	eax,eax
	ret
  .scroll_line_up:
	mov	eax,dword [cell_h]
	sar	eax,1
	neg	eax
	fastcall ViewScrollBy,[hwnd],eax
	xor	eax,eax
	ret
  .scroll_line_down:
	mov	eax,dword [cell_h]
	sar	eax,1
	fastcall ViewScrollBy,[hwnd],eax
	xor	eax,eax
	ret
  .scroll_page_up:
	mov	eax,dword [cell_h]
	imul	eax,4
	neg	eax
	fastcall ViewScrollBy,[hwnd],eax
	xor	eax,eax
	ret
  .scroll_page_down:
	mov	eax,dword [cell_h]
	imul	eax,4
	fastcall ViewScrollBy,[hwnd],eax
	xor	eax,eax
	ret
  .scroll_thumb:
	mov	dword [si.cbSize],sizeof.SCROLLINFO
	mov	dword [si.fMask],SIF_TRACKPOS
	invoke	GetScrollInfo,[hwnd],SB_VERT,addr si
	mov	eax,dword [si.nTrackPos]
	mov	dword [scroll_y],eax
	fastcall ClampScroll,[hwnd]
	invoke	InvalidateRect,[hwnd],0,1
	xor	eax,eax
	ret

  .wm_mousewheel:
	movsx	eax,word [wparam+2]
	test	eax,eax
	jg	.wheel_up
	mov	eax,dword [cell_h]
	neg	eax
	jmp	.wheel_ready
  .wheel_up:
	mov	eax,dword [cell_h]
  .wheel_ready:
	sar	eax,1
	neg	eax
	fastcall ViewScrollBy,[hwnd],eax
	xor	eax,eax
	ret

  .wm_lbuttondown:
	movsx	eax,word [lparam]
	mov	dword [x],eax
	movsx	eax,word [lparam+2]
	add	eax,dword [scroll_y]
	mov	dword [y],eax
	cmp	dword [x],0
	jl	.click_done
	cmp	dword [y],0
	jl	.click_done
	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	cdq
	idiv	dword [cell_w]
	test	eax,eax
	jg	.click_cols_ready
	mov	eax,1
  .click_cols_ready:
	mov	dword [cols],eax
	mov	eax,dword [x]
	cdq
	idiv	dword [cell_w]
	mov	dword [col],eax
	mov	eax,dword [y]
	cdq
	idiv	dword [cell_h]
	mov	dword [row],eax
	mov	eax,dword [row]
	imul	eax,dword [cols]
	add	eax,dword [col]
	mov	dword [idx],eax
	cmp	eax,dword [glyph_count]
	jae	.click_done
	mov	rbx,[glyph_codes]
	mov	ecx,dword [rbx+rax*4]
	fastcall InsertSelection,ecx
  .click_done:
	xor	eax,eax
	ret

  .wm_paint:
	fastcall ViewOnPaint,[hwnd]
	xor	eax,eax
	ret
endp

proc WindowProc uses rbx, hwnd,wmsg,wparam,lparam
    locals
	rc RECT
    endl

	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_CREATE
	je	.wm_create
	cmp	edx,WM_COMMAND
	je	.wm_command
	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_ERASEBKGND
	je	.wm_erasebkgnd
	cmp	edx,WM_CTLCOLORSTATIC
	je	.wm_ctlcolorstatic
	cmp	edx,WM_CTLCOLOREDIT
	je	.wm_ctlcoloredit
	cmp	edx,WM_DESTROY
	je	.wm_destroy
  .def_window:
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	fastcall CreateControls,[hwnd]
	fastcall Layout,[hwnd]
	fastcall UpdateAll
	xor	eax,eax
	ret

  .wm_command:
	mov	eax,dword [wparam]
	mov	ecx,eax
	and	ecx,0FFFFh
	shr	eax,16
	cmp	ecx,ID_FONT_COMBO
	je	.font_cmd
	cmp	ecx,ID_SIZE_COMBO
	je	.size_cmd
	cmp	ecx,ID_COPY_BUTTON
	je	.copy_cmd
	cmp	ecx,ID_COLOR_FORE
	je	.fore_cmd
	cmp	ecx,ID_COLOR_BACK
	je	.back_cmd
	cmp	ecx,ID_COLOR_PAIR
	je	.pair_cmd
	cmp	ecx,ID_MERGED_CHECK
	je	.merged_cmd
	xor	eax,eax
	ret
  .font_cmd:
	cmp	eax,CBN_SELCHANGE
	jne	.done_zero
	fastcall OnFontComboChange
	jmp	.done_zero
  .size_cmd:
	cmp	eax,CBN_SELCHANGE
	jne	.done_zero
	fastcall OnSizeComboChange
	jmp	.done_zero
  .copy_cmd:
	fastcall CopyExportText
	jmp	.done_zero
  .fore_cmd:
	fastcall PickColor,addr color_fore
	jmp	.done_zero
  .back_cmd:
	fastcall PickColor,addr color_back
	jmp	.done_zero
  .pair_cmd:
	fastcall PickColor,addr color_pair
	jmp	.done_zero
  .merged_cmd:
	invoke	SendMessageW,[hCheckMerged],BM_GETCHECK,0,0
	cmp	eax,BST_CHECKED
	jne	.merged_off
	mov	dword [show_merged_pairs],1
	jmp	.merged_update
  .merged_off:
	mov	dword [show_merged_pairs],0
  .merged_update:
	mov	dword [scroll_y],0
	fastcall UpdateAll
	jmp	.done_zero

  .wm_size:
	fastcall Layout,[hwnd]
	fastcall UpdateViewScroll,[hView]
	invoke	InvalidateRect,[hView],0,1
	xor	eax,eax
	ret

  .wm_erasebkgnd:
	cmp	qword [hPanelBrush],0
	je	.def_window
	invoke	GetClientRect,[hwnd],addr rc
	invoke	FillRect,[wparam],addr rc,[hPanelBrush]
	mov	eax,1
	ret

  .wm_ctlcolorstatic:
	invoke	SetBkMode,[wparam],TRANSPARENT
	invoke	SetTextColor,[wparam],dword [color_fore]
	mov	rax,[hPanelBrush]
	ret

  .wm_ctlcoloredit:
	invoke	SetTextColor,[wparam],dword [color_fore]
	invoke	SetBkColor,[wparam],dword [color_back]
	mov	rax,[hViewBrush]
	ret

  .wm_destroy:
	fastcall GlyphList_Clear
	cmp	qword [hGlyphFont],0
	je	.no_glyph_font
	invoke	DeleteObject,[hGlyphFont]
  .no_glyph_font:
	cmp	qword [hUIFont],0
	je	.no_ui_font
	invoke	DeleteObject,[hUIFont]
  .no_ui_font:
	cmp	qword [hMonoFont],0
	je	.no_mono_font
	invoke	DeleteObject,[hMonoFont]
  .no_mono_font:
	cmp	qword [hPanelBrush],0
	je	.no_panel_brush
	invoke	DeleteObject,[hPanelBrush]
  .no_panel_brush:
	cmp	qword [hViewBrush],0
	je	.no_view_brush
	invoke	DeleteObject,[hViewBrush]
  .no_view_brush:
	invoke	PostQuitMessage,0
  .done_zero:
	xor	eax,eax
	ret
endp

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[main_wc.hInstance],rax
	mov	[view_wc.hInstance],rax

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[main_wc.hCursor],rax
	mov	[view_wc.hCursor],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[main_wc.hIcon],rax
	mov	[main_wc.hIconSm],rax

	invoke	RegisterClassExW,addr view_wc
	test	rax,rax
	jz	.fatal
	invoke	RegisterClassExW,addr main_wc
	test	rax,rax
	jz	.fatal

	invoke	CreateWindowExW,0,main_class,app_name,\
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,\
		CW_USEDEFAULT,CW_USEDEFAULT,1120,760,\
		0,0,[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	.fatal
	invoke	ShowWindow,[hMain],SW_SHOWDEFAULT
	invoke	UpdateWindow,[hMain]

  .message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	.exit
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	.message_loop

  .exit:
	invoke	ExitProcess,[msg.wParam]

  .fatal:
	invoke	MessageBoxW,0,fatal_text,app_name,MB_ICONERROR
	invoke	ExitProcess,1
endp
