; uwpchar.asm - assembly icon-font browser/export sample.

include 'windows.inc'
include 'resource.h'
include 'font_icons.inc'
include 'uwpchar_data.inc'

HEAP_ZERO_MEMORY		= 0008h
GGI_MARK_NONEXISTING_GLYPHS	= 0001h
WHEEL_DELTA			= 120

UWP_VIEW_BG		= 0141212h
UWP_PANEL_BG		= 026201Ch
UWP_TEXT		= 0F7F2EEh
UWP_MUTED		= 0B2A49Ah
UWP_PAIR_FILL		= 0FFD27Eh
UWP_MIN_WINDOW_W	= 760
UWP_MIN_WINDOW_H	= 500
UWP_SCAN_MIN		= 0E000h
UWP_SCAN_MAX		= 0F8CCh
UWP_TAB_OUTPUT		= 0
UWP_TAB_COMPLEX		= 1
UWP_COMPLEX_MAX_LAYERS	= 32
UWP_HEART		= 0EB51h
UWP_HEART_FILL		= 0EB52h
UWP_HEART_RED		= 0000DCh
VK_OEM_PLUS		= 0BBh
VK_OEM_MINUS		= 0BDh
UWP_VKEY_HANDLED	= -2
UWP_VKEY_DEFAULT	= -1

define __GLOBAL_DATA__ uwpchar_data
define __GLOBAL_BSS__ uwpchar_bss

macro uwpchar_data
	view_class	GLOBWSTR 'UwpCharAsmView',0
	complex_view_class GLOBWSTR 'UwpCharComplexView',0

	font_mdl2	GLOBWSTR 'Segoe MDL2 Assets',0
	font_fluent	GLOBWSTR 'Segoe Fluent Icons',0
	font_mdl2_ns	GLOBWSTR 'SegoeMDL2',0
	font_fluent_ns	GLOBWSTR 'SegoeFluent',0
	tab_output_text GLOBWSTR 'Output',0
	tab_complex_text GLOBWSTR 'Complex',0
	menu_append_text GLOBWSTR 'Append to output',0
	menu_color_text GLOBWSTR 'Layer color',0
	menu_move_up_text GLOBWSTR 'Move layer up',0
	menu_move_down_text GLOBWSTR 'Move layer down',0
	menu_remove_text GLOBWSTR 'Remove layer',0
	menu_clear_text GLOBWSTR 'Clear layers',0
	code_name_fmt GLOBWSTR 'U+%04X, %s',0

	insert_single_fmt GLOBWSTR 13,10,'namespace %s',13,10,9,'%s := 0%04Xh',13,10,'end namespace',0
	insert_pair_fmt GLOBWSTR 13,10,'namespace %s',13,10,9,'%s := 0%04Xh',13,10,\
		9,'%s := 0%04Xh ; layered pair',13,10,'end namespace',0
	complex_export_begin GLOBWSTR 13,10,'icon_layers:',13,10,0
	complex_export_layer_fmt GLOBWSTR 9,'FONTICON_LAYER glyph: 0%04Xh, color: %08Xh ; %s',13,10,0
	complex_export_end GLOBWSTR 'icon_layer_count = ($ - icon_layers) / sizeof.FONTICON_LAYER',13,10,0
	edit_seed	GLOBWSTR '; Click glyphs to append fasm namespace constants.',13,10,\
		'; MDL2 Fill/Solid companions export as two-layer pairs.',0

	align 8
	size_values	dd 16,20,24,32,40,48,64
	icc INITCOMMONCONTROLSEX dwSize: sizeof.INITCOMMONCONTROLSEX,\
		dwICC: ICC_TAB_CLASSES

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

	complex_view_wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: ComplexViewProc,\
		cbClsExtra: 0,\
		cbWndExtra: 0,\
		hInstance: 0,\
		hIcon: 0,\
		hCursor: 0,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszMenuName: 0,\
		lpszClassName: complex_view_class,\
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
	hCheckLegacy	dq ?
	hLabelFilter	dq ?
	hFilter		dq ?
	hModeTab	dq ?
	hComplexView	dq ?
	hComplexList	dq ?
	hGlyphFont	dq ?
	hUIFont	dq ?
	hMonoFont	dq ?
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
	show_legacy	dd ?
	mode_tab	dd ?
	complex_count	dd ?
	complex_selected dd ?
	pair_count dd ?
	color_fore	dd ?
	color_back	dd ?
	color_pair	dd ?
	custom_colors	dd 16 dup ?
	complex_layers rb sizeof.FONTICON_LAYER * UWP_COMPLEX_MAX_LAYERS
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

proc CurrentNamespace
	cmp	dword [current_font],UWPCHAR_FONT_FLUENT
	je	.fluent
	lea	rax,[font_mdl2_ns]
	ret
  .fluent:
	lea	rax,[font_fluent_ns]
	ret
endp

proc BuildGlyphNameWide code,destp,cap
	mov	dword [code],ecx
	mov	[destp],rdx
	mov	dword [cap],r8d
	fastcall FindName,dword [code]
	test	rax,rax
	jz	.fallback_name
	fastcall CopyAsciiToWide,[destp],rax,dword [cap]
	ret
  .fallback_name:
	cmp	dword [code],0E000h
	jb	.fallback_u
	cmp	dword [code],0F8FFh
	ja	.fallback_u
	invoke	wsprintfW,[destp],'ICON_%04X',dword [code]
	mov	rax,[destp]
	ret
  .fallback_u:
	invoke	wsprintfW,[destp],'U_%04X',dword [code]
	mov	rax,[destp]
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

proc AsciiLen strp
	mov	[strp],rcx
	mov	rdx,[strp]
	xor	eax,eax
  .scan:
	cmp	byte [rdx+rax],0
	je	.done
	inc	eax
	jmp	.scan
  .done:
	ret
endp

proc AsciiCompare uses rsi rdi, leftp,rightp
	mov	[leftp],rcx
	mov	[rightp],rdx
	mov	rsi,[leftp]
	mov	rdi,[rightp]
  .compare:
	movzx	eax,byte [rsi]
	movzx	edx,byte [rdi]
	cmp	eax,edx
	jb	.less
	ja	.greater
	test	eax,eax
	jz	.equal
	inc	rsi
	inc	rdi
	jmp	.compare
  .less:
	mov	eax,-1
	ret
  .greater:
	mov	eax,1
	ret
  .equal:
	xor	eax,eax
	ret
endp

proc NameContainsFilter uses rbx rsi rdi, namep,filterp,filter_len
	mov	[namep],rcx
	mov	[filterp],rdx
	mov	dword [filter_len],r8d
	cmp	dword [filter_len],0
	je	.yes
	mov	rbx,[namep]
  .start:
	mov	al,byte [rbx]
	test	al,al
	jz	.no
	mov	rsi,rbx
	mov	rdi,[filterp]
	mov	ecx,dword [filter_len]
  .compare:
	test	ecx,ecx
	jz	.yes
	mov	al,byte [rsi]
	test	al,al
	jz	.next_start
	movzx	edx,word [rdi]
	cmp	edx,7Fh
	ja	.next_start
	movzx	eax,al
	cmp	eax,'A'
	jb	.name_ready
	cmp	eax,'Z'
	ja	.name_ready
	add	eax,20h
  .name_ready:
	cmp	edx,'A'
	jb	.filter_ready
	cmp	edx,'Z'
	ja	.filter_ready
	add	edx,20h
  .filter_ready:
	cmp	eax,edx
	jne	.next_start
	inc	rsi
	add	rdi,2
	dec	ecx
	jmp	.compare
  .next_start:
	inc	rbx
	jmp	.start
  .yes:
	mov	eax,1
	ret
  .no:
	xor	eax,eax
	ret
