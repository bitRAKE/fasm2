OBJID_MENU = 0FFFFFFFDh
CHILDID_SELF = 0

struct MENUBARINFO
	cbSize dd ?
	rcBar RECT
	dd ?
	hMenu dq ?
	hwndMenu dq ?
	flags dd ?
	dd ?
ends

proc MenuFindTab textp
	mov	rax,rcx
  .scan:
	movzx	ecx,word [rax]
	test	ecx,ecx
	jz	.none
	cmp	ecx,9
	je	.found
	add	rax,2
	jmp	.scan
  .found:
	ret
  .none:
	xor	eax,eax
	ret
endp

proc MenuSplitGlyph textp,label_out
	mov	rax,rcx
	mov	[rdx],rax
	mov	r8,rdx

	movzx	eax,word [rcx]
	cmp	eax,0E000h
	jb	.none
	cmp	eax,0F8FFh
	ja	.none

	lea	rdx,[rcx+2]
  .skip_spaces:
	cmp	word [rdx],' '
	jne	.set_label
	add	rdx,2
	jmp	.skip_spaces
  .set_label:
	mov	[r8],rdx
	movzx	eax,word [rcx]
	ret
  .none:
	xor	eax,eax
	ret
endp

proc MenuCopyRect dstp,srcp
	mov	eax,[rdx+RECT.left]
	mov	[rcx+RECT.left],eax
	mov	eax,[rdx+RECT.top]
	mov	[rcx+RECT.top],eax
	mov	eax,[rdx+RECT.right]
	mov	[rcx+RECT.right],eax
	mov	eax,[rdx+RECT.bottom]
	mov	[rcx+RECT.bottom],eax
	ret
endp

proc MenuFillRect hdc,rectp,color
    locals
	brush dq ?
    endl
	mov	[hdc],rcx
	mov	[rectp],rdx
	mov	dword [color],r8d
	invoke	CreateSolidBrush,dword [color]
	mov	[brush],rax
	test	rax,rax
	jz	.done
	invoke	FillRect,[hdc],[rectp],[brush]
	invoke	DeleteObject,[brush]
  .done:
	ret
endp

proc MenuDrawGlyph hdc,font,rectp,glyph,color
    locals
	old_font dq ?
	old_color dd ?
	old_bk dd ?
	glyph_text dw ?,0
    endl
	mov	[hdc],rcx
	mov	[font],rdx
	mov	[rectp],r8
	mov	dword [glyph],r9d
	mov	qword [old_font],0
	mov	ax,word [glyph]
	mov	[glyph_text],ax
	cmp	qword [font],0
	je	.no_font
	invoke	SelectObject,[hdc],[font]
	mov	[old_font],rax
  .no_font:
	invoke	SetBkMode,[hdc],TRANSPARENT
	mov	[old_bk],eax
	invoke	SetTextColor,[hdc],dword [color]
	mov	[old_color],eax
	invoke	DrawTextW,[hdc],addr glyph_text,1,[rectp],\
		DT_CENTER or DT_VCENTER or DT_SINGLELINE or DT_NOPREFIX
	invoke	SetTextColor,[hdc],dword [old_color]
	invoke	SetBkMode,[hdc],dword [old_bk]
	cmp	qword [old_font],0
	je	.done
	invoke	SelectObject,[hdc],[old_font]
  .done:
	ret
endp

proc MenuLoadItemInfo menup,position,infop,textp
	mov	[menup],rcx
	mov	dword [position],edx
	mov	[infop],r8
	mov	[textp],r9
	invoke	RtlZeroMemory,[infop],sizeof.MENUITEMINFO
	mov	rcx,[infop]
	mov	[rcx+MENUITEMINFO.cbSize],sizeof.MENUITEMINFO
	mov	[rcx+MENUITEMINFO.fMask],MIIM_ID or MIIM_STRING or MIIM_FTYPE or MIIM_STATE or MIIM_SUBMENU
	mov	rax,[textp]
	mov	[rcx+MENUITEMINFO.dwTypeData],rax
	mov	[rcx+MENUITEMINFO.cch],MENU_TEXT_MAX - 1
	invoke	GetMenuItemInfo,[menup],dword [position],1,[infop]
	ret
