; game_logic.inc - board sizing, snake simulation, apples, bonuses, and AI.

proc ClearBoard
	xor	ecx,ecx
	mov	edx,[board_cell_count]
  .loop:
	cmp	ecx,edx
	jae	.done
	mov	byte [board_cells+rcx],CELL_EMPTY
	inc	ecx
	jmp	.loop
  .done:
	ret
endp

proc BoardCellFromXY xvalue,yvalue
	mov	eax,edx
	imul	eax,[board_cols]
	add	eax,ecx
	ret
endp

proc IsReverseDirection old_dir,new_dir
	mov	eax,ecx
	xor	eax,2
	cmp	eax,edx
	sete	al
	movzx	eax,al
	ret
endp

proc SetPendingDirection new_dir
	mov	eax,[snake_direction]
	xor	eax,2
	cmp	eax,ecx
	je	.done
	mov	[snake_pending_direction],ecx
  .done:
	ret
endp

proc QueueBonus bonus_type
	mov	r8d,ecx
	cmp	dword [bonus_count],BONUS_QUEUE_MAX
	jb	.have_room
	fastcall SetGameMessage,bonus_full_text,50
	xor	eax,eax
	ret
  .have_room:
	mov	eax,[bonus_head]
	add	eax,[bonus_count]
	xor	edx,edx
	mov	ecx,BONUS_QUEUE_MAX
	div	ecx
	mov	byte [bonus_queue+rdx],r8b
	inc	dword [bonus_count]
	mov	eax,1
	ret
endp

proc DequeueBonus
	cmp	dword [bonus_count],0
	ja	.have_bonus
	xor	eax,eax
	ret
  .have_bonus:
	mov	ecx,[bonus_head]
	movzx	eax,byte [bonus_queue+rcx]
	inc	ecx
	cmp	ecx,BONUS_QUEUE_MAX
	jb	.store
	xor	ecx,ecx
  .store:
	mov	[bonus_head],ecx
	dec	dword [bonus_count]
	ret
endp

proc RemoveSnakeFromBoard uses rbx
	xor	ebx,ebx
  .loop:
	cmp	ebx,[snake_length]
	jae	.done
	mov	ecx,[snake_cells+rbx*4]
	mov	byte [board_cells+rcx],CELL_EMPTY
	inc	ebx
	jmp	.loop
  .done:
	ret
endp

proc PlaceSnakeAt uses rbx, xvalue,yvalue,length_value
	mov	[xvalue],rcx
	mov	[yvalue],rdx
	mov	[length_value],r8
	mov	eax,dword [length_value]
	cmp	eax,START_SNAKE_LENGTH
	jae	.check_cols
	mov	eax,START_SNAKE_LENGTH
  .check_cols:
	mov	ecx,[board_cols]
	dec	ecx
	cmp	eax,ecx
	jbe	.store_length
	mov	eax,ecx
  .store_length:
	mov	[snake_length],eax
	mov	dword [snake_direction],DIR_RIGHT
	mov	dword [snake_pending_direction],DIR_RIGHT
	mov	dword [snake_growth_pending],0

	xor	ebx,ebx
  .loop:
	cmp	ebx,[snake_length]
	jae	.done
	mov	ecx,dword [xvalue]
	sub	ecx,ebx
	mov	edx,dword [yvalue]
	fastcall BoardCellFromXY,rcx,rdx
	mov	[snake_cells+rbx*4],eax
	mov	byte [board_cells+rax],CELL_SNAKE
	inc	ebx
	jmp	.loop
  .done:
	ret
endp

proc PlaceSnakeCentered
	mov	ecx,[board_cols]
	shr	ecx,1
	cmp	ecx,START_SNAKE_LENGTH
	jae	.have_x
	mov	ecx,START_SNAKE_LENGTH
  .have_x:
	mov	edx,[board_rows]
	shr	edx,1
	fastcall PlaceSnakeAt,rcx,rdx,START_SNAKE_LENGTH
	ret
endp