endp

proc AsciiEndsWith uses rsi rdi, namep,suffixp
    locals
	name_len dd ?
	suffix_len dd ?
	root_len dd ?
    endl

	mov	[namep],rcx
	mov	[suffixp],rdx
	fastcall AsciiLen,[namep]
	mov	dword [name_len],eax
	fastcall AsciiLen,[suffixp]
	mov	dword [suffix_len],eax
	cmp	dword [name_len],eax
	jb	.no
	mov	ecx,dword [name_len]
	sub	ecx,dword [suffix_len]
	mov	dword [root_len],ecx
	mov	rsi,[namep]
	add	rsi,rcx
	mov	rdi,[suffixp]
	mov	ecx,dword [suffix_len]
  .compare:
	test	ecx,ecx
	jz	.yes
	mov	al,byte [rsi]
	cmp	al,byte [rdi]
	jne	.no
	inc	rsi
	inc	rdi
	dec	ecx
	jmp	.compare
  .yes:
	mov	eax,dword [root_len]
	ret
  .no:
	mov	eax,-1
	ret
endp

proc BuildAsciiCandidate uses rsi rdi, destp,srcp,prefix_len,suffixp
	mov	[destp],rcx
	mov	[srcp],rdx
	mov	dword [prefix_len],r8d
	mov	[suffixp],r9
	mov	rdi,[destp]
	mov	rsi,[srcp]
	mov	ecx,dword [prefix_len]
  .copy_prefix:
	test	ecx,ecx
	jz	.copy_suffix_ready
	mov	al,byte [rsi]
	mov	byte [rdi],al
	inc	rsi
	inc	rdi
	dec	ecx
	jmp	.copy_prefix
  .copy_suffix_ready:
	mov	rsi,[suffixp]
	test	rsi,rsi
	jz	.terminate
  .copy_suffix:
	mov	al,byte [rsi]
	test	al,al
	jz	.terminate
	mov	byte [rdi],al
	inc	rsi
	inc	rdi
	jmp	.copy_suffix
  .terminate:
	mov	byte [rdi],0
	mov	rax,[destp]
	ret
endp

proc FindName code
	mov	dword [code],ecx
	mov	eax,dword [code]
	sub	eax,uwpchar_code_min
	jc	.not_found
	cmp	eax,uwpchar_code_range
	jae	.not_found
	bt	dword [uwpchar_codes],eax
	jnc	.not_found
	mov	edx,eax
	shr	edx,uwpchar_name_block_shift
	movzx	edx,word [uwpchar_name_block_offsets+rdx*2]
	cmp	edx,uwpchar_name_missing
	je	.not_found
	and	eax,uwpchar_name_block_mask
	lea	rcx,[uwpchar_names]
	add	rcx,rdx
	movzx	eax,word [rcx+rax*2]
	cmp	eax,uwpchar_name_missing
	je	.not_found
	lea	rdx,[uwpchar_name_strings]
	add	rax,rdx
	add	rax,uwpchar_name_text_offset
	ret
  .not_found:
	xor	eax,eax
	ret
endp

proc FindCodeByName uses rbx, namep
    locals
	low	dd ?
	high	dd ?
	mid	dd ?
	table_namep dq ?
    endl

	mov	[namep],rcx
	mov	dword [low],0
	mov	dword [high],uwpchar_name_count
	lea	rbx,[uwpchar_name_index]
  .loop:
	mov	eax,dword [low]
	cmp	eax,dword [high]
	jae	.not_found
	add	eax,dword [high]
	shr	eax,1
	mov	dword [mid],eax
	movzx	edx,word [rbx+rax*2]
	lea	rax,[uwpchar_name_strings]
	add	rax,rdx
	lea	rdx,[rax+uwpchar_name_text_offset]
	mov	[table_namep],rdx
	fastcall AsciiCompare,[namep],[table_namep]
	test	eax,eax
	jz	.found
	jl	.before_mid
	mov	eax,dword [mid]
	inc	eax
	mov	dword [low],eax
	jmp	.loop
  .before_mid:
	mov	eax,dword [mid]
	mov	dword [high],eax
	jmp	.loop
  .found:
	mov	eax,dword [mid]
	movzx	edx,word [rbx+rax*2]
	lea	rax,[uwpchar_name_strings]
	add	rax,rdx
	movzx	eax,word [rax]
	ret
  .not_found:
	xor	eax,eax
	ret
endp

proc FindMdl2PairFill uses rbx, code,namep,base_codep,base_namepp
    locals
	root_len dd ?
	base_code dd ?
	candidate rb 256
    endl

	mov	dword [code],ecx
	mov	[namep],rdx
	mov	[base_codep],r8
	mov	[base_namepp],r9

	fastcall AsciiEndsWith,[namep],<A,'Fill'>
	cmp	eax,-1
	jne	.try_root
	fastcall AsciiEndsWith,[namep],<A,'Filled'>
	cmp	eax,-1
	jne	.try_root
	fastcall AsciiEndsWith,[namep],<A,'Solid'>
	cmp	eax,-1
	je	.fail

  .try_root:
	test	eax,eax
	jle	.fail
	mov	dword [root_len],eax
	fastcall BuildAsciiCandidate,addr candidate,[namep],dword [root_len],0
	fastcall FindCodeByName,addr candidate
	test	eax,eax
	jnz	.found_base
	fastcall BuildAsciiCandidate,addr candidate,[namep],dword [root_len],<A,'Outline'>
	fastcall FindCodeByName,addr candidate
	test	eax,eax
	jz	.fail

  .found_base:
	mov	dword [base_code],eax
	mov	rbx,[base_codep]
	mov	dword [rbx],eax
	fastcall FindName,dword [base_code]
	mov	rbx,[base_namepp]
	mov	[rbx],rax
	mov	eax,1
	ret

  .fail:
	xor	eax,eax
	ret
endp

proc FindMdl2FillForRoot uses rbx, rootp,root_len,fill_codep,fill_namepp
    locals
	fill_code dd ?
	candidate rb 256
    endl

	mov	[rootp],rcx
	mov	dword [root_len],edx
	mov	[fill_codep],r8
	mov	[fill_namepp],r9

	fastcall BuildAsciiCandidate,addr candidate,[rootp],dword [root_len],<A,'Fill'>
	fastcall FindCodeByName,addr candidate
	test	eax,eax
	jnz	.found_fill
	fastcall BuildAsciiCandidate,addr candidate,[rootp],dword [root_len],<A,'Filled'>
	fastcall FindCodeByName,addr candidate
	test	eax,eax
	jnz	.found_fill
	fastcall BuildAsciiCandidate,addr candidate,[rootp],dword [root_len],<A,'Solid'>
	fastcall FindCodeByName,addr candidate
	test	eax,eax
	jz	.fail

  .found_fill:
	mov	dword [fill_code],eax
	mov	rbx,[fill_codep]
	mov	dword [rbx],eax
	fastcall FindName,dword [fill_code]
	mov	rbx,[fill_namepp]
	mov	[rbx],rax
	mov	eax,1
	ret

  .fail:
	xor	eax,eax
	ret
