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

struct FONTICON_LAYER
  glyph dd ?
  color dd ?
ends

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

proc FontIcon_CreateFont uses rbx, pixels,facep
	mov	eax,ecx
	mov	rbx,rdx
	neg	eax
	invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		FONTICON_CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,rbx
	ret
endp

proc FontIcon_CreateFontForWindow uses rbx rsi, hwnd,pixels96,facep
	mov	ebx,edx
	mov	rsi,r8
	invoke	GetDpiForWindow,rcx
	test	eax,eax
	jnz	.have_dpi
	mov	eax,FONTICON_DPI_BASE
  .have_dpi:
	invoke	MulDiv,ebx,eax,FONTICON_DPI_BASE
	fastcall FontIcon_CreateFont,eax,rsi
	ret
endp

proc FontIcon_DrawGlyph uses rbx rsi rdi, hdc,glyph,rectp,hfont,color
    locals
	glyph_text dw ?,0
	old_font   dq ?
	old_color  dd ?
	old_bk     dd ?
    endl

	mov	rbx,rcx
	mov	rsi,r8
	mov	rdi,r9

	mov	word [glyph_text],dx
	mov	word [glyph_text+2],0

	invoke	SetBkMode,rbx,TRANSPARENT
	mov	[old_bk],eax
	invoke	SetTextColor,rbx,dword [color]
	mov	[old_color],eax
	invoke	SelectObject,rbx,rdi
	mov	[old_font],rax

	invoke	DrawTextW,rbx,addr glyph_text,1,rsi,\
		DT_CENTER or DT_VCENTER or DT_SINGLELINE or DT_NOPREFIX

	invoke	SelectObject,rbx,[old_font]
	invoke	SetTextColor,rbx,[old_color]
	invoke	SetBkMode,rbx,[old_bk]
	xor	eax,eax
	ret
endp

proc FontIcon_DrawLayeredGlyph uses rbx rsi rdi r12, hdc,baseGlyph,fillGlyph,rectp,hfont,baseColor,fillColor
	mov	rbx,rcx
	mov	esi,edx
	mov	edi,r8d
	mov	r12,r9
	fastcall FontIcon_DrawGlyph,rbx,edi,r12,[hfont],dword [fillColor]
	fastcall FontIcon_DrawGlyph,rbx,esi,r12,[hfont],dword [baseColor]
	xor	eax,eax
	ret
endp

proc FontIcon_DrawLayeredGlyphArray uses rbx rsi rdi r12, hdc,layerp,count,rectp,hfont
	mov	rdi,rcx
	mov	rsi,rdx
	mov	ebx,r8d
	mov	r12,r9
	test	ebx,ebx
	jz	.done
  .draw:
	fastcall FontIcon_DrawGlyph,rdi,\
		[rsi+FONTICON_LAYER.glyph],r12,[hfont],\
		[rsi+FONTICON_LAYER.color]
	add	rsi,sizeof.FONTICON_LAYER
	dec	ebx
	jnz	.draw
  .done:
	xor	eax,eax
	ret
endp

proc FontIcon_SetStaticGlyph uses rbx rsi, hwnd,glyph,hfont
    locals
	glyph_text dw ?,0
    endl

	mov	rbx,rcx
	mov	rsi,r8
	mov	word [glyph_text],dx
	mov	word [glyph_text+2],0
	invoke	SetWindowTextW,rbx,addr glyph_text
	test	rsi,rsi
	je	.done
	invoke	SendMessageW,rbx,WM_SETFONT,rsi,1
  .done:
	mov	eax,1
	ret
endp

proc FontIcon_PrepareControlColor uses rbx rsi rdi, hdc,color,brush
	mov	rdi,rcx
	mov	esi,edx
	mov	rbx,r8
	invoke	SetBkMode,rdi,TRANSPARENT
	invoke	SetTextColor,rdi,esi
	mov	rax,rbx
	ret
endp