endp

proc MenuGetBarRect hwnd,rectp
    locals
	mbi MENUBARINFO
	wnd RECT
	dx dd ?
	dy dd ?
    endl
	mov	[hwnd],rcx
	mov	[rectp],rdx
	invoke	RtlZeroMemory,addr mbi,sizeof.MENUBARINFO
	mov	[mbi.cbSize],sizeof.MENUBARINFO
	invoke	GetMenuBarInfo,[hwnd],OBJID_MENU,CHILDID_SELF,addr mbi
	test	eax,eax
	jz	.fail
	invoke	GetWindowRect,[hwnd],addr wnd
	mov	rdx,[rectp]
	fastcall MenuCopyRect,rdx,addr mbi.rcBar
	mov	eax,[wnd.left]
	neg	eax
	mov	[dx],eax
	mov	eax,[wnd.top]
	neg	eax
	mov	[dy],eax
	invoke	OffsetRect,[rectp],dword [dx],dword [dy]
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc MenuBuildItemState item_state,infop
	mov	dword [item_state],ecx
	mov	[infop],rdx
	xor	eax,eax
	mov	edx,dword [item_state]
	test	edx,ODS_HOTLIGHT
	jz	.no_hot
	or	eax,MENU_STATE_HOT
  .no_hot:
	test	edx,ODS_SELECTED
	jz	.no_selected
	or	eax,MENU_STATE_SELECTED
  .no_selected:
	test	edx,ODS_GRAYED or ODS_DISABLED
	jz	.no_disabled_ods
	or	eax,MENU_STATE_DISABLED
  .no_disabled_ods:
	test	edx,ODS_INACTIVE
	jz	.no_inactive
	or	eax,MENU_STATE_INACTIVE
  .no_inactive:
	test	edx,ODS_NOACCEL
	jz	.no_noaccel
	or	eax,MENU_STATE_NOACCEL
  .no_noaccel:
	test	edx,ODS_DEFAULT
	jz	.no_default_ods
	or	eax,MENU_STATE_DEFAULT
  .no_default_ods:
	mov	rcx,[infop]
	mov	edx,[rcx+MENUITEMINFO.fState]
	test	edx,MFS_DISABLED
	jz	.no_disabled_info
	or	eax,MENU_STATE_DISABLED
  .no_disabled_info:
	test	edx,MFS_CHECKED
	jz	.no_checked
	or	eax,MENU_STATE_CHECKED
  .no_checked:
	test	edx,MFS_DEFAULT
	jz	.no_default_info
	or	eax,MENU_STATE_DEFAULT
  .no_default_info:
	mov	edx,[rcx+MENUITEMINFO.fType]
	test	edx,MFT_RADIOCHECK
	jz	.no_radio
	or	eax,MENU_STATE_RADIO
  .no_radio:
	cmp	qword [rcx+MENUITEMINFO.hSubMenu],0
	je	.done
	or	eax,MENU_STATE_HASSUB
  .done:
	ret
endp