endp

proc FindMdl2Pair uses rbx, code,namep,base_codep,fill_codep,base_namepp,fill_namepp
    locals
	base_namep dq ?
	fill_namep dq ?
	base_code dd ?
	fill_code dd ?
	root_len dd ?
    endl

	mov	dword [code],ecx
	mov	[namep],rdx
	mov	[base_codep],r8
	mov	[fill_codep],r9

	fastcall FindMdl2PairFill,dword [code],[namep],addr base_code,addr base_namep
	test	eax,eax
	jz	.try_base
	mov	eax,dword [code]
	mov	dword [fill_code],eax
	mov	rax,[namep]
	mov	[fill_namep],rax
	jmp	.store_pair

  .try_base:
	fastcall AsciiEndsWith,[namep],<A,'Outline'>
	cmp	eax,-1
	je	.try_name
	mov	dword [root_len],eax
	fastcall FindMdl2FillForRoot,[namep],dword [root_len],addr fill_code,addr fill_namep
	test	eax,eax
	jnz	.base_found

  .try_name:
	fastcall AsciiLen,[namep]
	mov	dword [root_len],eax
	fastcall FindMdl2FillForRoot,[namep],dword [root_len],addr fill_code,addr fill_namep
	test	eax,eax
	jz	.fail

  .base_found:
	mov	eax,dword [code]
	mov	dword [base_code],eax
	mov	rax,[namep]
	mov	[base_namep],rax

  .store_pair:
	mov	rbx,[base_codep]
	mov	eax,dword [base_code]
	mov	dword [rbx],eax
	mov	rbx,[fill_codep]
	mov	eax,dword [fill_code]
	mov	dword [rbx],eax
	mov	rbx,[base_namepp]
	mov	rax,[base_namep]
	mov	[rbx],rax
	mov	rbx,[fill_namepp]
	mov	rax,[fill_namep]
	mov	[rbx],rax
	mov	eax,1
	ret

  .fail:
	xor	eax,eax
	ret
endp

proc CountNamePairs uses rbx rsi
    locals
	base_code dd ?
	base_namep dq ?
	namep dq ?
	pairs dd ?
    endl

	lea	rsi,[uwpchar_name_index]
	mov	ebx,uwpchar_name_count
	mov	dword [pairs],0
  .loop:
	test	ebx,ebx
	jz	.done
	movzx	eax,word [rsi]
	lea	rdx,[uwpchar_name_strings]
	add	rdx,rax
	movzx	ecx,word [rdx]
	add	rdx,uwpchar_name_text_offset
	mov	[namep],rdx
	fastcall FindMdl2PairFill,ecx,[namep],addr base_code,addr base_namep
	test	eax,eax
	jz	.next
	inc	dword [pairs]
  .next:
	add	rsi,2
	dec	ebx
	jmp	.loop
  .done:
	mov	eax,dword [pairs]
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
	cmp	qword [hComplexView],0
	je	.done
	invoke	InvalidateRect,[hComplexView],0,1
  .done:
	ret
endp

proc ChooseColorValue uses rdi, colorp
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
	mov	eax,1
	ret
  .done:
	xor	eax,eax
	ret
endp

proc PickColor colorp
	mov	[colorp],rcx
	fastcall ChooseColorValue,[colorp]
	test	eax,eax
	jz	.done
	fastcall RebuildColorBrush
	invoke	InvalidateRect,[hView],0,1
  .done:
	ret
endp

proc ComplexUpdateSelectionState
	mov	eax,dword [complex_selected]
	cmp	eax,0
	jl	.disabled
	cmp	eax,dword [complex_count]
	jae	.disabled
	ret
  .disabled:
	mov	dword [complex_selected],-1
	ret
endp

proc ComplexLayer_RebuildList uses rsi
    locals
	idx dd ?
	name_w rw 256
	line rw 320
    endl

	cmp	qword [hComplexList],0
	je	.selection
	invoke	SendMessageW,[hComplexList],LB_RESETCONTENT,0,0
	lea	rsi,[complex_layers]
	mov	dword [idx],0
  .loop:
	mov	eax,dword [idx]
	cmp	eax,dword [complex_count]
	jae	.selection
	fastcall BuildGlyphNameWide,dword [rsi+FONTICON_LAYER.glyph],addr name_w,256
	invoke	wsprintfW,addr line,code_name_fmt,dword [rsi+FONTICON_LAYER.glyph],addr name_w
	invoke	SendMessageW,[hComplexList],LB_ADDSTRING,0,addr line
	add	rsi,sizeof.FONTICON_LAYER
	inc	dword [idx]
	jmp	.loop
  .selection:
	fastcall ComplexUpdateSelectionState
	cmp	qword [hComplexList],0
	je	.no_selection
	cmp	dword [complex_selected],0
	jl	.no_selection
	invoke	SendMessageW,[hComplexList],LB_SETCURSEL,dword [complex_selected],0
  .no_selection:
	cmp	qword [hComplexView],0
	je	.done
	invoke	InvalidateRect,[hComplexView],0,1
  .done:
	ret
endp

proc ComplexLayer_Clear
	mov	dword [complex_count],0
	mov	dword [complex_selected],-1
	cmp	qword [hComplexList],0
	je	.no_list
	invoke	SendMessageW,[hComplexList],LB_RESETCONTENT,0,0
  .no_list:
	fastcall ComplexUpdateSelectionState
	cmp	qword [hComplexView],0
	je	.done
	invoke	InvalidateRect,[hComplexView],0,1
  .done:
	ret
endp

proc ComplexLayer_Select index
	mov	dword [index],ecx
	mov	eax,dword [index]
	mov	dword [complex_selected],eax
	cmp	eax,0
	jl	.no_list
	cmp	eax,dword [complex_count]
	jae	.no_list
	invoke	SendMessageW,[hComplexList],LB_SETCURSEL,dword [index],0
  .no_list:
	fastcall ComplexUpdateSelectionState
	ret
endp

proc ComplexLayer_Add uses rbx, code,color
    locals
	idx dd ?
    endl

	mov	dword [code],ecx
	mov	dword [color],edx
	mov	eax,dword [complex_count]
	cmp	eax,UWP_COMPLEX_MAX_LAYERS
	jae	.fail
	mov	dword [idx],eax
	lea	rbx,[complex_layers]
	mov	ecx,dword [code]
	mov	dword [rbx+rax*sizeof.FONTICON_LAYER+FONTICON_LAYER.glyph],ecx
	mov	ecx,dword [color]
	mov	dword [rbx+rax*sizeof.FONTICON_LAYER+FONTICON_LAYER.color],ecx
	inc	dword [complex_count]
	mov	eax,dword [idx]
	mov	dword [complex_selected],eax
	fastcall ComplexLayer_RebuildList
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc ComplexLayer_RemoveSelected uses rbx, index
	mov	eax,dword [complex_selected]
	mov	dword [index],eax
	cmp	eax,0
	jl	.done
	cmp	eax,dword [complex_count]
	jae	.done
	mov	ecx,dword [index]
	mov	edx,dword [complex_count]
	dec	edx
	cmp	ecx,edx
	jae	.shift_done
	lea	rbx,[complex_layers]
  .shift_loop:
	mov	rax,qword [rbx+rcx*sizeof.FONTICON_LAYER+sizeof.FONTICON_LAYER]
	mov	qword [rbx+rcx*sizeof.FONTICON_LAYER],rax
	inc	ecx
	cmp	ecx,edx
	jb	.shift_loop
  .shift_done:
	dec	dword [complex_count]
	mov	eax,dword [complex_count]
	test	eax,eax
	jz	.no_selection
	mov	ecx,dword [index]
	cmp	ecx,eax
	jb	.select_ready
	dec	eax
	mov	ecx,eax
  .select_ready:
	mov	dword [complex_selected],ecx
	jmp	.invalidate
  .no_selection:
	mov	dword [complex_selected],-1
  .invalidate:
	fastcall ComplexLayer_RebuildList
  .done:
	ret