proc SpawnApple uses rbx
	mov	ebx,256
  .random_try:
	fastcall RandomBelow,dword [board_cell_count]
	mov	ecx,eax
	cmp	byte [board_cells+rcx],CELL_EMPTY
	je	.place
	dec	ebx
	jnz	.random_try
	xor	ecx,ecx
  .scan:
	cmp	ecx,[board_cell_count]
	jae	.done
	cmp	byte [board_cells+rcx],CELL_EMPTY
	je	.place
	inc	ecx
	jmp	.scan

  .place:
	mov	[apple_cell],ecx
	fastcall RandomBelow,100
	mov	edx,CELL_APPLE_NORMAL
	cmp	dword [apples_eaten],3
	jb	.store
	cmp	eax,55
	jb	.store
	mov	edx,CELL_APPLE_GROW
	cmp	eax,75
	jb	.store
	mov	edx,CELL_APPLE_INVINCIBLE
	cmp	eax,85
	jb	.store
	mov	edx,CELL_APPLE_TELEPORT
	cmp	eax,95
	jb	.store
	mov	edx,CELL_APPLE_SPLIT
  .store:
	mov	[apple_type],edx
	mov	ecx,[apple_cell]
	mov	byte [board_cells+rcx],dl
  .done:
	ret
endp

proc UpdateLevelAndSpeed
	mov	eax,[apples_eaten]
	xor	edx,edx
	mov	ecx,5
	div	ecx
	inc	eax
	mov	[level],eax

	mov	eax,[board_cell_count]
	mov	ecx,80
	cmp	eax,320
	jb	.speed_done
	mov	ecx,100
	cmp	eax,700
	jb	.speed_done
	mov	ecx,125
  .speed_done:
	mov	eax,[level]
	cmp	eax,1
	jbe	.store
	dec	eax
	imul	eax,4
	cmp	ecx,eax
	jbe	.min_speed
	sub	ecx,eax
	cmp	ecx,55
	jae	.store
  .min_speed:
	mov	ecx,55
  .store:
	mov	[tick_ms],ecx
	invoke	SetTimer,[hMain],TIMER_ID,dword [tick_ms],0
	ret
endp

proc HandleApple apple_value
	inc	dword [apples_eaten]
	mov	eax,ecx
	cmp	eax,CELL_APPLE_GROW
	je	.grow
	cmp	eax,CELL_APPLE_INVINCIBLE
	je	.invincible
	cmp	eax,CELL_APPLE_TELEPORT
	je	.teleport
	cmp	eax,CELL_APPLE_SPLIT
	je	.split

	add	dword [score],10
	add	dword [snake_growth_pending],1
	fastcall SetGameMessage,apple_red_text,40
	jmp	.finish

  .grow:
	add	dword [score],25
	add	dword [snake_growth_pending],4
	fastcall SetGameMessage,apple_gold_text,40
	jmp	.finish

  .invincible:
	add	dword [score],30
	fastcall QueueBonus,BONUS_INVINCIBLE
	fastcall SetGameMessage,bonus_inv_text,60
	jmp	.finish

  .teleport:
	add	dword [score],35
	fastcall QueueBonus,BONUS_TELEPORT
	fastcall SetGameMessage,bonus_tele_text,60
	jmp	.finish

  .split:
	add	dword [score],45
	fastcall QueueBonus,BONUS_SPLIT
	fastcall SetGameMessage,bonus_split_text,60

  .finish:
	fastcall UpdateLevelAndSpeed
	fastcall SpawnApple
	ret
endp

proc TrimSnakeToLength uses rbx, new_length
	mov	[new_length],rcx
	mov	eax,dword [new_length]
	cmp	eax,START_SNAKE_LENGTH
	jae	.have_min
	mov	eax,START_SNAKE_LENGTH
  .have_min:
	cmp	eax,[snake_length]
	jae	.done
	mov	ebx,eax
  .clear_tail:
	cmp	ebx,[snake_length]
	jae	.store
	mov	ecx,[snake_cells+rbx*4]
	mov	byte [board_cells+rcx],CELL_EMPTY
	inc	ebx
	jmp	.clear_tail
  .store:
	mov	eax,dword [new_length]
	mov	[snake_length],eax
  .done:
	ret
endp