proc MenuMeasureItemSurface uses rbx, hwnd,measurep,surface
    locals
	info MENUITEMINFO
	text rw MENU_TEXT_MAX
	labelp dq ?
	accelp dq ?
	accel_textp dq ?
	label_size SIZE
	accel_size SIZE
	hdc dq ?
	old_font dq ?
	dpi dd ?
	label_chars dd ?
	accel_chars dd ?
	width dd ?
	height dd ?
    endl
	mov	[hwnd],rcx
	mov	[measurep],rdx
	mov	dword [surface],r8d
	mov	rbx,rdx
	invoke	IsMenu,qword [rbx+UAHMEASUREMENUITEM.um+UAHMENU.hmenu]
	test	eax,eax
	jz	.no

	fastcall MenuLoadItemInfo,qword [rbx+UAHMEASUREMENUITEM.um+UAHMENU.hmenu],\
		dword [rbx+UAHMEASUREMENUITEM.umi+UAHMENUITEM.iPosition],\
		addr info,addr text
	test	eax,eax
	jz	.no

	invoke	GetDpiForWindow,[hwnd]
	test	eax,eax
	jnz	.dpi_ready
	mov	eax,96
  .dpi_ready:
	mov	[dpi],eax
	fastcall EnsureMenuFonts,eax

	cmp	dword [surface],MENU_SURFACE_BAR
	je	.normal
	mov	eax,[info.fType]
	and	eax,MFT_SEPARATOR
	jz	.normal
	fastcall ScaleForDpi,MENU_SEP_H_96,dword [dpi]
	mov	dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemHeight],eax
	fastcall ScaleForDpi,48,dword [dpi]
	mov	dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemWidth],eax
	mov	eax,1
	ret

  .normal:
	mov	[label_size.cx],0
	mov	[label_size.cy],0
	mov	[accel_size.cx],0
	mov	[accel_size.cy],0

	fastcall MenuSplitGlyph,addr text,addr labelp
	fastcall MenuFindTab,[labelp]
	mov	[accelp],rax
	test	rax,rax
	jz	.whole_label
	sub	rax,[labelp]
	shr	rax,1
	mov	[label_chars],eax
	jmp	.measure
  .whole_label:
	invoke	lstrlenW,[labelp]
	mov	[label_chars],eax

  .measure:
	invoke	GetDC,[hwnd]
	mov	[hdc],rax
	test	rax,rax
	jz	.no
	invoke	SelectObject,[hdc],[hMenuFontText]
	mov	[old_font],rax
	invoke	GetTextExtentPoint32,[hdc],[labelp],dword [label_chars],addr label_size
	cmp	qword [accelp],0
	je	.release_dc
	mov	rax,[accelp]
	add	rax,2
	mov	[accel_textp],rax
	invoke	lstrlenW,[accel_textp]
	mov	[accel_chars],eax
	invoke	GetTextExtentPoint32,[hdc],[accel_textp],dword [accel_chars],addr accel_size
  .release_dc:
	invoke	SelectObject,[hdc],[old_font]
	invoke	ReleaseDC,[hwnd],[hdc]

	cmp	dword [surface],MENU_SURFACE_BAR
	je	.bar_dimensions

	fastcall ScaleForDpi,MENU_GUTTER_96,dword [dpi]
	mov	[width],eax
	mov	eax,[label_size.cx]
	add	[width],eax
	fastcall ScaleForDpi,MENU_PAD_96,dword [dpi]
	add	[width],eax
	cmp	qword [accelp],0
	je	.submenu_width
	fastcall ScaleForDpi,28,dword [dpi]
	add	[width],eax
	mov	eax,[accel_size.cx]
	add	[width],eax
  .submenu_width:
	cmp	qword [info.hSubMenu],0
	je	.height
	fastcall ScaleForDpi,MENU_ARROW_96,dword [dpi]
	add	[width],eax

  .height:
	mov	eax,[label_size.cy]
	mov	[height],eax
	fastcall ScaleForDpi,10,dword [dpi]
	add	[height],eax
	fastcall ScaleForDpi,MENU_MIN_H_96,dword [dpi]
	cmp	[height],eax
	jge	.store
	mov	[height],eax
	jmp	.store

  .bar_dimensions:
	mov	eax,[label_size.cx]
	mov	[width],eax
	fastcall ScaleForDpi,MENU_BAR_PAD_X_96,dword [dpi]
	shl	eax,1
	add	[width],eax
	mov	eax,[label_size.cy]
	mov	[height],eax
	fastcall ScaleForDpi,MENU_BAR_PAD_Y_96,dword [dpi]
	shl	eax,1
	add	[height],eax
	fastcall ScaleForDpi,MENU_BAR_MIN_H_96,dword [dpi]
	cmp	[height],eax
	jge	.store
	mov	[height],eax

  .store:
	cmp	dword [surface],MENU_SURFACE_BAR
	je	.store_bar
	mov	eax,[width]
	mov	dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemWidth],eax
	mov	eax,[height]
	mov	dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemHeight],eax
	mov	eax,1
	ret

  .store_bar:
	mov	eax,[width]
	mov	ecx,dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemWidth]
	cmp	eax,ecx
	jle	.height_bar
	mov	dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemWidth],eax
  .height_bar:
	mov	eax,[height]
	mov	ecx,dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemHeight]
	cmp	eax,ecx
	jle	.done_ok
	mov	dword [rbx+UAHMEASUREMENUITEM.mis+MEASUREITEMSTRUCT.itemHeight],eax
  .done_ok:
	mov	eax,1
	ret

  .no:
	xor	eax,eax
	ret