endp

proc ComplexLayer_MoveSelected uses rbx, delta
    locals
	index dd ?
	target dd ?
	temp dq ?
    endl

	mov	dword [delta],ecx
	mov	eax,dword [complex_selected]
	mov	dword [index],eax
	cmp	eax,0
	jl	.done
	cmp	eax,dword [complex_count]
	jae	.done
	add	eax,dword [delta]
	mov	dword [target],eax
	cmp	eax,0
	jl	.done
	cmp	eax,dword [complex_count]
	jae	.done
	lea	rbx,[complex_layers]
	mov	ecx,dword [index]
	mov	rax,qword [rbx+rcx*sizeof.FONTICON_LAYER]
	mov	[temp],rax
	mov	edx,dword [target]
	mov	rax,qword [rbx+rdx*sizeof.FONTICON_LAYER]
	mov	qword [rbx+rcx*sizeof.FONTICON_LAYER],rax
	mov	rax,[temp]
	mov	qword [rbx+rdx*sizeof.FONTICON_LAYER],rax
	mov	eax,dword [target]
	mov	dword [complex_selected],eax
	fastcall ComplexLayer_RebuildList
  .done:
	ret
endp

proc ComplexPickSelectedColor uses rbx
	mov	eax,dword [complex_selected]
	cmp	eax,0
	jl	.done
	cmp	eax,dword [complex_count]
	jae	.done
	lea	rbx,[complex_layers]
	lea	rcx,[rbx+rax*sizeof.FONTICON_LAYER+FONTICON_LAYER.color]
	fastcall ChooseColorValue,rcx
	test	eax,eax
	jz	.done
	invoke	InvalidateRect,[hComplexView],0,1
  .done:
	ret
endp

proc ComplexListKey key
	mov	dword [key],ecx
	cmp	dword [key],VK_RETURN
	je	.color
	cmp	dword [key],VK_SPACE
	je	.color
	cmp	dword [key],VK_DELETE
	je	.remove
	cmp	dword [key],VK_BACK
	je	.remove
	cmp	dword [key],VK_ADD
	je	.move_up
	cmp	dword [key],VK_OEM_PLUS
	je	.move_up
	cmp	dword [key],VK_SUBTRACT
	je	.move_down
	cmp	dword [key],VK_OEM_MINUS
	je	.move_down
	cmp	dword [key],VK_UP
	je	.ctrl_up
	cmp	dword [key],VK_DOWN
	je	.ctrl_down
	xor	eax,eax
	ret
  .ctrl_up:
	invoke	GetKeyState,VK_CONTROL
	test	ax,8000h
	jz	.default
  .move_up:
	fastcall ComplexLayer_MoveSelected,-1
	mov	eax,1
	ret
  .ctrl_down:
	invoke	GetKeyState,VK_CONTROL
	test	ax,8000h
	jz	.default
  .move_down:
	fastcall ComplexLayer_MoveSelected,1
	mov	eax,1
	ret
  .color:
	fastcall ComplexPickSelectedColor
	mov	eax,1
	ret
  .remove:
	fastcall ComplexLayer_RemoveSelected
	mov	eax,1
	ret
  .default:
	xor	eax,eax
	ret
endp

proc ComplexShowContextMenu uses rbx, screen_x,screen_y
    locals
	popup_handle dq ?
	flags dd ?
	cmd dd ?
    endl

	mov	dword [screen_x],ecx
	mov	dword [screen_y],edx
	invoke	CreatePopupMenu
	mov	[popup_handle],rax
	test	rax,rax
	jz	.done
	invoke	AppendMenuW,[popup_handle],MF_STRING,ID_COMPLEX_APPEND,addr menu_append_text
	invoke	AppendMenuW,[popup_handle],MF_SEPARATOR,0,0
	mov	dword [flags],MF_STRING
	cmp	dword [complex_selected],0
	jl	.disable_layer_items
	mov	eax,dword [complex_selected]
	cmp	eax,dword [complex_count]
	jb	.layer_flags_ready
  .disable_layer_items:
	or	dword [flags],MF_GRAYED
  .layer_flags_ready:
	invoke	AppendMenuW,[popup_handle],dword [flags],ID_COMPLEX_COLOR,addr menu_color_text
	invoke	AppendMenuW,[popup_handle],dword [flags],ID_COMPLEX_MOVE_UP,addr menu_move_up_text
	invoke	AppendMenuW,[popup_handle],dword [flags],ID_COMPLEX_MOVE_DOWN,addr menu_move_down_text
	invoke	AppendMenuW,[popup_handle],dword [flags],ID_COMPLEX_REMOVE,addr menu_remove_text
	invoke	AppendMenuW,[popup_handle],MF_SEPARATOR,0,0
	mov	dword [flags],MF_STRING
	cmp	dword [complex_count],0
	jne	.clear_ready
	or	dword [flags],MF_GRAYED
  .clear_ready:
	invoke	AppendMenuW,[popup_handle],dword [flags],ID_COMPLEX_CLEAR,addr menu_clear_text
	invoke	TrackPopupMenu,[popup_handle],TPM_RIGHTBUTTON or TPM_RETURNCMD,\
		dword [screen_x],dword [screen_y],0,[hMain],0
	mov	dword [cmd],eax
	invoke	DestroyMenu,[popup_handle]
	mov	eax,dword [cmd]
	cmp	eax,ID_COMPLEX_APPEND
	je	.append
	cmp	eax,ID_COMPLEX_COLOR
	je	.color
	cmp	eax,ID_COMPLEX_MOVE_UP
	je	.move_up
	cmp	eax,ID_COMPLEX_MOVE_DOWN
	je	.move_down
	cmp	eax,ID_COMPLEX_REMOVE
	je	.remove
	cmp	eax,ID_COMPLEX_CLEAR
	je	.clear
	jmp	.done
  .append:
	fastcall AppendComplexExport
	jmp	.done
  .color:
	fastcall ComplexPickSelectedColor
	jmp	.done
  .move_up:
	fastcall ComplexLayer_MoveSelected,-1
	jmp	.done
  .move_down:
	fastcall ComplexLayer_MoveSelected,1
	jmp	.done
  .remove:
	fastcall ComplexLayer_RemoveSelected
	jmp	.done
  .clear:
	fastcall ComplexLayer_Clear
  .done:
	ret
