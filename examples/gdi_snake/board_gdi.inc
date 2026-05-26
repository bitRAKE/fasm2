; board_gdi.inc - off-screen board rendering and WM_ERASEBKGND blitting.

proc CreateGameBrushes
	iterate <name,	colorref>,\
		board,	COLORREF_BOARD,\
		snake,	COLORREF_SNAKE,\
		red,	COLORREF_APPLE_RED,\
		gold,	COLORREF_APPLE_GOLD,\
		blue,	COLORREF_APPLE_BLUE,\
		purple,	COLORREF_APPLE_PURPLE,\
		cyan,	COLORREF_APPLE_CYAN

		invoke CreateSolidBrush, colorref
		mov [brush_#name], rax
	end iterate
	xor	eax,eax
	ret
endp

proc DestroyGameBrushes
	iterate name, board,snake,red,gold,blue,purple,cyan
		mov rcx, [brush_#name]
		jrcxz .%
		invoke DeleteObject, rcx
		and [brush_#name], 0
	.%:
	end iterate
	ret
endp

proc BrushForCell cell_value
	iterate <brush,	cell>,\
		snake,	SNAKE,\
		red,	APPLE_NORMAL,\
		gold,	APPLE_GROW,\
		purple,	APPLE_INVINCIBLE,\
		blue,	APPLE_TELEPORT,\
		cyan,	APPLE_SPLIT

		mov rax, [brush_#brush]
		cmp ecx, CELL_#cell
		jz @F
	end iterate
	xor eax, eax
@@:	retn
endp

proc DestroyBoardSurface
	cmp	qword [board_dc],0
	je	.done_dc
	cmp	qword [board_old_bitmap],0
	je	.no_select
	invoke	SelectObject,[board_dc],[board_old_bitmap]
  .no_select:
	cmp	qword [board_bitmap],0
	je	.no_bitmap
	invoke	DeleteObject,[board_bitmap]
  .no_bitmap:
	invoke	DeleteDC,[board_dc]
  .done_dc:
	mov	qword [board_dc],0
	mov	qword [board_bitmap],0
	mov	qword [board_old_bitmap],0
	ret
endp

proc CreateBoardSurface
    locals
	hdc dq ?
    endl

	fastcall DestroyBoardSurface
	mov	eax,[board_cols]
	imul	eax,BOARD_CELL_PX
	mov	[board_px_w],eax
	mov	eax,[board_rows]
	imul	eax,BOARD_CELL_PX
	mov	[board_px_h],eax

	invoke	GetDC,[hMain]
	mov	[hdc],rax
	test	rax,rax
	jz	.fail
	invoke	CreateCompatibleDC,[hdc]
	mov	[board_dc],rax
	test	rax,rax
	jz	.release_fail
	invoke	CreateCompatibleBitmap,[hdc],dword [board_px_w],dword [board_px_h]
	mov	[board_bitmap],rax
	test	rax,rax
	jz	.release_fail
	invoke	SelectObject,[board_dc],[board_bitmap]
	mov	[board_old_bitmap],rax
	invoke	ReleaseDC,[hMain],[hdc]
	mov	eax,1
	ret

  .release_fail:
	invoke	ReleaseDC,[hMain],[hdc]
	fastcall DestroyBoardSurface
  .fail:
	xor	eax,eax
	ret
endp

proc RenderBoard uses rbx r12 r13 r14
	cmp	qword [board_dc],0
	je	.done
	mov	dword [cell_rect.left],0
	mov	dword [cell_rect.top],0
	mov	eax,[board_px_w]
	mov	[cell_rect.right],eax
	mov	eax,[board_px_h]
	mov	[cell_rect.bottom],eax
	invoke	FillRect,[board_dc],addr cell_rect,[brush_board]

	xor	ebx,ebx
	xor	r12d,r12d
  .row_loop:
	cmp	r12d,[board_rows]
	jae	.done
	xor	r13d,r13d
  .col_loop:
	cmp	r13d,[board_cols]
	jae	.next_row
	movzx	ecx,byte [board_cells+rbx]
	test	ecx,ecx
	jz	.skip_cell
	fastcall BrushForCell,rcx
	test	rax,rax
	jz	.skip_cell
	mov	r14,rax
	mov	eax,r13d
	imul	eax,BOARD_CELL_PX
	mov	[cell_rect.left],eax
	add	eax,BOARD_CELL_PX
	dec	eax
	mov	[cell_rect.right],eax
	mov	eax,r12d
	imul	eax,BOARD_CELL_PX
	mov	[cell_rect.top],eax
	add	eax,BOARD_CELL_PX
	dec	eax
	mov	[cell_rect.bottom],eax
	invoke	FillRect,[board_dc],addr cell_rect,r14
  .skip_cell:
	inc	ebx
	inc	r13d
	jmp	.col_loop
  .next_row:
	inc	r12d
	jmp	.row_loop
  .done:
	xor	eax,eax
	ret
endp

proc StretchBoardToHdc hdc
    locals
	draw_w dd ?
	draw_h dd ?
    endl

	mov	[hdc],rcx
	invoke	GetClientRect,[hMain],addr client_rect
	cmp	qword [board_dc],0
	je	.fill
	mov	eax,[client_rect.right]
	sub	eax,[client_rect.left]
	mov	[draw_w],eax
	mov	edx,[client_rect.bottom]
	sub	edx,[client_rect.top]
	mov	[draw_h],edx
	invoke	SetStretchBltMode,[hdc],HALFTONE
	invoke	StretchBlt,[hdc],0,0,dword [draw_w],dword [draw_h],[board_dc],0,0,\
		dword [board_px_w],dword [board_px_h],SRCCOPY
	mov	eax,1
	ret
  .fill:
	invoke	FillRect,[hdc],addr client_rect,[brush_board]
	mov	eax,1
	ret
endp
