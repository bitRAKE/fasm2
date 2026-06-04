; font_icons.inc - reusable GDI helpers for Segoe icon-font glyphs.

FONTICON_DPI_BASE		= 96
FONTICON_CLEARTYPE_QUALITY	= 5
FONTICON_DIB_RGB_COLORS		= 0
FONTICON_I_IMAGENONE		= -2
FONTICON_TBCDRF_USECDCOLORS	= 00800000h

FONTICON_CDIS_SELECTED		= 0001h
FONTICON_CDIS_DISABLED		= 0004h
FONTICON_CDIS_CHECKED		= 0008h
FONTICON_CDIS_HOT		= 0040h

struct FONTICON_BITMAPINFO32
  bmiHeader BITMAPINFOHEADER
  bmiColors dd ?
ends

struct FONTICON_ICONINFO
  fIcon    dd ?
  xHotspot dd ?
  yHotspot dd ?
           dd ?
  hbmMask  dq ?
  hbmColor dq ?
ends

struct FONTICON_NMCUSTOMDRAW
  hdr          NMHDR
  dwDrawStage  dd ?
               dd ?
  hdc          dq ?
  rc           RECT
  dwItemSpec   dq ?
  uItemState   dd ?
               dd ?
  lItemlParam  dq ?
ends

struct FONTICON_NMTBCUSTOMDRAW
  nmcd                 FONTICON_NMCUSTOMDRAW
  hbrMonoDither        dq ?
  hbrLines             dq ?
  hpenLines            dq ?
  clrText              dd ?
  clrMark              dd ?
  clrTextHighlight     dd ?
  clrBtnFace           dd ?
  clrBtnHighlight      dd ?
  clrHighlightHotTrack dd ?
  rcText               RECT
  nStringBkMode        dd ?
  nHLStringBkMode      dd ?
  iListGap             dd ?
                       dd ?
ends

proc FontIcon_CreateFont pixels,facep
	mov	dword [pixels],ecx
	mov	[facep],rdx
	mov	eax,dword [pixels]
	neg	eax
	invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,[facep]
	ret
endp

proc FontIcon_CreateFontForWindow hwnd,pixels96,facep
	mov	[hwnd],rcx
	mov	dword [pixels96],edx
	mov	[facep],r8
	invoke	GetDpiForWindow,[hwnd]
	test	eax,eax
	jnz	.have_dpi
	mov	eax,FONTICON_DPI_BASE
  .have_dpi:
	invoke	MulDiv,dword [pixels96],eax,FONTICON_DPI_BASE
	fastcall FontIcon_CreateFont,rax,[facep]
	ret
endp

proc FontIcon_DrawGlyph hdc,glyph,rectp,hfont,color
    locals
	glyph_text dw ?,0
	draw_rect  RECT
	old_font   dq ?
	old_color  dd ?
	old_bk     dd ?
    endl

	mov	[hdc],rcx
	mov	dword [glyph],edx
	mov	[rectp],r8
	mov	[hfont],r9

	mov	ax,word [glyph]
	mov	word [glyph_text],ax
	mov	word [glyph_text+2],0

	mov	rax,[rectp]
	mov	ecx,dword [rax+RECT.left]
	mov	dword [draw_rect.left],ecx
	mov	ecx,dword [rax+RECT.top]
	mov	dword [draw_rect.top],ecx
	mov	ecx,dword [rax+RECT.right]
	mov	dword [draw_rect.right],ecx
	mov	ecx,dword [rax+RECT.bottom]
	mov	dword [draw_rect.bottom],ecx

	invoke	SetBkMode,[hdc],TRANSPARENT
	mov	dword [old_bk],eax
	invoke	SetTextColor,[hdc],dword [color]
	mov	dword [old_color],eax
	invoke	SelectObject,[hdc],[hfont]
	mov	[old_font],rax

	invoke	DrawTextW,[hdc],addr glyph_text,1,addr draw_rect,\
		DT_CENTER or DT_VCENTER or DT_SINGLELINE or DT_NOPREFIX

	invoke	SelectObject,[hdc],[old_font]
	invoke	SetTextColor,[hdc],dword [old_color]
	invoke	SetBkMode,[hdc],dword [old_bk]
	xor	eax,eax
	ret
endp