endp

proc AddSelectionToComplex uses rbx, code
    locals
	namep dq ?
	base_namep dq ?
	fill_namep dq ?
	base_code dd ?
	fill_code dd ?
    endl

	mov	dword [code],ecx
	fastcall FindName,dword [code]
	mov	[namep],rax
	test	rax,rax
	jz	.single
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.single
	cmp	dword [show_merged_pairs],0
	je	.single
	mov	eax,dword [complex_count]
	cmp	eax,UWP_COMPLEX_MAX_LAYERS - 1
	jae	.single
	fastcall FindMdl2Pair,dword [code],[namep],addr base_code,addr fill_code,addr base_namep,addr fill_namep
	test	eax,eax
	jz	.single
	fastcall ComplexLayer_Add,dword [fill_code],dword [color_pair]
	fastcall ComplexLayer_Add,dword [base_code],dword [color_fore]
	ret
  .single:
	fastcall ComplexLayer_Add,dword [code],dword [color_fore]
	ret
endp

proc ComplexSeedHeart
	fastcall ComplexLayer_Clear
	fastcall ComplexLayer_Add,UWP_HEART_FILL,UWP_HEART_RED
	fastcall ComplexLayer_Add,UWP_HEART,0FFFFFFh
	ret
endp

proc AppendComplexExport uses rbx rsi
    locals
	idx dd ?
	name_w rw 256
	line rw 384
    endl

	cmp	dword [complex_count],0
	je	.done
	fastcall AppendToEdit,addr complex_export_begin
	lea	rsi,[complex_layers]
	mov	dword [idx],0
  .loop:
	mov	eax,dword [idx]
	cmp	eax,dword [complex_count]
	jae	.finish
	fastcall BuildGlyphNameWide,dword [rsi+FONTICON_LAYER.glyph],addr name_w,256
	invoke	wsprintfW,addr line,complex_export_layer_fmt,\
		dword [rsi+FONTICON_LAYER.glyph],dword [rsi+FONTICON_LAYER.color],addr name_w
	fastcall AppendToEdit,addr line
	add	rsi,sizeof.FONTICON_LAYER
	inc	dword [idx]
	jmp	.loop
  .finish:
	fastcall AppendToEdit,addr complex_export_end
	invoke	SendMessageW,[hModeTab],TCM_SETCURSEL,UWP_TAB_OUTPUT,0
	fastcall SetModeTab,UWP_TAB_OUTPUT
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
	base_code dd ?
	base_namep dq ?
	fill_code dd ?
	fill_namep dq ?
	namep dq ?
	filter_hwnd dq ?
	count	dd ?
	filter_len dd ?
	filter_text rw 128
	chars	rw 512
	glyphs	rw 512
    endl

	fastcall GlyphList_Clear
	fastcall UpdateGlyphFont
	fastcall UpdateCellMetrics
	mov	dword [filter_len],0
	invoke	GetDlgItem,[hMain],ID_FILTER_EDIT
	mov	[filter_hwnd],rax
	test	rax,rax
	jz	.filter_ready
	invoke	GetWindowTextW,[filter_hwnd],addr filter_text,128
	mov	dword [filter_len],eax
  .filter_ready:

	invoke	GetDC,0
	mov	[hdc],rax
	test	rax,rax
	jz	.done
	invoke	SelectObject,[hdc],[hGlyphFont]
	mov	[old_font],rax

	mov	dword [code],UWP_SCAN_MIN
  .chunk:
	cmp	dword [code],UWP_SCAN_MAX
	ja	.scan_done
	mov	dword [count],0

  .fill_chunk:
	cmp	dword [count],512
	jae	.have_chunk
	mov	eax,dword [code]
	cmp	eax,UWP_SCAN_MAX
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
	mov	qword [namep],0
	cmp	dword [show_legacy],0
	jne	.legacy_ready
	mov	eax,dword [candidate]
	cmp	eax,0E000h
	jb	.legacy_ready
	cmp	eax,0E5FFh
	jbe	.next_glyph
  .legacy_ready:
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.filter_check
	fastcall FindName,dword [candidate]
	mov	[namep],rax
	test	rax,rax
	jz	.filter_check
	cmp	dword [show_merged_pairs],0
	je	.filter_check
	fastcall FindMdl2PairFill,dword [candidate],[namep],addr base_code,addr base_namep
	test	eax,eax
	jnz	.next_glyph
  .filter_check:
	cmp	dword [filter_len],0
	je	.push_candidate
	cmp	qword [namep],0
	jne	.have_name
	fastcall FindName,dword [candidate]
	mov	[namep],rax
  .have_name:
	cmp	qword [namep],0
	je	.next_glyph
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.match_single
	cmp	dword [show_merged_pairs],0
	je	.match_single
	fastcall FindMdl2Pair,dword [candidate],[namep],addr base_code,addr fill_code,addr base_namep,addr fill_namep
	test	eax,eax
	jz	.match_single
	fastcall NameContainsFilter,[base_namep],addr filter_text,dword [filter_len]
	test	eax,eax
	jnz	.push_candidate
	fastcall NameContainsFilter,[fill_namep],addr filter_text,dword [filter_len]
	test	eax,eax
	jnz	.push_candidate
	jmp	.next_glyph
  .match_single:
	fastcall NameContainsFilter,[namep],addr filter_text,dword [filter_len]
	test	eax,eax
	jz	.next_glyph
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

proc UpdateStatus
    locals
	text rw 160
    endl

	invoke	wsprintfW,addr text,'Glyphs: %u | Names: %u | Pairs: %u',dword [glyph_count],uwpchar_name_count,dword [pair_count]
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
	namep dq ?
	base_namep dq ?
	fill_namep dq ?
	base_code dd ?
	fill_code dd ?
    endl

	mov	[hdc],rcx
	mov	dword [code],edx
	mov	[rectp],r8
	fastcall FindName,dword [code]
	mov	[namep],rax
	test	rax,rax
	jz	.single
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.single
	cmp	dword [show_merged_pairs],0
	je	.single
	fastcall FindMdl2Pair,dword [code],[namep],addr base_code,addr fill_code,addr base_namep,addr fill_namep
	test	eax,eax
	jz	.single
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

	invoke	wsprintfW,addr label_text,'U+%04X',dword [code]
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

