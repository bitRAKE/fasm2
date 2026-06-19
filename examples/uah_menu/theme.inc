proc ThemeBg
	cmp	dword [dark_mode],0
	je	.light
	mov	eax,THEME_DARK_BG
	ret
  .light:
	mov	eax,THEME_LIGHT_BG
	ret
endp

proc ThemeText
	cmp	dword [dark_mode],0
	je	.light
	mov	eax,THEME_DARK_TEXT
	ret
  .light:
	mov	eax,THEME_LIGHT_TEXT
	ret
endp

proc ThemeBorder
	cmp	dword [dark_mode],0
	je	.light
	mov	eax,THEME_DARK_BORDER
	ret
  .light:
	mov	eax,THEME_LIGHT_BORDER
	ret
endp

proc ThemeMuted
	cmp	dword [dark_mode],0
	je	.light
	mov	eax,THEME_DARK_MUTED
	ret
  .light:
	mov	eax,THEME_LIGHT_MUTED
	ret
endp

proc ThemeHover
	cmp	dword [dark_mode],0
	je	.light
	mov	eax,THEME_DARK_HOVER
	ret
  .light:
	mov	eax,THEME_LIGHT_HOVER
	ret
endp

proc ScaleForDpi pixels,dpi
	mov	dword [pixels],ecx
	mov	dword [dpi],edx
	cmp	dword [dpi],0
	jne	.ready
	mov	dword [dpi],96
  .ready:
	invoke	MulDiv,dword [pixels],dword [dpi],96
	ret
endp

proc ThemeBlendColor from_color,to_color,percent_to
    locals
	from_component dd ?
	red	dd ?
	green	dd ?
	blue	dd ?
    endl
	mov	dword [from_color],ecx
	mov	dword [to_color],edx
	mov	dword [percent_to],r8d

	mov	eax,dword [from_color]
	and	eax,0FFh
	mov	[from_component],eax
	mov	ecx,dword [to_color]
	and	ecx,0FFh
	sub	ecx,eax
	invoke	MulDiv,ecx,dword [percent_to],100
	add	eax,[from_component]
	mov	[red],eax

	mov	eax,dword [from_color]
	shr	eax,8
	and	eax,0FFh
	mov	[from_component],eax
	mov	ecx,dword [to_color]
	shr	ecx,8
	and	ecx,0FFh
	sub	ecx,eax
	invoke	MulDiv,ecx,dword [percent_to],100
	add	eax,[from_component]
	mov	[green],eax

	mov	eax,dword [from_color]
	shr	eax,16
	and	eax,0FFh
	mov	[from_component],eax
	mov	ecx,dword [to_color]
	shr	ecx,16
	and	ecx,0FFh
	sub	ecx,eax
	invoke	MulDiv,ecx,dword [percent_to],100
	add	eax,[from_component]
	mov	[blue],eax

	mov	eax,[blue]
	shl	eax,16
	mov	ecx,[green]
	shl	ecx,8
	or	eax,ecx
	or	eax,[red]
	ret
endp

proc CreateFontPixels dpi,pixels,facep
	mov	dword [dpi],ecx
	mov	dword [pixels],edx
	mov	[facep],r8
	invoke	MulDiv,dword [pixels],dword [dpi],96
	neg	eax
	invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,[facep]
	ret
endp

proc DeleteMenuFonts
	mov	rcx,[hMenuFontText]
	jrcxz	.symbol
	invoke	DeleteObject,rcx
	mov	qword [hMenuFontText],0
  .symbol:
	mov	rcx,[hMenuFontSymbol]
	jrcxz	.glyph
	invoke	DeleteObject,rcx
	mov	qword [hMenuFontSymbol],0
  .glyph:
	mov	rcx,[hMenuFontGlyph]
	jrcxz	.done
	invoke	DeleteObject,rcx
	mov	qword [hMenuFontGlyph],0
  .done:
	mov	dword [menu_font_dpi],0
	ret
endp

proc EnsureMenuFonts dpi
	mov	dword [dpi],ecx
	cmp	dword [dpi],0
	jne	.have_dpi
	mov	dword [dpi],96
  .have_dpi:
	mov	eax,dword [dpi]
	cmp	eax,dword [menu_font_dpi]
	jne	.rebuild
	cmp	qword [hMenuFontText],0
	jne	.done
  .rebuild:
	fastcall DeleteMenuFonts
	mov	eax,dword [dpi]
	mov	dword [menu_font_dpi],eax
	fastcall CreateFontPixels,dword [dpi],16,'Segoe UI'
	mov	[hMenuFontText],rax
	fastcall CreateFontPixels,dword [dpi],14,'Segoe UI Symbol'
	mov	[hMenuFontSymbol],rax
	fastcall CreateFontPixels,dword [dpi],18,'Segoe Fluent Icons'
	mov	[hMenuFontGlyph],rax
  .done:
	ret
endp

proc ApplyMenuAppMode
	cmp	dword [dark_mode],0
	je	.light
	fastcall UahSetPreferredAppMode,UAH_APPMODE_FORCEDARK
	fastcall UahMenuHook_SetDark,1
	jmp	.refresh
  .light:
	fastcall UahSetPreferredAppMode,UAH_APPMODE_FORCELIGHT
	fastcall UahMenuHook_SetDark,0
  .refresh:
	fastcall UahRefreshImmersiveColorPolicyState
	fastcall UahFlushMenuThemes
	cmp	qword [hMain],0
	je	.done
	fastcall UahAllowDarkModeForWindow,[hMain],dword [dark_mode]
  .done:
	ret
endp

proc ToggleTheme
	xor	dword [dark_mode],1
	fastcall ApplyMenuAppMode
	fastcall ApplyRichTheme
	ret
endp