proc TeleportSnake
    locals
	target_len dd ?
	xpos dd ?
	ypos dd ?
    endl

	mov	eax,[snake_length]
	mov	[target_len],eax
	fastcall RemoveSnakeFromBoard
	mov	ecx,[apple_cell]
	cmp	ecx,[board_cell_count]
	jae	.size_for_row
	mov	byte [board_cells+rcx],CELL_EMPTY
  .size_for_row:
	mov	eax,[board_cols]
	sub	eax,2
	cmp	eax,[target_len]
	jae	.have_len
	mov	[target_len],eax
  .have_len:
	mov	ecx,[board_cols]
	sub	ecx,[target_len]
	fastcall RandomBelow,rcx
	add	eax,[target_len]
	mov	[xpos],eax
	fastcall RandomBelow,dword [board_rows]
	mov	[ypos],eax
	fastcall PlaceSnakeAt,dword [xpos],dword [ypos],dword [target_len]
	fastcall SpawnApple
	fastcall SetGameMessage,active_tele_text,50
	ret
endp

proc SplitSnake
	mov	eax,[snake_length]
	shr	eax,1
	fastcall TrimSnakeToLength,rax
	fastcall SetGameMessage,active_split_text,50
	ret
endp

proc ActivateQueuedBonus
	fastcall DequeueBonus
	test	eax,eax
	jnz	.have_bonus
	fastcall SetGameMessage,no_bonus_text,40
	ret
  .have_bonus:
	cmp	eax,BONUS_INVINCIBLE
	je	.invincible
	cmp	eax,BONUS_TELEPORT
	je	.teleport
	cmp	eax,BONUS_SPLIT
	je	.split
	ret
  .invincible:
	mov	dword [invincible_ticks],90
	fastcall SetGameMessage,active_inv_text,60
	ret
  .teleport:
	fastcall TeleportSnake
	ret
  .split:
	fastcall SplitSnake
	ret
endp

proc ResetRound
	fastcall ClearBoard
	fastcall PlaceSnakeCentered
	fastcall SpawnApple
	fastcall RenderBoard
	ret
endp

proc ResetAttract
	mov	dword [game_state],STATE_ATTRACT
	mov	dword [board_locked],0
	mov	dword [score],0
	mov	dword [lives],0
	mov	dword [level],1
	mov	dword [apples_eaten],0
	mov	dword [bonus_head],0
	mov	dword [bonus_count],0
	mov	dword [invincible_ticks],0
	mov	dword [message_ticks],0
	fastcall ResetRound
	ret
endp

proc StartNewGame
	mov	dword [board_locked],1
	mov	dword [game_state],STATE_PLAYING
	mov	dword [score],0
	mov	dword [lives],START_LIVES
	mov	dword [level],1
	mov	dword [apples_eaten],0
	mov	dword [bonus_head],0
	mov	dword [bonus_count],0
	mov	dword [invincible_ticks],0
	mov	dword [message_ticks],0
	fastcall UpdateLevelAndSpeed
	fastcall ResetRound
	invoke	InvalidateRect,[hMain],0,1
	ret
endp

proc LoseLife
	cmp	dword [game_state],STATE_ATTRACT
	jne	.playing
	fastcall ResetAttract
	ret
  .playing:
	cmp	dword [invincible_ticks],0
	je	.hurt
	fastcall SetGameMessage,inv_hit_text,30
	ret
  .hurt:
	dec	dword [lives]
	cmp	dword [lives],0
	jg	.next_life
	fastcall FinishGameWithHighScores
	ret
  .next_life:
	fastcall SetGameMessage,life_lost_text,70
	fastcall ResetRound
	ret
endp