proc ComplexViewOnPaint uses rbx rsi rdi, hwnd
    locals
	ps PAINTSTRUCT
	rc RECT
	draw_rect RECT
	width dd ?
	height dd ?
	size dd ?
	hPreviewFont dq ?
    endl

	mov	[hwnd],rcx
	invoke	BeginPaint,[hwnd],addr ps
	mov	rdi,rax
	invoke	GetClientRect,[hwnd],addr rc
	invoke	FillRect,rdi,addr rc,[hViewBrush]
	cmp	dword [complex_count],0
	je	.paint_done
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
	sub	eax,28
	cmp	eax,32
	jge	.have_size
	mov	eax,32
  .have_size:
	mov	dword [size],eax
	mov	eax,dword [width]
	sub	eax,dword [size]
	cdq
	mov	ebx,2
	idiv	ebx
	mov	dword [draw_rect.left],eax
	mov	eax,dword [height]
	sub	eax,dword [size]
	cdq
	idiv	ebx
	mov	dword [draw_rect.top],eax
	mov	eax,dword [draw_rect.left]
	add	eax,dword [size]
	mov	dword [draw_rect.right],eax
	mov	eax,dword [draw_rect.top]
	add	eax,dword [size]
	mov	dword [draw_rect.bottom],eax
	fastcall CurrentFace
	fastcall FontIcon_CreateFontForWindow,[hMain],dword [size],rax
	mov	[hPreviewFont],rax
	test	rax,rax
	jz	.paint_done
	fastcall FontIcon_DrawLayeredGlyphArray,rdi,addr complex_layers,dword [complex_count],addr draw_rect,[hPreviewFont]
	invoke	DeleteObject,[hPreviewFont]
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
	name_ascii dq ?
	base_name_ascii dq ?
	fill_name_ascii dq ?
	namep dq ?
	pair_namep dq ?
	namespacep dq ?
	base_code dd ?
	fill_code dd ?
	fallback rw 32
	name_w rw 256
	pair_name_w rw 256
	line rw 768
    endl

	mov	dword [code],ecx
	fastcall CurrentNamespace
	mov	[namespacep],rax
	fastcall FindName,dword [code]
	mov	[name_ascii],rax
	test	rax,rax
	jz	.fallback_name
	fastcall CopyAsciiToWide,addr name_w,rax,256
	mov	[namep],rax
	mov	qword [pair_namep],0
	cmp	dword [current_font],UWPCHAR_FONT_MDL2
	jne	.single
	cmp	dword [show_merged_pairs],0
	je	.single
	fastcall FindMdl2Pair,dword [code],[name_ascii],addr base_code,addr fill_code,addr base_name_ascii,addr fill_name_ascii
	test	eax,eax
	jz	.single
	fastcall CopyAsciiToWide,addr name_w,[base_name_ascii],256
	mov	[namep],rax
	fastcall CopyAsciiToWide,addr pair_name_w,[fill_name_ascii],256
	mov	[pair_namep],rax
	invoke	wsprintfW,addr line,insert_pair_fmt,[namespacep],\
		[namep],dword [base_code],[pair_namep],dword [fill_code]
	fastcall AppendToEdit,addr line
	ret

  .fallback_name:
	cmp	dword [code],0E000h
	jb	.fallback_u
	cmp	dword [code],0F8FFh
	ja	.fallback_u
	invoke	wsprintfW,addr fallback,'ICON_%04X',dword [code]
	jmp	.fallback_ready
  .fallback_u:
	invoke	wsprintfW,addr fallback,'U_%04X',dword [code]
  .fallback_ready:
	lea	rax,[fallback]
	mov	[namep],rax
  .single:
	invoke	wsprintfW,addr line,insert_single_fmt,[namespacep],[namep],dword [code]
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
	invoke	InvalidateRect,[hComplexView],0,1
	ret
endp

proc InitModeTabs
    locals
	ti TC_ITEM
    endl

	mov	dword [ti.mask],TCIF_TEXT
	lea	rax,[tab_output_text]
	mov	[ti.pszText],rax
	invoke	SendMessageW,[hModeTab],TCM_INSERTITEMW,UWP_TAB_OUTPUT,addr ti
	lea	rax,[tab_complex_text]
	mov	[ti.pszText],rax
	invoke	SendMessageW,[hModeTab],TCM_INSERTITEMW,UWP_TAB_COMPLEX,addr ti
	invoke	SendMessageW,[hModeTab],TCM_SETCURSEL,UWP_TAB_OUTPUT,0
	mov	dword [mode_tab],UWP_TAB_OUTPUT
	ret
endp

proc SetModeTab mode
	mov	dword [mode],ecx
	mov	eax,dword [mode]
	cmp	eax,UWP_TAB_COMPLEX
	je	.complex
	mov	dword [mode_tab],UWP_TAB_OUTPUT
	invoke	ShowWindow,[hEdit],SW_SHOW
	invoke	ShowWindow,[hComplexView],SW_HIDE
	invoke	ShowWindow,[hComplexList],SW_HIDE
	ret
  .complex:
	mov	dword [mode_tab],UWP_TAB_COMPLEX
	invoke	ShowWindow,[hEdit],SW_HIDE
	invoke	ShowWindow,[hComplexView],SW_SHOW
	invoke	ShowWindow,[hComplexList],SW_SHOW
	fastcall ComplexUpdateSelectionState
	invoke	InvalidateRect,[hComplexView],0,1
	ret
endp

proc ApplyFonts
	cmp	qword [hUIFont],0
	jne	.have_ui
	invoke	CreateFontW,-13,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,'Segoe UI'
	mov	[hUIFont],rax
  .have_ui:
	cmp	qword [hMonoFont],0
	jne	.have_mono
	invoke	CreateFontW,-13,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,FIXED_PITCH or FF_DONTCARE,'Consolas'
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
	invoke	SendMessageW,[hCheckLegacy],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hLabelFilter],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hFilter],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hModeTab],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hComplexList],WM_SETFONT,[hUIFont],1
	invoke	SendMessageW,[hEdit],WM_SETFONT,[hMonoFont],1
	ret
endp

proc InitDialogControls hwnd
	mov	[hwnd],rcx
	mov	dword [current_font],UWPCHAR_FONT_MDL2
	mov	dword [font_size],24
	mov	dword [show_merged_pairs],1
	mov	dword [show_legacy],0
	mov	dword [color_fore],UWP_TEXT
	mov	dword [color_back],UWP_VIEW_BG
	mov	dword [color_pair],UWP_PAIR_FILL
	fastcall CountNamePairs
	mov	dword [pair_count],eax
	fastcall UpdateCellMetrics

	invoke	GetDlgItem,[hwnd],IDC_FONT_LABEL
	mov	[hLabelFont],rax
	invoke	GetDlgItem,[hwnd],ID_FONT_COMBO
	mov	[hComboFont],rax
	invoke	GetDlgItem,[hwnd],IDC_SIZE_LABEL
	mov	[hLabelSize],rax
	invoke	GetDlgItem,[hwnd],ID_SIZE_COMBO
	mov	[hComboSize],rax
	invoke	GetDlgItem,[hwnd],ID_COPY_BUTTON
	mov	[hButtonCopy],rax
	invoke	GetDlgItem,[hwnd],ID_STATUS
	mov	[hStatus],rax
	invoke	GetDlgItem,[hwnd],ID_COLOR_FORE
	mov	[hButtonFore],rax
	invoke	GetDlgItem,[hwnd],ID_COLOR_BACK
	mov	[hButtonBack],rax
	invoke	GetDlgItem,[hwnd],ID_COLOR_PAIR
	mov	[hButtonPair],rax
	invoke	GetDlgItem,[hwnd],ID_MERGED_CHECK
	mov	[hCheckMerged],rax
	invoke	SendMessageW,[hCheckMerged],BM_SETCHECK,BST_CHECKED,0
	invoke	GetDlgItem,[hwnd],ID_LEGACY_CHECK
	mov	[hCheckLegacy],rax
	invoke	GetDlgItem,[hwnd],IDC_FILTER_LABEL
	mov	[hLabelFilter],rax
	invoke	GetDlgItem,[hwnd],ID_FILTER_EDIT
	mov	[hFilter],rax
	invoke	SendMessageW,[hFilter],EM_SETLIMITTEXT,127,0
	invoke	GetDlgItem,[hwnd],ID_MODE_TAB
	mov	[hModeTab],rax
	invoke	GetDlgItem,[hwnd],ID_EXPORT_EDIT
	mov	[hEdit],rax
	invoke	SendMessageW,[hEdit],EM_SETLIMITTEXT,0,0
	fastcall SetExportSeed
	invoke	GetDlgItem,[hwnd],ID_COMPLEX_VIEW
	mov	[hComplexView],rax
	invoke	GetDlgItem,[hwnd],ID_COMPLEX_LIST
	mov	[hComplexList],rax
	invoke	GetDlgItem,[hwnd],ID_VIEW
	mov	[hView],rax

	fastcall InitModeTabs
	fastcall ComplexSeedHeart

	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,'Segoe MDL2 Assets'
	invoke	SendMessageW,[hComboFont],CB_ADDSTRING,0,'Segoe Fluent Icons'
	invoke	SendMessageW,[hComboFont],CB_SETCURSEL,0,0

	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'16'
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'20'
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'24'
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'32'
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'40'
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'48'
	invoke	SendMessageW,[hComboSize],CB_ADDSTRING,0,'64'
	invoke	SendMessageW,[hComboSize],CB_SETCURSEL,2,0
	fastcall RebuildColorBrush
	fastcall ApplyFonts
	fastcall SetModeTab,UWP_TAB_OUTPUT
	ret
