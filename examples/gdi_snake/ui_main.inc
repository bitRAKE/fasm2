; ui_main.inc - borderless window messages, keyboard controls, and overlay.

proc DrawOverlay hdc
	mov	[hdc],rcx
	invoke	GetClientRect,[hMain],addr client_rect
	invoke	SetBkMode,[hdc],TRANSPARENT
	invoke	SetTextColor,[hdc],COLORREF_TEXT

	cmp	dword [game_state],STATE_PLAYING
	je	.draw_state
	cmp	dword [score_page_mode],0
	je	.draw_title
	fastcall DrawHighScores,[hdc]
	jmp	.done

  .draw_title:
	mov	eax,[client_rect.left]
	add	eax,32
	mov	[overlay_rect.left],eax
	mov	eax,[client_rect.right]
	sub	eax,32
	mov	[overlay_rect.right],eax

	mov	eax,[client_rect.bottom]
	sub	eax,[client_rect.top]
	shr	eax,1
	add	eax,[client_rect.top]
	sub	eax,44
	mov	[overlay_rect.top],eax
	add	eax,88
	mov	[overlay_rect.bottom],eax

	cmp	dword [game_state],STATE_SCORES
	je	.game_over
	invoke	DrawText,[hdc],attract_text,-1,addr overlay_rect,\
		DT_CENTER or DT_WORDBREAK or DT_NOPREFIX
	jmp	.done
  .game_over:
	invoke	DrawText,[hdc],gameover_text,-1,addr overlay_rect,\
		DT_CENTER or DT_WORDBREAK or DT_NOPREFIX
	jmp	.done

  .draw_state:
	mov	eax,[client_rect.left]
	add	eax,14
	mov	[overlay_rect.left],eax
	mov	eax,[client_rect.top]
	add	eax,12
	mov	[overlay_rect.top],eax
	mov	eax,[client_rect.right]
	sub	eax,14
	mov	[overlay_rect.right],eax
	mov	eax,[client_rect.bottom]
	sub	eax,12
	mov	[overlay_rect.bottom],eax

	invoke	wsprintf,overlay_buffer,overlay_format,\
		dword [score],dword [lives],dword [level],\
		dword [board_cols],dword [board_rows],dword [bonus_count]
	invoke	DrawText,[hdc],overlay_buffer,-1,addr overlay_rect,\
		DT_LEFT or DT_TOP or DT_NOPREFIX

	cmp	dword [message_ticks],0
	je	.no_message
	cmp	qword [message_text],0
	je	.no_message
	invoke	SetTextColor,[hdc],COLORREF_ALERT
	mov	eax,[client_rect.left]
	add	eax,14
	mov	[overlay_rect.left],eax
	mov	eax,[client_rect.bottom]
	sub	eax,48
	mov	[overlay_rect.top],eax
	mov	eax,[client_rect.right]
	sub	eax,14
	mov	[overlay_rect.right],eax
	mov	eax,[client_rect.bottom]
	sub	eax,12
	mov	[overlay_rect.bottom],eax
	invoke	DrawText,[hdc],[message_text],-1,addr overlay_rect,\
		DT_LEFT or DT_BOTTOM or DT_SINGLELINE or DT_NOPREFIX

  .no_message:
  .done:
	ret
endp

proc SnapPixelSpan pixel_value,min_cells,max_cells
	mov	r9d,edx

	mov	eax,ecx
	add	eax,PREVIEW_CELL_PX / 2
	xor	edx,edx
	mov	ecx,PREVIEW_CELL_PX
	div	ecx
	fastcall ClampBoardValue,rax,r9,r8
	imul	eax,PREVIEW_CELL_PX
	ret
endp