proc ComputeNextCell direction_value
    locals
	xpos dd ?
	ypos dd ?
    endl

	mov	eax,[snake_cells]
	xor	edx,edx
	div	dword [board_cols]
	mov	[ypos],eax
	mov	[xpos],edx

	cmp	ecx,DIR_UP
	je	.up
	cmp	ecx,DIR_RIGHT
	je	.right
	cmp	ecx,DIR_DOWN
	je	.down
	dec	dword [xpos]
	jmp	.check
  .up:
	dec	dword [ypos]
	jmp	.check
  .right:
	inc	dword [xpos]
	jmp	.check
  .down:
	inc	dword [ypos]

  .check:
	cmp	dword [invincible_ticks],0
	je	.bounds
	cmp	dword [xpos],0
	jge	.inv_x_high
	mov	eax,[board_cols]
	dec	eax
	mov	[xpos],eax
  .inv_x_high:
	mov	eax,[board_cols]
	cmp	dword [xpos],eax
	jb	.inv_y_low
	mov	dword [xpos],0
  .inv_y_low:
	cmp	dword [ypos],0
	jge	.inv_y_high
	mov	eax,[board_rows]
	dec	eax
	mov	[ypos],eax
  .inv_y_high:
	mov	eax,[board_rows]
	cmp	dword [ypos],eax
	jb	.to_cell
	mov	dword [ypos],0
	jmp	.to_cell

  .bounds:
	cmp	dword [xpos],0
	jl	.out
	cmp	dword [ypos],0
	jl	.out
	mov	eax,[board_cols]
	cmp	dword [xpos],eax
	jae	.out
	mov	eax,[board_rows]
	cmp	dword [ypos],eax
	jae	.out

  .to_cell:
	fastcall BoardCellFromXY,dword [xpos],dword [ypos]
	ret
  .out:
	mov	eax,-1
	ret
endp

proc IsDirectionSafe direction_value
	mov	r8d,ecx
	fastcall IsReverseDirection,dword [snake_direction],r8
	test	eax,eax
	jnz	.no
	fastcall ComputeNextCell,r8
	cmp	eax,-1
	je	.no
	mov	ecx,eax
	movzx	eax,byte [board_cells+rcx]
	cmp	eax,CELL_SNAKE
	je	.no
	mov	eax,1
	ret
  .no:
	xor	eax,eax
	ret
endp

proc ChooseAIDirection uses rbx
    locals
	head_x dd ?
	head_y dd ?
	apple_x dd ?
	apple_y dd ?
    endl

	mov	eax,[snake_cells]
	xor	edx,edx
	div	dword [board_cols]
	mov	[head_y],eax
	mov	[head_x],edx
	mov	eax,[apple_cell]
	xor	edx,edx
	div	dword [board_cols]
	mov	[apple_y],eax
	mov	[apple_x],edx

	mov	eax,[apple_x]
	cmp	eax,[head_x]
	ja	.try_right
	jb	.try_left
	mov	eax,[apple_y]
	cmp	eax,[head_y]
	ja	.try_down
	jb	.try_up
	jmp	.fallback

  .try_right:
	fastcall IsDirectionSafe,DIR_RIGHT
	test	eax,eax
	jnz	.set_right
	jmp	.try_vertical
  .try_left:
	fastcall IsDirectionSafe,DIR_LEFT
	test	eax,eax
	jnz	.set_left
  .try_vertical:
	mov	eax,[apple_y]
	cmp	eax,[head_y]
	ja	.try_down
	jb	.try_up
	jmp	.fallback
  .try_down:
	fastcall IsDirectionSafe,DIR_DOWN
	test	eax,eax
	jnz	.set_down
	jmp	.fallback
  .try_up:
	fastcall IsDirectionSafe,DIR_UP
	test	eax,eax
	jnz	.set_up

  .fallback:
	xor	ebx,ebx
  .fallback_loop:
	cmp	ebx,4
	jae	.done
	fastcall IsDirectionSafe,rbx
	test	eax,eax
	jnz	.set_ebx
	inc	ebx
	jmp	.fallback_loop

  .set_right:
	mov	dword [snake_pending_direction],DIR_RIGHT
	ret
  .set_left:
	mov	dword [snake_pending_direction],DIR_LEFT
	ret
  .set_down:
	mov	dword [snake_pending_direction],DIR_DOWN
	ret
  .set_up:
	mov	dword [snake_pending_direction],DIR_UP
	ret
  .set_ebx:
	mov	[snake_pending_direction],ebx
  .done:
	ret
endp