endp

proc MenuMeasureItem hwnd,measurep
	mov	[hwnd],rcx
	mov	[measurep],rdx
	fastcall MenuMeasureItemSurface,[hwnd],[measurep],MENU_SURFACE_POPUP
	ret
endp

proc MenuMeasureBarItem hwnd,measurep
	mov	[hwnd],rcx
	mov	[measurep],rdx
	fastcall MenuMeasureItemSurface,[hwnd],[measurep],MENU_SURFACE_BAR
	ret
endp

proc MenuPaintItemSurface uses rbx, hwnd,itemp,surface
    locals
	info MENUITEMINFO
	text rw MENU_TEXT_MAX
	labelp dq ?
	accelp dq ?
	accel_textp dq ?
	hdc dq ?
	old_font dq ?
	rect RECT
	text_rect RECT
	icon_rect RECT
	hover_rect RECT
	line_rect RECT
	arrow_rect RECT
	bg dd ?
	fg dd ?
	item_fg dd ?
	hover_color dd ?
	dpi dd ?
	gutter dd ?
	pad dd ?
	arrow dd ?
	inset_x dd ?
	inset_y dd ?
	glyph dd ?
	label_chars dd ?
	state dd ?
	text_flags dd ?
    endl
	mov	[hwnd],rcx
	mov	[itemp],rdx
	mov	dword [surface],r8d
	mov	rbx,rdx

	mov	rax,qword [rbx+UAHDRAWMENUITEM.um+UAHMENU.hdc]
	test	rax,rax
	jnz	.have_hdc
	mov	rax,qword [rbx+UAHDRAWMENUITEM.dis+DRAWITEMSTRUCT.hDC]
  .have_hdc:
	mov	[hdc],rax
	test	rax,rax
	jz	.done

	fastcall MenuLoadItemInfo,qword [rbx+UAHDRAWMENUITEM.um+UAHMENU.hmenu],\
		dword [rbx+UAHDRAWMENUITEM.umi+UAHMENUITEM.iPosition],\
		addr info,addr text
	test	eax,eax
	jz	.done

	invoke	GetDpiForWindow,[hwnd]
	test	eax,eax
	jnz	.dpi_ready
	mov	eax,96
  .dpi_ready:
	mov	[dpi],eax
	fastcall EnsureMenuFonts,eax
	fastcall ScaleForDpi,MENU_GUTTER_96,dword [dpi]
	mov	[gutter],eax
	fastcall ScaleForDpi,MENU_PAD_96,dword [dpi]
	mov	[pad],eax
	fastcall ScaleForDpi,MENU_ARROW_96,dword [dpi]
	mov	[arrow],eax

	lea	rdx,[rbx+UAHDRAWMENUITEM.dis+DRAWITEMSTRUCT.rcItem]
	fastcall MenuCopyRect,addr rect,rdx
	fastcall MenuBuildItemState,\
		dword [rbx+UAHDRAWMENUITEM.dis+DRAWITEMSTRUCT.itemState],addr info
	mov	[state],eax

	fastcall ThemeBg
	mov	[bg],eax
	fastcall ThemeText
	mov	[fg],eax
	mov	[item_fg],eax
	fastcall MenuFillRect,[hdc],addr rect,dword [bg]

	cmp	dword [surface],MENU_SURFACE_BAR
	je	.not_separator
	mov	eax,[info.fType]
	and	eax,MFT_SEPARATOR
	jz	.not_separator
	fastcall MenuCopyRect,addr line_rect,addr rect
	mov	eax,[line_rect.top]
	add	eax,[line_rect.bottom]
	sar	eax,1
	mov	[line_rect.top],eax
	inc	eax
	mov	[line_rect.bottom],eax
	mov	eax,[pad]
	add	[line_rect.left],eax
	sub	[line_rect.right],eax
	fastcall ThemeBorder
	fastcall MenuFillRect,[hdc],addr line_rect,eax
	jmp	.done

  .not_separator:
	mov	eax,[state]
	and	eax,MENU_STATE_DISABLED
	jz	.check_hover
	fastcall ThemeBlendColor,dword [fg],dword [bg],58
	mov	[item_fg],eax

  .check_hover:
	mov	eax,[state]
	and	eax,MENU_STATE_DISABLED
	jnz	.layout
	mov	eax,[state]
	and	eax,MENU_STATE_SELECTED or MENU_STATE_HOT
	jz	.layout
	fastcall ThemeHover
	mov	[hover_color],eax
	fastcall MenuCopyRect,addr hover_rect,addr rect
	cmp	dword [surface],MENU_SURFACE_BAR
	je	.bar_hover_inset
	fastcall ScaleForDpi,3,dword [dpi]
	neg	eax
	mov	[inset_x],eax
	fastcall ScaleForDpi,2,dword [dpi]
	neg	eax
	mov	[inset_y],eax
	invoke	InflateRect,addr hover_rect,dword [inset_x],dword [inset_y]
	fastcall MenuFillRect,[hdc],addr hover_rect,dword [hover_color]
	jmp	.layout
  .bar_hover_inset:
	fastcall ScaleForDpi,2,dword [dpi]
	neg	eax
	mov	[inset_x],eax
	fastcall ScaleForDpi,1,dword [dpi]
	neg	eax
	mov	[inset_y],eax
	invoke	InflateRect,addr hover_rect,dword [inset_x],dword [inset_y]
	fastcall MenuFillRect,[hdc],addr hover_rect,dword [hover_color]

  .layout:
	cmp	dword [surface],MENU_SURFACE_BAR
	je	.draw_bar

  .draw_gutter:
	fastcall MenuCopyRect,addr icon_rect,addr rect
	mov	eax,[icon_rect.left]
	add	eax,[gutter]
	mov	[icon_rect.right],eax
	fastcall MenuSplitGlyph,addr text,addr labelp
	mov	[glyph],eax

	mov	eax,[state]
	and	eax,MENU_STATE_CHECKED
	jz	.maybe_glyph
	mov	dword [glyph],ICON_CHECK
	mov	eax,[state]
	and	eax,MENU_STATE_RADIO
	jz	.draw_check
	mov	dword [glyph],ICON_RADIO
  .draw_check:
	fastcall MenuDrawGlyph,[hdc],[hMenuFontSymbol],addr icon_rect,\
		dword [glyph],dword [item_fg]
	jmp	.draw_text

  .maybe_glyph:
	cmp	dword [glyph],0
	je	.draw_text
	fastcall MenuDrawGlyph,[hdc],[hMenuFontGlyph],addr icon_rect,\
		dword [glyph],dword [item_fg]

  .draw_text:
	fastcall MenuCopyRect,addr text_rect,addr rect
	mov	eax,[gutter]
	add	[text_rect.left],eax
	mov	eax,[pad]
	sub	[text_rect.right],eax
	mov	eax,[state]
	and	eax,MENU_STATE_HASSUB
	jz	.text_ready
	mov	eax,[arrow]
	sub	[text_rect.right],eax
  .text_ready:
	invoke	SelectObject,[hdc],[hMenuFontText]
	mov	[old_font],rax
	invoke	SetBkMode,[hdc],TRANSPARENT
	invoke	SetTextColor,[hdc],dword [item_fg]
	fastcall MenuFindTab,[labelp]
	mov	[accelp],rax
	test	rax,rax
	jz	.draw_label_all
	sub	rax,[labelp]
	shr	rax,1
	mov	[label_chars],eax
	invoke	DrawTextW,[hdc],[labelp],dword [label_chars],addr text_rect,\
		DT_LEFT or DT_VCENTER or DT_SINGLELINE or DT_END_ELLIPSIS
	fastcall ThemeMuted
	mov	ecx,[state]
	and	ecx,MENU_STATE_DISABLED
	je	.accel_color_ready
	mov	eax,[item_fg]
  .accel_color_ready:
	invoke	SetTextColor,[hdc],eax
	mov	rax,[accelp]
	add	rax,2
	mov	[accel_textp],rax
	invoke	DrawTextW,[hdc],[accel_textp],-1,addr text_rect,\
		DT_RIGHT or DT_VCENTER or DT_SINGLELINE or DT_HIDEPREFIX
	jmp	.after_text

  .draw_label_all:
	invoke	DrawTextW,[hdc],[labelp],-1,addr text_rect,\
		DT_LEFT or DT_VCENTER or DT_SINGLELINE or DT_END_ELLIPSIS

  .after_text:
	cmp	qword [old_font],0
	je	.submenu
	invoke	SelectObject,[hdc],[old_font]

  .submenu:
	mov	eax,[state]
	and	eax,MENU_STATE_HASSUB
	jz	.done
	fastcall MenuCopyRect,addr arrow_rect,addr rect
	mov	eax,[arrow_rect.right]
	sub	eax,[arrow]
	mov	[arrow_rect.left],eax
	mov	dword [glyph],ICON_SUBMENU
	fastcall MenuDrawGlyph,[hdc],[hMenuFontSymbol],addr arrow_rect,\
		dword [glyph],dword [item_fg]
	jmp	.done

  .draw_bar:
	fastcall MenuSplitGlyph,addr text,addr labelp
	fastcall MenuCopyRect,addr text_rect,addr rect
	fastcall ScaleForDpi,MENU_BAR_PAD_X_96,dword [dpi]
	add	[text_rect.left],eax
	sub	[text_rect.right],eax
	fastcall ScaleForDpi,MENU_BAR_PAD_Y_96,dword [dpi]
	add	[text_rect.top],eax
	sub	[text_rect.bottom],eax
	mov	dword [text_flags],DT_CENTER or DT_VCENTER or DT_SINGLELINE or DT_END_ELLIPSIS
	mov	eax,[state]
	and	eax,MENU_STATE_NOACCEL
	jz	.bar_flags_ready
	or	dword [text_flags],DT_HIDEPREFIX
  .bar_flags_ready:
	invoke	SelectObject,[hdc],[hMenuFontText]
	mov	[old_font],rax
	invoke	SetBkMode,[hdc],TRANSPARENT
	invoke	SetTextColor,[hdc],dword [item_fg]
	fastcall MenuFindTab,[labelp]
	mov	[accelp],rax
	test	rax,rax
	jz	.bar_draw_all
	sub	rax,[labelp]
	shr	rax,1
	mov	[label_chars],eax
	invoke	DrawTextW,[hdc],[labelp],dword [label_chars],addr text_rect,\
		dword [text_flags]
	jmp	.bar_after_text
  .bar_draw_all:
	invoke	DrawTextW,[hdc],[labelp],-1,addr text_rect,dword [text_flags]
  .bar_after_text:
	cmp	qword [old_font],0
	je	.done
	invoke	SelectObject,[hdc],[old_font]
  .done:
	ret
