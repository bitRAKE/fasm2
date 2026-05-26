; board_gdi.inc - off-screen board rendering and WM_ERASEBKGND blitting.

proc CreateGameBrushes
	invoke	CreateSolidBrush,COLORREF_BOARD
	mov	[brush_board],rax
	invoke	CreateSolidBrush,COLORREF_SNAKE
	mov	[brush_snake],rax
	invoke	CreateSolidBrush,COLORREF_APPLE_RED
	mov	[brush_red],rax
	invoke	CreateSolidBrush,COLORREF_APPLE_GOLD
	mov	[brush_gold],rax
	invoke	CreateSolidBrush,COLORREF_APPLE_BLUE
	mov	[brush_blue],rax
	invoke	CreateSolidBrush,COLORREF_APPLE_PURPLE
	mov	[brush_purple],rax
	invoke	CreateSolidBrush,COLORREF_APPLE_CYAN
	mov	[brush_cyan],rax
	xor	eax,eax
	ret
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

proc DestroyGameBrushes
	cmp	qword [brush_board],0
	je	.no_board
	invoke	DeleteObject,[brush_board]
  .no_board:
	cmp	qword [brush_snake],0
	je	.no_snake
	invoke	DeleteObject,[brush_snake]
  .no_snake:
	cmp	qword [brush_red],0
	je	.no_red
	invoke	DeleteObject,[brush_red]
  .no_red:
	cmp	qword [brush_gold],0
	je	.no_gold
	invoke	DeleteObject,[brush_gold]
  .no_gold:
	cmp	qword [brush_blue],0
	je	.no_blue
	invoke	DeleteObject,[brush_blue]
  .no_blue:
	cmp	qword [brush_purple],0
	je	.no_purple
	invoke	DeleteObject,[brush_purple]
  .no_purple:
	cmp	qword [brush_cyan],0
	je	.done
	invoke	DeleteObject,[brush_cyan]
  .done:
	mov	qword [brush_board],0
	mov	qword [brush_snake],0
	mov	qword [brush_red],0
	mov	qword [brush_gold],0
	mov	qword [brush_blue],0
	mov	qword [brush_purple],0
	mov	qword [brush_cyan],0
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

proc BrushForCell cell_value
	mov	eax,ecx
	cmp	eax,CELL_SNAKE
	je	.snake
	cmp	eax,CELL_APPLE_NORMAL
	je	.red
	cmp	eax,CELL_APPLE_GROW
	je	.gold
	cmp	eax,CELL_APPLE_INVINCIBLE
	je	.purple
	cmp	eax,CELL_APPLE_TELEPORT
	je	.blue
	cmp	eax,CELL_APPLE_SPLIT
	je	.cyan
	xor	eax,eax
	ret
  .snake:
	mov	rax,[brush_snake]
	ret
  .red:
	mov	rax,[brush_red]
	ret
  .gold:
	mov	rax,[brush_gold]
	ret
  .purple:
	mov	rax,[brush_purple]
	ret
  .blue:
	mov	rax,[brush_blue]
	ret
  .cyan:
	mov	rax,[brush_cyan]
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