endp

proc Layout hwnd
    locals
	rc RECT
	width dd ?
	height dd ?
	view_x dd ?
	view_w dd ?
	tab_y dd ?
	tab_h dd ?
	page_x dd ?
	page_y dd ?
	page_w dd ?
	page_h dd ?
	list_h dd ?
	list_y dd ?
	preview_x dd ?
	preview_size dd ?
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
	invoke	MoveWindow,[hCheckMerged],160,44,116,24,1
	invoke	MoveWindow,[hCheckLegacy],280,44,104,24,1
	invoke	MoveWindow,[hLabelFilter],8,80,48,22,1
	invoke	MoveWindow,[hFilter],64,76,320,24,1
	invoke	MoveWindow,[hButtonFore],8,112,64,28,1
	invoke	MoveWindow,[hButtonBack],80,112,64,28,1
	invoke	MoveWindow,[hButtonPair],152,112,64,28,1
	invoke	MoveWindow,[hButtonCopy],232,112,76,28,1
	invoke	MoveWindow,[hStatus],8,148,360,22,1

	mov	dword [tab_y],176
	mov	eax,dword [height]
	sub	eax,dword [tab_y]
	sub	eax,8
	cmp	eax,150
	jge	.tab_h_ready
	mov	eax,150
  .tab_h_ready:
	mov	dword [tab_h],eax
	invoke	MoveWindow,[hModeTab],8,dword [tab_y],376,dword [tab_h],1

	mov	dword [page_x],16
	mov	eax,dword [tab_y]
	add	eax,30
	mov	dword [page_y],eax
	mov	dword [page_w],360
	mov	eax,dword [tab_h]
	sub	eax,38
	cmp	eax,112
	jge	.page_h_ready
	mov	eax,112
  .page_h_ready:
	mov	dword [page_h],eax
	invoke	MoveWindow,[hEdit],dword [page_x],dword [page_y],dword [page_w],dword [page_h],1

	mov	eax,dword [page_h]
	sub	eax,92
	cmp	eax,96
	jge	.preview_min_ready
	mov	eax,96
  .preview_min_ready:
	cmp	eax,dword [page_w]
	jle	.preview_size_ready
	mov	eax,dword [page_w]
  .preview_size_ready:
	mov	dword [preview_size],eax
	mov	eax,dword [page_w]
	sub	eax,dword [preview_size]
	cdq
	mov	ecx,2
	idiv	ecx
	add	eax,dword [page_x]
	mov	dword [preview_x],eax
	invoke	MoveWindow,[hComplexView],dword [preview_x],dword [page_y],\
		dword [preview_size],dword [preview_size],1
	mov	eax,dword [page_y]
	add	eax,dword [preview_size]
	add	eax,8
	mov	dword [list_y],eax
	mov	eax,dword [page_h]
	sub	eax,dword [preview_size]
	sub	eax,8
	cmp	eax,64
	jge	.list_h_ready
	mov	eax,64
  .list_h_ready:
	mov	dword [list_h],eax
	invoke	MoveWindow,[hComplexList],dword [page_x],dword [list_y],dword [page_w],dword [list_h],1

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
	cmp	dword [mode_tab],UWP_TAB_COMPLEX
	jne	.output_click
	fastcall AddSelectionToComplex,ecx
	jmp	.click_done
  .output_click:
	fastcall InsertSelection,ecx
  .click_done:
	xor	eax,eax
	ret

  .wm_paint:
	fastcall ViewOnPaint,[hwnd]
	xor	eax,eax
	ret
endp

proc ComplexViewProc hwnd,wmsg,wparam,lparam
    locals
	pt POINT
	rc RECT
    endl

	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_PAINT
	je	.wm_paint
	cmp	edx,WM_ERASEBKGND
	je	.wm_erasebkgnd
	cmp	edx,WM_LBUTTONUP
	je	.wm_lbuttonup
	cmp	edx,WM_CONTEXTMENU
	je	.wm_contextmenu
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .wm_paint:
	fastcall ComplexViewOnPaint,[hwnd]
	xor	eax,eax
	ret
  .wm_erasebkgnd:
	mov	eax,1
	ret
  .wm_lbuttonup:
	fastcall AppendComplexExport
	xor	eax,eax
	ret
  .wm_contextmenu:
	cmp	qword [lparam],-1
	je	.keyboard_menu
	movsx	eax,word [lparam]
	mov	dword [pt.x],eax
	movsx	eax,word [lparam+2]
	mov	dword [pt.y],eax
	jmp	.show_menu
  .keyboard_menu:
	invoke	GetClientRect,[hwnd],addr rc
	mov	eax,dword [rc.right]
	sub	eax,dword [rc.left]
	sar	eax,1
	mov	dword [pt.x],eax
	mov	eax,dword [rc.bottom]
	sub	eax,dword [rc.top]
	sar	eax,1
	mov	dword [pt.y],eax
	invoke	ClientToScreen,[hwnd],addr pt
  .show_menu:
	fastcall ComplexShowContextMenu,dword [pt.x],dword [pt.y]
	xor	eax,eax
	ret
endp