proc FontIcon_DrawLayeredGlyph hdc,baseGlyph,fillGlyph,rectp,hfont,baseColor,fillColor
	mov	[hdc],rcx
	mov	dword [baseGlyph],edx
	mov	dword [fillGlyph],r8d
	mov	[rectp],r9
	fastcall FontIcon_DrawGlyph,[hdc],dword [fillGlyph],[rectp],[hfont],dword [fillColor]
	fastcall FontIcon_DrawGlyph,[hdc],dword [baseGlyph],[rectp],[hfont],dword [baseColor]
	xor	eax,eax
	ret
endp

proc FontIcon_SetStaticGlyph hwnd,glyph,hfont
    locals
	glyph_text dw ?,0
    endl

	mov	[hwnd],rcx
	mov	dword [glyph],edx
	mov	[hfont],r8
	mov	ax,word [glyph]
	mov	word [glyph_text],ax
	mov	word [glyph_text+2],0
	invoke	SetWindowTextW,[hwnd],addr glyph_text
	cmp	qword [hfont],0
	je	.done
	invoke	SendMessageW,[hwnd],WM_SETFONT,[hfont],1
  .done:
	mov	eax,1
	ret
endp

proc FontIcon_PrepareControlColor hdc,color,brush
	mov	[hdc],rcx
	mov	dword [color],edx
	mov	[brush],r8
	invoke	SetBkMode,[hdc],TRANSPARENT
	invoke	SetTextColor,[hdc],dword [color]
	mov	rax,[brush]
	ret
endp