proc SnapSizingRect edge,rectp
    locals
	width dd ?
	height dd ?
	snapped_w dd ?
	snapped_h dd ?
    endl

	mov	[edge],rcx
	mov	[rectp],rdx
	cmp	dword [board_locked],0
	je	.snap
	xor	eax,eax
	ret

  .snap:
	mov	r10,[rectp]
	mov	eax,[r10+RECT.right]
	sub	eax,[r10+RECT.left]
	mov	[width],eax
	mov	eax,[r10+RECT.bottom]
	sub	eax,[r10+RECT.top]
	mov	[height],eax

	fastcall SnapPixelSpan,dword [width],MIN_BOARD_COLS,MAX_BOARD_COLS
	mov	[snapped_w],eax
	fastcall SnapPixelSpan,dword [height],MIN_BOARD_ROWS,MAX_BOARD_ROWS
	mov	[snapped_h],eax

	mov	r10,[rectp]
	mov	eax,dword [edge]
	cmp	eax,WMSZ_LEFT
	je	.width_from_left
	cmp	eax,WMSZ_TOPLEFT
	je	.width_from_left
	cmp	eax,WMSZ_BOTTOMLEFT
	je	.width_from_left
	mov	ecx,[r10+RECT.left]
	add	ecx,[snapped_w]
	mov	[r10+RECT.right],ecx
	jmp	.height
  .width_from_left:
	mov	ecx,[r10+RECT.right]
	sub	ecx,[snapped_w]
	mov	[r10+RECT.left],ecx

  .height:
	mov	eax,dword [edge]
	cmp	eax,WMSZ_TOP
	je	.height_from_top
	cmp	eax,WMSZ_TOPLEFT
	je	.height_from_top
	cmp	eax,WMSZ_TOPRIGHT
	je	.height_from_top
	mov	ecx,[r10+RECT.top]
	add	ecx,[snapped_h]
	mov	[r10+RECT.bottom],ecx
	jmp	.done
  .height_from_top:
	mov	ecx,[r10+RECT.bottom]
	sub	ecx,[snapped_h]
	mov	[r10+RECT.top],ecx
  .done:
	mov	eax,1
	ret
endp

proc HitTestBorderless hwnd,lparam_value
    locals
	xpos dd ?
	ypos dd ?
	left_hit dd ?
	right_hit dd ?
	top_hit dd ?
	bottom_hit dd ?
    endl

	mov	r8,rcx
	cmp	dword [board_locked],0
	je	.resize_enabled
	mov	eax,HTCLIENT
	ret

  .resize_enabled:
	mov	eax,edx
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx
	invoke	GetWindowRect,r8,addr window_rect

	mov	dword [left_hit],0
	mov	dword [right_hit],0
	mov	dword [top_hit],0
	mov	dword [bottom_hit],0

	mov	eax,[window_rect.left]
	add	eax,RESIZE_GRIP_PX
	cmp	dword [xpos],eax
	jg	.check_right
	mov	dword [left_hit],1
  .check_right:
	mov	eax,[window_rect.right]
	sub	eax,RESIZE_GRIP_PX
	cmp	dword [xpos],eax
	jl	.check_top
	mov	dword [right_hit],1
  .check_top:
	mov	eax,[window_rect.top]
	add	eax,RESIZE_GRIP_PX
	cmp	dword [ypos],eax
	jg	.check_bottom
	mov	dword [top_hit],1
  .check_bottom:
	mov	eax,[window_rect.bottom]
	sub	eax,RESIZE_GRIP_PX
	cmp	dword [ypos],eax
	jl	.decide
	mov	dword [bottom_hit],1

  .decide:
	cmp	dword [top_hit],0
	je	.not_top
	cmp	dword [left_hit],0
	jne	.top_left
	cmp	dword [right_hit],0
	jne	.top_right
	mov	eax,HTTOP
	ret
  .not_top:
	cmp	dword [bottom_hit],0
	je	.not_bottom
	cmp	dword [left_hit],0
	jne	.bottom_left
	cmp	dword [right_hit],0
	jne	.bottom_right
	mov	eax,HTBOTTOM
	ret
  .not_bottom:
	cmp	dword [left_hit],0
	jne	.left
	cmp	dword [right_hit],0
	jne	.right
	mov	eax,HTCAPTION
	ret
  .top_left:
	mov	eax,HTTOPLEFT
	ret
  .top_right:
	mov	eax,HTTOPRIGHT
	ret
  .bottom_left:
	mov	eax,HTBOTTOMLEFT
	ret
  .bottom_right:
	mov	eax,HTBOTTOMRIGHT
	ret
  .left:
	mov	eax,HTLEFT
	ret
  .right:
	mov	eax,HTRIGHT
	ret