endp

proc MenuPaintItem hwnd,itemp
	mov	[hwnd],rcx
	mov	[itemp],rdx
	fastcall MenuPaintItemSurface,[hwnd],[itemp],MENU_SURFACE_POPUP
	ret
endp

proc MenuPaintBarItem hwnd,itemp
	mov	[hwnd],rcx
	mov	[itemp],rdx
	fastcall MenuPaintItemSurface,[hwnd],[itemp],MENU_SURFACE_BAR
	ret
endp

proc MenuPaintSurface hwnd,menuinfo,surface
    locals
	rc RECT
    endl
	mov	[hwnd],rcx
	mov	[menuinfo],rdx
	mov	dword [surface],r8d
	cmp	qword [menuinfo],0
	je	.done
	mov	rcx,[menuinfo]
	cmp	qword [rcx+UAHMENU.hdc],0
	je	.done
	cmp	dword [surface],MENU_SURFACE_BAR
	je	.bar_rect
	invoke	GetClientRect,[hwnd],addr rc
	jmp	.fill
  .bar_rect:
	fastcall MenuGetBarRect,[hwnd],addr rc
	test	eax,eax
	jz	.done
  .fill:
	fastcall ThemeBg
	mov	rdx,[menuinfo]
	fastcall MenuFillRect,[rdx+UAHMENU.hdc],addr rc,eax
  .done:
	ret