proc FontIcon_ToolbarCustomDraw notify,hfont,color
	mov	[notify],rcx
	mov	[hfont],rdx
	mov	dword [color],r8d
	mov	rax,[notify]
	cmp	dword [rax+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_PREPAINT
	je	.prepaint
	cmp	dword [rax+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_ITEMPREPAINT
	je	.item_prepaint
	xor	eax,eax
	ret
  .prepaint:
	mov	eax,CDRF_NOTIFYITEMDRAW
	ret
  .item_prepaint:
	cmp	qword [hfont],0
	je	.no_font
	invoke	SelectObject,[rax+FONTICON_NMTBCUSTOMDRAW.nmcd.hdc],[hfont]
  .no_font:
	mov	rax,[notify]
	mov	ecx,dword [color]
	mov	dword [rax+FONTICON_NMTBCUSTOMDRAW.clrText],ecx
	mov	dword [rax+FONTICON_NMTBCUSTOMDRAW.nStringBkMode],TRANSPARENT
	mov	eax,CDRF_NEWFONT or FONTICON_TBCDRF_USECDCOLORS
	ret
endp

proc FontIcon_ToolbarGlyphCustomDraw uses rbx rsi, notify,hfont,restColor,hotColor,pressedColor,disabledColor,restBack,hotBack,pressedBack,checkedBack
    locals
	draw_rect RECT
	item_state dd ?
	text_color dd ?
	back_color dd ?
	back_brush dq ?
    endl

	mov	[notify],rcx
	mov	[hfont],rdx
	mov	dword [restColor],r8d
	mov	dword [hotColor],r9d
	mov	rbx,[notify]
	cmp	dword [rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_PREPAINT
	je	.prepaint
	cmp	dword [rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_ITEMPREPAINT
	je	.item_prepaint
	xor	eax,eax
	ret

  .prepaint:
	mov	eax,CDRF_NOTIFYITEMDRAW
	ret

  .item_prepaint:
	mov	eax,dword [rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.uItemState]
	mov	dword [item_state],eax
	mov	eax,dword [restColor]
	mov	dword [text_color],eax
	mov	eax,dword [restBack]
	mov	dword [back_color],eax

	test	dword [item_state],FONTICON_CDIS_DISABLED
	jnz	.disabled
	test	dword [item_state],FONTICON_CDIS_SELECTED
	jnz	.pressed
	test	dword [item_state],FONTICON_CDIS_CHECKED
	jnz	.checked
	test	dword [item_state],FONTICON_CDIS_HOT
	jnz	.hot
	jmp	.colors_ready

  .hot:
	mov	eax,dword [hotColor]
	mov	dword [text_color],eax
	mov	eax,dword [hotBack]
	mov	dword [back_color],eax
	jmp	.colors_ready

  .pressed:
	mov	eax,dword [pressedColor]
	mov	dword [text_color],eax
	mov	eax,dword [pressedBack]
	mov	dword [back_color],eax
	jmp	.colors_ready

  .checked:
	mov	eax,dword [pressedColor]
	mov	dword [text_color],eax
	mov	eax,dword [checkedBack]
	mov	dword [back_color],eax
	jmp	.colors_ready

  .disabled:
	mov	eax,dword [disabledColor]
	mov	dword [text_color],eax

  .colors_ready:
	lea	rsi,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.rc]
	mov	eax,dword [rsi+RECT.left]
	mov	dword [draw_rect.left],eax
	mov	eax,dword [rsi+RECT.top]
	mov	dword [draw_rect.top],eax
	mov	eax,dword [rsi+RECT.right]
	mov	dword [draw_rect.right],eax
	mov	eax,dword [rsi+RECT.bottom]
	mov	dword [draw_rect.bottom],eax

	invoke	CreateSolidBrush,dword [back_color]
	mov	[back_brush],rax
	test	rax,rax
	jz	.no_fill
	invoke	FillRect,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.hdc],addr draw_rect,[back_brush]
	invoke	DeleteObject,[back_brush]

  .no_fill:
	test	dword [item_state],FONTICON_CDIS_SELECTED
	jz	.draw
	add	dword [draw_rect.left],1
	add	dword [draw_rect.top],1
	add	dword [draw_rect.right],1
	add	dword [draw_rect.bottom],1

  .draw:
	fastcall FontIcon_DrawGlyph,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.hdc],\
		dword [rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.lItemlParam],\
		addr draw_rect,[hfont],dword [text_color]
	mov	eax,CDRF_SKIPDEFAULT
	ret
endp

proc FontIcon_CreateGlyphCursor uses rbx rsi rdi, glyph,pixels,hotx,hoty,color,facep
    locals
	bmi		FONTICON_BITMAPINFO32
	ii		FONTICON_ICONINFO
	rc		RECT
	bits		dq ?
	memdc		dq ?
	hfont		dq ?
	hbitmap		dq ?
	old_bitmap	dq ?
	hmask		dq ?
	maskdc		dq ?
	old_mask	dq ?
	created_cursor	dq ?
	count		dd ?
	alpha_value	dd ?
	red_value	dd ?
	green_value	dd ?
	blue_value	dd ?
    endl

	mov	dword [glyph],ecx
	mov	dword [pixels],edx
	mov	dword [hotx],r8d
	mov	dword [hoty],r9d

	mov	eax,dword [color]
	mov	edx,eax
	and	edx,0FFh
	mov	dword [red_value],edx
	mov	edx,eax
	shr	edx,8
	and	edx,0FFh
	mov	dword [green_value],edx
	shr	eax,16
	and	eax,0FFh
	mov	dword [blue_value],eax

	mov	qword [bits],0
	mov	qword [memdc],0
	mov	qword [hfont],0
	mov	qword [hbitmap],0
	mov	qword [old_bitmap],0
	mov	qword [hmask],0
	mov	qword [maskdc],0
	mov	qword [old_mask],0
	mov	qword [created_cursor],0

	fastcall FontIcon_CreateFont,dword [pixels],[facep]
	mov	[hfont],rax
	test	rax,rax
	jz	.cleanup

	invoke	CreateCompatibleDC,0
	mov	[memdc],rax
	test	rax,rax
	jz	.cleanup

	mov	dword [bmi.bmiHeader.biSize],sizeof.BITMAPINFOHEADER
	mov	eax,dword [pixels]
	mov	dword [bmi.bmiHeader.biWidth],eax
	neg	eax
	mov	dword [bmi.bmiHeader.biHeight],eax
	mov	word [bmi.bmiHeader.biPlanes],1
	mov	word [bmi.bmiHeader.biBitCount],32
	mov	dword [bmi.bmiHeader.biCompression],BI_RGB
	mov	eax,dword [pixels]
	imul	eax,dword [pixels]
	shl	eax,2
	mov	dword [bmi.bmiHeader.biSizeImage],eax
	mov	dword [bmi.bmiHeader.biXPelsPerMeter],0
	mov	dword [bmi.bmiHeader.biYPelsPerMeter],0
	mov	dword [bmi.bmiHeader.biClrUsed],0
	mov	dword [bmi.bmiHeader.biClrImportant],0
	mov	dword [bmi.bmiColors],0

	invoke	CreateDIBSection,[memdc],addr bmi,FONTICON_DIB_RGB_COLORS,addr bits,0,0
	mov	[hbitmap],rax
	test	rax,rax
	jz	.cleanup

	invoke	SelectObject,[memdc],[hbitmap]
	mov	[old_bitmap],rax
	invoke	PatBlt,[memdc],0,0,dword [pixels],dword [pixels],WHITENESS

	mov	dword [rc.left],0
	mov	dword [rc.top],0
	mov	eax,dword [pixels]
	mov	dword [rc.right],eax
	mov	dword [rc.bottom],eax
	fastcall FontIcon_DrawGlyph,[memdc],dword [glyph],addr rc,[hfont],0

	mov	eax,dword [pixels]
	imul	eax,dword [pixels]
	mov	dword [count],eax
	mov	rdi,[bits]
	mov	ecx,dword [count]
	test	ecx,ecx
	jz	.alpha_done

  .alpha_loop:
	movzx	eax,byte [rdi]
	movzx	edx,byte [rdi+1]
	add	eax,edx
	movzx	edx,byte [rdi+2]
	add	eax,edx
	xor	edx,edx
	mov	ebx,3
	div	ebx
	mov	esi,255
	sub	esi,eax
	mov	dword [alpha_value],esi

	mov	eax,dword [blue_value]
	imul	eax,esi
	add	eax,127
	xor	edx,edx
	mov	ebx,255
	div	ebx
	mov	byte [rdi],al

	mov	eax,dword [green_value]
	imul	eax,esi
	add	eax,127
	xor	edx,edx
	mov	ebx,255
	div	ebx
	mov	byte [rdi+1],al

	mov	eax,dword [red_value]
	imul	eax,esi
	add	eax,127
	xor	edx,edx
	mov	ebx,255
	div	ebx
	mov	byte [rdi+2],al

	mov	eax,dword [alpha_value]
	mov	byte [rdi+3],al
	add	rdi,4
	dec	ecx
	jnz	.alpha_loop

  .alpha_done:
	invoke	CreateBitmap,dword [pixels],dword [pixels],1,1,0
	mov	[hmask],rax
	test	rax,rax
	jz	.cleanup
	invoke	CreateCompatibleDC,0
	mov	[maskdc],rax
	test	rax,rax
	jz	.cleanup
	invoke	SelectObject,[maskdc],[hmask]
	mov	[old_mask],rax
	invoke	PatBlt,[maskdc],0,0,dword [pixels],dword [pixels],BLACKNESS

	mov	dword [ii.fIcon],0
	mov	eax,dword [hotx]
	mov	dword [ii.xHotspot],eax
	mov	eax,dword [hoty]
	mov	dword [ii.yHotspot],eax
	mov	rax,[hmask]
	mov	[ii.hbmMask],rax
	mov	rax,[hbitmap]
	mov	[ii.hbmColor],rax

	invoke	CreateIconIndirect,addr ii
	mov	[created_cursor],rax

  .cleanup:
	cmp	qword [old_bitmap],0
	je	.no_old_bitmap
	invoke	SelectObject,[memdc],[old_bitmap]
  .no_old_bitmap:
	cmp	qword [old_mask],0
	je	.no_old_mask
	invoke	SelectObject,[maskdc],[old_mask]
  .no_old_mask:
	cmp	qword [maskdc],0
	je	.no_mask_dc
	invoke	DeleteDC,[maskdc]
  .no_mask_dc:
	cmp	qword [hmask],0
	je	.no_mask
	invoke	DeleteObject,[hmask]
  .no_mask:
	cmp	qword [hbitmap],0
	je	.no_bitmap
	invoke	DeleteObject,[hbitmap]
  .no_bitmap:
	cmp	qword [hfont],0
	je	.no_font
	invoke	DeleteObject,[hfont]
  .no_font:
	cmp	qword [memdc],0
	je	.no_dc
	invoke	DeleteDC,[memdc]
  .no_dc:
	mov	rax,[created_cursor]
	ret
endp
