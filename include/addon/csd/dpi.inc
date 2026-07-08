; addon/csd/dpi.inc - DPI helpers for the CSD examples.

CSD_TEXT_FONT_PX96 = 13

proc CsdGetWindowDpi hwnd
	invoke	GetDpiForWindow,rcx
	test	eax,eax
	jnz	dpi_done
	mov	eax,CSD_DPI_BASE
  dpi_done:
	ret
endp

proc CsdGetResizeBorderForDpi dpi
    locals
	dpi_value dd ?
    endl

	mov	[dpi_value],ecx
	invoke	GetSystemMetricsForDpi,SM_CXSIZEFRAME,dword [dpi_value]
	test	eax,eax
	jnz	border_done
	fastcall CsdDipToPx,8,dword [dpi_value]
  border_done:
	ret
endp

proc CsdAdjustWindowRectForDpi rectp,style,exstyle,dpi
    locals
	style_value	dd ?
	exstyle_value	dd ?
	dpi_value	dd ?
    endl

	mov	[style_value],edx
	mov	[exstyle_value],r8d
	mov	[dpi_value],r9d
	invoke	AdjustWindowRectExForDpi,rcx,dword [style_value],0,\
		dword [exstyle_value],dword [dpi_value]
	ret
endp

proc CsdCreateTextFontForDpi dpi
    locals
	dpi_value dd ?
    endl

	mov	[dpi_value],ecx
	cmp	dword [dpi_value],0
	jne	text_font_have_dpi
	mov	dword [dpi_value],CSD_DPI_BASE

  text_font_have_dpi:
	invoke	MulDiv,CSD_TEXT_FONT_PX96,dword [dpi_value],CSD_DPI_BASE
	neg	eax
	invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		CLEARTYPE_QUALITY,DEFAULT_PITCH or FF_DONTCARE,'Segoe UI'
	ret
endp

proc CsdApplySuggestedDpiRect uses rbx, hwnd,rectp
    locals
	width	dd ?
	height	dd ?
    endl

	mov	[hwnd],rcx
	mov	rbx,rdx
	mov	eax,[rbx+RECT.right]
	sub	eax,[rbx+RECT.left]
	mov	[width],eax
	mov	eax,[rbx+RECT.bottom]
	sub	eax,[rbx+RECT.top]
	mov	[height],eax
	invoke	SetWindowPos,[hwnd],0,\
		dword [rbx+RECT.left],dword [rbx+RECT.top],\
		dword [width],dword [height],\
		SWP_NOZORDER or SWP_NOACTIVATE
	ret
endp
