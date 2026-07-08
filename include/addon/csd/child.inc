; addon/csd/child.inc - caption child-window placement helpers.

proc CsdCaptionMoveChild uses rbx rsi, hwndChild,geomp,index,dpi,padXDip,padYDip
    locals
	rc	RECT
	padx	dd ?
	pady	dd ?
	child_w	dd ?
	child_h	dd ?
	dpi_value dd ?
    endl

	mov	rbx,rcx
	mov	rsi,rdx
	mov	[dpi_value],r9d
	mov	eax,r8d
	imul	eax,sizeof.CSD_CAPTION_GEOMETRY
	add	rsi,rax

	invoke	CopyRect,addr rc,rsi
	fastcall CsdDipToPx,dword [padXDip],dword [dpi_value]
	mov	[padx],eax
	fastcall CsdDipToPx,dword [padYDip],dword [dpi_value]
	mov	[pady],eax
	mov	eax,[padx]
	add	[rc.left],eax
	sub	[rc.right],eax
	mov	eax,[pady]
	add	[rc.top],eax
	sub	[rc.bottom],eax
	mov	eax,[rc.right]
	sub	eax,[rc.left]
	mov	[child_w],eax
	mov	eax,[rc.bottom]
	sub	eax,[rc.top]
	mov	[child_h],eax
	invoke	MoveWindow,rbx,dword [rc.left],dword [rc.top],\
		dword [child_w],dword [child_h],1
	ret
endp