endp

proc MenuPaintBackground hwnd,menuinfo
	mov	[hwnd],rcx
	mov	[menuinfo],rdx
	fastcall MenuPaintSurface,[hwnd],[menuinfo],MENU_SURFACE_POPUP
	ret
endp

proc MenuPaintBarBackground hwnd,menuinfo
	mov	[hwnd],rcx
	mov	[menuinfo],rdx
	fastcall MenuPaintSurface,[hwnd],[menuinfo],MENU_SURFACE_BAR
	ret
endp

proc MenuPaintBarBottomLine hwnd
    locals
	hdc dq ?
	rc RECT
	line RECT
    endl
	mov	[hwnd],rcx
	fastcall MenuGetBarRect,[hwnd],addr rc
	test	eax,eax
	jz	.done
	invoke	GetWindowDC,[hwnd]
	mov	[hdc],rax
	test	rax,rax
	jz	.done
	fastcall MenuCopyRect,addr line,addr rc
	mov	eax,[line.bottom]
	dec	eax
	mov	[line.top],eax
	fastcall ThemeBorder
	fastcall MenuFillRect,[hdc],addr line,eax
	invoke	ReleaseDC,[hwnd],[hdc]
  .done:
	ret
endp

proc MenuFramePopup hwnd
    locals
	hdc dq ?
	rc RECT
	brush dq ?
	left dd ?
	top dd ?
    endl
	mov	[hwnd],rcx
	invoke	GetWindowDC,[hwnd]
	mov	[hdc],rax
	test	rax,rax
	jz	.done
	invoke	GetWindowRect,[hwnd],addr rc
	mov	eax,[rc.left]
	neg	eax
	mov	[left],eax
	mov	eax,[rc.top]
	neg	eax
	mov	[top],eax
	invoke	OffsetRect,addr rc,dword [left],dword [top]
	fastcall ThemeBorder
	invoke	CreateSolidBrush,eax
	mov	[brush],rax
	test	rax,rax
	jz	.release
	invoke	FrameRect,[hdc],addr rc,[brush]
	invoke	DeleteObject,[brush]
  .release:
	invoke	ReleaseDC,[hwnd],[hdc]
  .done:
	ret