proc FontIcon_ToolbarCustomDraw uses rbx rsi rdi, notify,hfont,color
	mov	rbx,rcx
	mov	rsi,rdx
	mov	edi,r8d
	cmp	[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_PREPAINT
	je	.prepaint
	cmp	[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_ITEMPREPAINT
	je	.item_prepaint
	xor	eax,eax
	ret
  .prepaint:
	mov	eax,CDRF_NOTIFYITEMDRAW
	ret
  .item_prepaint:
	test	rsi,rsi
	je	.no_font
	invoke	SelectObject,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.hdc],rsi
  .no_font:
	mov	[rbx+FONTICON_NMTBCUSTOMDRAW.clrText],edi
	mov	[rbx+FONTICON_NMTBCUSTOMDRAW.nStringBkMode],TRANSPARENT
	mov	eax,CDRF_NEWFONT or FONTICON_TBCDRF_USECDCOLORS
	ret
endp

proc FontIcon_ToolbarGlyphCustomDraw uses rbx rsi rdi r12 r13, notify,hfont,restColor,hotColor,pressedColor,disabledColor,restBack,hotBack,pressedBack,checkedBack
    locals
	draw_rect RECT
	item_state dd ?
	text_color dd ?
	back_color dd ?
	back_brush dq ?
    endl

	mov	rbx,rcx
	mov	rdi,rdx
	mov	r12d,r8d
	mov	r13d,r9d
	cmp	[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_PREPAINT
	je	.prepaint
	cmp	[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.dwDrawStage],CDDS_ITEMPREPAINT
	je	.item_prepaint
	xor	eax,eax
	ret

  .prepaint:
	mov	eax,CDRF_NOTIFYITEMDRAW
	ret

  .item_prepaint:
	mov	eax,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.uItemState]
	mov	[item_state],eax
	mov	[text_color],r12d
	mov	eax,dword [restBack]
	mov	[back_color],eax

	test	[item_state],FONTICON_CDIS_DISABLED
	jnz	.disabled
	test	[item_state],FONTICON_CDIS_SELECTED
	jnz	.pressed
	test	[item_state],FONTICON_CDIS_CHECKED
	jnz	.checked
	test	[item_state],FONTICON_CDIS_HOT
	jnz	.hot
	jmp	.colors_ready

  .hot:
	mov	[text_color],r13d
	mov	eax,dword [hotBack]
	mov	[back_color],eax
	jmp	.colors_ready

  .pressed:
	mov	eax,dword [pressedColor]
	mov	[text_color],eax
	mov	eax,dword [pressedBack]
	mov	[back_color],eax
	jmp	.colors_ready

  .checked:
	mov	eax,dword [pressedColor]
	mov	[text_color],eax
	mov	eax,dword [checkedBack]
	mov	[back_color],eax
	jmp	.colors_ready

  .disabled:
	mov	eax,dword [disabledColor]
	mov	[text_color],eax

  .colors_ready:
	lea	rsi,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.rc]
	mov	eax,[rsi+RECT.left]
	mov	[draw_rect.left],eax
	mov	eax,[rsi+RECT.top]
	mov	[draw_rect.top],eax
	mov	eax,[rsi+RECT.right]
	mov	[draw_rect.right],eax
	mov	eax,[rsi+RECT.bottom]
	mov	[draw_rect.bottom],eax

	invoke	CreateSolidBrush,[back_color]
	mov	[back_brush],rax
	test	rax,rax
	jz	.no_fill
	invoke	FillRect,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.hdc],addr draw_rect,[back_brush]
	invoke	DeleteObject,[back_brush]

  .no_fill:
	test	[item_state],FONTICON_CDIS_SELECTED
	jz	.draw
	add	[draw_rect.left],1
	add	[draw_rect.top],1
	add	[draw_rect.right],1
	add	[draw_rect.bottom],1

  .draw:
	fastcall FontIcon_DrawGlyph,[rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.hdc],\
		dword [rbx+FONTICON_NMTBCUSTOMDRAW.nmcd.lItemlParam],\
		addr draw_rect,rdi,[text_color]
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
	mov	rcx,[old_bitmap]
	jrcxz	.no_old_bitmap
	invoke	SelectObject,[memdc],rcx
  .no_old_bitmap:
	mov	rcx,[old_mask]
	jrcxz	.no_old_mask
	invoke	SelectObject,[maskdc],rcx
  .no_old_mask:
	mov	rcx,[maskdc]
	jrcxz	.no_mask_dc
	invoke	DeleteDC,rcx
  .no_mask_dc:
	mov	rcx,[hmask]
	jrcxz	.no_mask
	invoke	DeleteObject,rcx
  .no_mask:
	mov	rcx,[hbitmap]
	jrcxz	.no_bitmap
	invoke	DeleteObject,rcx
  .no_bitmap:
	mov	rcx,[hfont]
	jrcxz	.no_font
	invoke	DeleteObject,rcx
  .no_font:
	mov	rcx,[memdc]
	jrcxz	.no_dc
	invoke	DeleteDC,rcx
  .no_dc:
	mov	rax,[created_cursor]
	ret
endp