proc AdvanceSnake uses rbx
    locals
	new_cell dd ?
	grow_now dd ?
    endl

	mov	eax,[snake_pending_direction]
	mov	[snake_direction],eax
	fastcall ComputeNextCell,rax
	cmp	eax,-1
	jne	.have_cell
	fastcall LoseLife
	ret
  .have_cell:
	mov	[new_cell],eax
	mov	ecx,eax
	movzx	eax,byte [board_cells+rcx]
	mov	dword [grow_now],0

	cmp	eax,CELL_APPLE_NORMAL
	jb	.no_apple
	mov	dword [grow_now],1
	fastcall HandleApple,rax
	jmp	.check_tail

  .no_apple:
	cmp	dword [snake_growth_pending],0
	je	.check_tail
	mov	dword [grow_now],1

  .check_tail:
	cmp	dword [grow_now],0
	jne	.self_check
	mov	eax,[snake_length]
	dec	eax
	mov	ecx,[snake_cells+rax*4]
	mov	byte [board_cells+rcx],CELL_EMPTY

  .self_check:
	mov	ecx,[new_cell]
	cmp	byte [board_cells+rcx],CELL_SNAKE
	jne	.shift
	cmp	dword [invincible_ticks],0
	jne	.cut_through
	fastcall LoseLife
	ret
  .cut_through:
	mov	byte [board_cells+rcx],CELL_EMPTY
	fastcall SetGameMessage,inv_hit_text,25

  .shift:
	cmp	dword [grow_now],0
	je	.have_length
	mov	eax,[board_cell_count]
	cmp	dword [snake_length],eax
	jae	.have_length
	inc	dword [snake_length]
	cmp	dword [snake_growth_pending],0
	je	.have_length
	dec	dword [snake_growth_pending]

  .have_length:
	mov	ebx,[snake_length]
	dec	ebx
  .shift_loop:
	cmp	ebx,0
	jle	.store_head
	mov	eax,[snake_cells+rbx*4-4]
	mov	[snake_cells+rbx*4],eax
	dec	ebx
	jmp	.shift_loop
  .store_head:
	mov	eax,[new_cell]
	mov	[snake_cells],eax
	mov	byte [board_cells+rax],CELL_SNAKE
	ret
endp

proc GameTick
	cmp	dword [message_ticks],0
	je	.no_message
	dec	dword [message_ticks]
  .no_message:
	fastcall UpdateScorePage
	cmp	dword [invincible_ticks],0
	je	.no_invincible
	dec	dword [invincible_ticks]
  .no_invincible:
	cmp	dword [game_state],STATE_SCORES
	je	.render
	cmp	dword [game_state],STATE_ATTRACT
	jne	.advance
	fastcall ChooseAIDirection
  .advance:
	fastcall AdvanceSnake
  .render:
	fastcall RenderBoard
	invoke	InvalidateRect,[hMain],0,1
	ret
endp

proc UpdatePreviewBoard
    locals
	new_cols dd ?
	new_rows dd ?
    endl

	cmp	dword [board_locked],0
	jne	.done
	invoke	GetClientRect,[hMain],addr client_rect
	mov	eax,[client_rect.right]
	sub	eax,[client_rect.left]
	cdq
	mov	ecx,PREVIEW_CELL_PX
	idiv	ecx
	fastcall ClampBoardValue,rax,MIN_BOARD_COLS,MAX_BOARD_COLS
	mov	[new_cols],eax

	mov	eax,[client_rect.bottom]
	sub	eax,[client_rect.top]
	cdq
	mov	ecx,PREVIEW_CELL_PX
	idiv	ecx
	fastcall ClampBoardValue,rax,MIN_BOARD_ROWS,MAX_BOARD_ROWS
	mov	[new_rows],eax

	mov	eax,[new_cols]
	cmp	eax,[board_cols]
	jne	.changed
	mov	eax,[new_rows]
	cmp	eax,[board_rows]
	je	.done
  .changed:
	mov	eax,[new_cols]
	mov	[board_cols],eax
	mov	eax,[new_rows]
	mov	[board_rows],eax
	mov	eax,[board_cols]
	imul	eax,[board_rows]
	mov	[board_cell_count],eax
	fastcall CreateBoardSurface
	fastcall ResetAttract
	invoke	InvalidateRect,[hMain],0,1
  .done:
	ret
endp