endp

proc HandleKeyDown vkey
	mov	eax,ecx
	cmp	eax,VK_ESCAPE
	jne	.not_escape
	invoke	PostMessage,[hMain],WM_CLOSE,0,0
	ret

  .not_escape:
	cmp	dword [game_state],STATE_PLAYING
	je	.playing
	fastcall StartNewGame
	ret

  .playing:
	cmp	eax,VK_SPACE
	je	.bonus
	cmp	eax,VK_W
	je	.up
	cmp	eax,VK_A
	je	.left
	cmp	eax,VK_S
	je	.down
	cmp	eax,VK_D
	je	.right
	ret
  .bonus:
	fastcall ActivateQueuedBonus
	fastcall RenderBoard
	invoke	InvalidateRect,[hMain],0,1
	ret
  .up:
	fastcall SetPendingDirection,DIR_UP
	ret
  .left:
	fastcall SetPendingDirection,DIR_LEFT
	ret
  .down:
	fastcall SetPendingDirection,DIR_DOWN
	ret
  .right:
	fastcall SetPendingDirection,DIR_RIGHT
	ret
endp

proc WindowProc hwnd,wmsg,wparam,lparam
	cmp	edx,WM_CREATE
	je	.wm_create
	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_SIZING
	je	.wm_sizing
	cmp	edx,WM_NCHITTEST
	je	.wm_nchittest
	cmp	edx,WM_KEYDOWN
	je	.wm_keydown
	cmp	edx,WM_TIMER
	je	.wm_timer
	cmp	edx,WM_ERASEBKGND
	je	.wm_erasebkgnd
	cmp	edx,WM_PAINT
	je	.wm_paint
	cmp	edx,WM_CLOSE
	je	.wm_close
	cmp	edx,WM_DESTROY
	je	.wm_destroy

	invoke	DefWindowProc,rcx,edx,r8,r9
	ret

  .wm_create:
	mov	[hMain],rcx
	invoke	GetTickCount
	mov	[rng_state],eax
	fastcall CreateGameBrushes
	fastcall InitHighScores
	fastcall UpdatePreviewBoard
	mov	dword [tick_ms],100
	invoke	SetTimer,[hMain],TIMER_ID,dword [tick_ms],0
	xor	eax,eax
	ret

  .wm_size:
	fastcall UpdatePreviewBoard
	xor	eax,eax
	ret

  .wm_sizing:
	fastcall SnapSizingRect,[wparam],[lparam]
	ret

  .wm_nchittest:
	fastcall HitTestBorderless,rcx,r9
	ret

  .wm_keydown:
	fastcall HandleKeyDown,r8
	xor	eax,eax
	ret

  .wm_timer:
	cmp	r8d,TIMER_ID
	jne	.done_zero
	fastcall GameTick
	jmp	.done_zero

  .wm_erasebkgnd:
	fastcall StretchBoardToHdc,r8
	ret

  .wm_paint:
	invoke	BeginPaint,[hMain],addr ps
	fastcall DrawOverlay,rax
	invoke	EndPaint,[hMain],addr ps
	xor	eax,eax
	ret

  .wm_close:
	invoke	DestroyWindow,[hMain]
	xor	eax,eax
	ret

  .wm_destroy:
	invoke	KillTimer,[hMain],TIMER_ID
	fastcall DestroyBoardSurface
	fastcall DestroyGameBrushes
	fastcall DestroyHighScoreFonts
	invoke	PostQuitMessage,0
  .done_zero:
	xor	eax,eax
	ret
endp