endp

proc MenuPopupSubclassProc hwnd,wmsg,wparam,lparam,subclass_id,refdata
    locals
	result dq ?
	procp dq ?
    endl
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_UAHDRAWMENU
	je	.wm_uah_drawmenu
	cmp	edx,WM_UAHDRAWMENUITEM
	je	.wm_uah_drawitem
	cmp	edx,WM_UAHMEASUREMENUITEM
	je	.wm_uah_measure
	cmp	edx,WM_UAHNCPAINTMENUPOPUP
	je	.wm_uah_ncpaint
	cmp	edx,WM_NCDESTROY
	je	.wm_ncdestroy

	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_uah_drawmenu:
	fastcall MenuPaintBackground,[hwnd],[lparam]
	mov	eax,1
	ret

  .wm_uah_drawitem:
	cmp	qword [lparam],0
	je	.default_true
	fastcall MenuPaintItem,[hwnd],[lparam]
  .default_true:
	mov	eax,1
	ret

  .wm_uah_measure:
	cmp	qword [lparam],0
	je	.measure_default
	fastcall MenuMeasureItem,[hwnd],[lparam]
	test	eax,eax
	jnz	.measure_done
  .measure_default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .measure_done:
	mov	eax,1
	ret

  .wm_uah_ncpaint:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	mov	[result],rax
	fastcall MenuFramePopup,[hwnd]
	mov	rax,[result]
	ret

  .wm_ncdestroy:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	mov	[result],rax
	lea	rax,[MenuPopupSubclassProc]
	mov	[procp],rax
	invoke	RemoveWindowSubclass,[hwnd],[procp],[subclass_id]
	mov	rax,[result]
	ret
endp