proc UwpCharDlgProc uses rbx, hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_INITDIALOG
	je	.wm_initdialog
	cmp	edx,WM_COMMAND
	je	.wm_command
	cmp	edx,WM_NOTIFY
	je	.wm_notify
	cmp	edx,WM_VKEYTOITEM
	je	.wm_vkeytoitem
	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_GETMINMAXINFO
	je	.wm_getminmaxinfo
	cmp	edx,WM_CTLCOLOREDIT
	je	.wm_ctlcoloredit
	cmp	edx,WM_CLOSE
	je	.wm_close
	cmp	edx,WM_DESTROY
	je	.wm_destroy
	xor	eax,eax
	ret

  .wm_initdialog:
	mov	rax,[hwnd]
	mov	[hMain],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	rbx,rax
	invoke	SendMessageW,[hwnd],WM_SETICON,ICON_BIG,rbx
	invoke	SendMessageW,[hwnd],WM_SETICON,ICON_SMALL,rbx
	fastcall InitDialogControls,[hwnd]
	fastcall Layout,[hwnd]
	fastcall UpdateAll
	mov	eax,1
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
	cmp	ecx,ID_FILTER_EDIT
	je	.filter_cmd
	cmp	ecx,ID_MERGED_CHECK
	je	.merged_cmd
	cmp	ecx,ID_LEGACY_CHECK
	je	.legacy_cmd
	cmp	ecx,ID_COMPLEX_LIST
	je	.complex_list_cmd
	cmp	ecx,ID_COMPLEX_COLOR
	je	.complex_color_cmd
	cmp	ecx,ID_COMPLEX_APPEND
	je	.complex_append_cmd
	cmp	ecx,ID_COMPLEX_MOVE_UP
	je	.complex_move_up_cmd
	cmp	ecx,ID_COMPLEX_MOVE_DOWN
	je	.complex_move_down_cmd
	cmp	ecx,ID_COMPLEX_REMOVE
	je	.complex_remove_cmd
	cmp	ecx,ID_COMPLEX_CLEAR
	je	.complex_clear_cmd
	cmp	ecx,IDCANCEL
	je	.close_cmd
	xor	eax,eax
	ret
  .font_cmd:
	cmp	eax,CBN_SELCHANGE
	jne	.done_one
	fastcall OnFontComboChange
	jmp	.done_one
  .size_cmd:
	cmp	eax,CBN_SELCHANGE
	jne	.done_one
	fastcall OnSizeComboChange
	jmp	.done_one
  .copy_cmd:
	fastcall CopyExportText
	jmp	.done_one
  .fore_cmd:
	fastcall PickColor,addr color_fore
	jmp	.done_one
  .back_cmd:
	fastcall PickColor,addr color_back
	jmp	.done_one
  .pair_cmd:
	fastcall PickColor,addr color_pair
	jmp	.done_one
  .filter_cmd:
	mov	dword [scroll_y],0
	fastcall UpdateAll
	jmp	.done_one
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
	jmp	.done_one
  .legacy_cmd:
	invoke	SendMessageW,[hCheckLegacy],BM_GETCHECK,0,0
	cmp	eax,BST_CHECKED
	jne	.legacy_off
	mov	dword [show_legacy],1
	jmp	.legacy_update
  .legacy_off:
	mov	dword [show_legacy],0
  .legacy_update:
	mov	dword [scroll_y],0
	fastcall UpdateAll
	jmp	.done_one
  .complex_list_cmd:
	cmp	eax,LBN_SELCHANGE
	je	.complex_list_select
	cmp	eax,LBN_DBLCLK
	je	.complex_list_remove
	jmp	.done_one
  .complex_list_select:
	invoke	SendMessageW,[hComplexList],LB_GETCURSEL,0,0
	cmp	eax,LB_ERR
	je	.complex_list_no_selection
	fastcall ComplexLayer_Select,eax
	jmp	.done_one
  .complex_list_no_selection:
	mov	dword [complex_selected],-1
	fastcall ComplexUpdateSelectionState
	jmp	.done_one
  .complex_list_remove:
	invoke	SendMessageW,[hComplexList],LB_GETCURSEL,0,0
	cmp	eax,LB_ERR
	je	.done_one
	mov	dword [complex_selected],eax
	fastcall ComplexLayer_RemoveSelected
	jmp	.done_one
  .complex_color_cmd:
	fastcall ComplexPickSelectedColor
	jmp	.done_one
  .complex_append_cmd:
	fastcall AppendComplexExport
	jmp	.done_one
  .complex_move_up_cmd:
	fastcall ComplexLayer_MoveSelected,-1
	jmp	.done_one
  .complex_move_down_cmd:
	fastcall ComplexLayer_MoveSelected,1
	jmp	.done_one
  .complex_remove_cmd:
	fastcall ComplexLayer_RemoveSelected
	jmp	.done_one
  .complex_clear_cmd:
	fastcall ComplexLayer_Clear
	jmp	.done_one
  .close_cmd:
	invoke	EndDialog,[hwnd],0
	jmp	.done_one

  .wm_vkeytoitem:
	mov	rax,[lparam]
	cmp	rax,[hComplexList]
	jne	.vkey_default
	mov	eax,dword [wparam]
	and	eax,0FFFFh
	fastcall ComplexListKey,eax
	test	eax,eax
	jz	.vkey_default
	mov	eax,UWP_VKEY_HANDLED
	ret
  .vkey_default:
	mov	eax,UWP_VKEY_DEFAULT
	ret

  .wm_notify:
	mov	rbx,[lparam]
	cmp	qword [rbx+NMHDR.idFrom],ID_MODE_TAB
	jne	.done_zero
	cmp	dword [rbx+NMHDR.code],TCN_SELCHANGE
	jne	.done_zero
	invoke	SendMessageW,[hModeTab],TCM_GETCURSEL,0,0
	fastcall SetModeTab,eax
	jmp	.done_one

  .wm_size:
	cmp	qword [hView],0
	je	.done_one
	fastcall Layout,[hwnd]
	fastcall UpdateViewScroll,[hView]
	invoke	InvalidateRect,[hView],0,1
	invoke	InvalidateRect,[hComplexView],0,1
	mov	eax,1
	ret

  .wm_getminmaxinfo:
	mov	rbx,[lparam]
	mov	dword [rbx+MINMAXINFO.ptMinTrackSize.x],UWP_MIN_WINDOW_W
	mov	dword [rbx+MINMAXINFO.ptMinTrackSize.y],UWP_MIN_WINDOW_H
	mov	eax,1
	ret

  .wm_ctlcoloredit:
	mov	rax,[lparam]
	cmp	rax,[hEdit]
	jne	.done_zero
	invoke	SetTextColor,[wparam],dword [color_fore]
	invoke	SetBkColor,[wparam],dword [color_back]
	mov	rax,[hViewBrush]
	ret

  .wm_close:
	invoke	EndDialog,[hwnd],0
	mov	eax,1
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
	cmp	qword [hViewBrush],0
	je	.no_view_brush
	invoke	DeleteObject,[hViewBrush]
  .no_view_brush:
  .done_one:
	mov	eax,1
	ret
  .done_zero:
	xor	eax,eax
	ret
endp

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[view_wc.hInstance],rax
	mov	[complex_view_wc.hInstance],rax

	invoke	InitCommonControlsEx,addr icc

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[view_wc.hCursor],rax
	mov	[complex_view_wc.hCursor],rax

	invoke	RegisterClassExW,addr view_wc
	test	rax,rax
	jz	.fatal
	invoke	RegisterClassExW,addr complex_view_wc
	test	rax,rax
	jz	.fatal

	invoke	DialogBoxParamW,[hInstance],IDD_UWPCHAR,0,UwpCharDlgProc,0
	cmp	rax,-1
	jz	.fatal
	invoke	ExitProcess,0

  .fatal:
	invoke	MessageBoxW,0,'uwpchar.asm failed to start.','uwpchar.asm - icon font browser',MB_ICONERROR
	invoke	ExitProcess,1
endp
