; app_state.inc - process storage and data owned by the executable.

define __GLOBAL_DATA__ app_state_data
define __GLOBAL_BSS__ app_state_bss

macro app_state_data
	app_name	du 'GDI Snake',0
	class_name	du 'Fasm2GdiSnake',0

	overlay_format	du 'Score: %u   Lives: %u   Level: %u   Board: %ux%u',13,10
			du 'WASD move   Space bonus(%u)   Esc exit',0
	attract_text	du 'Resize the borderless window to choose difficulty.',13,10
			du 'Any key starts. WASD turns. ESC exits.',0
	gameover_text	du 'Game over.',13,10
			du 'Resize to change difficulty, then press any key.',0
	no_bonus_text	du 'Bonus queue is empty.',0
	bonus_full_text	du 'Bonus queue is full.',0
	bonus_inv_text	du 'Invincible bonus queued. Press Space.',0
	bonus_tele_text du 'Teleport bonus queued. Press Space.',0
	bonus_split_text du 'Split bonus queued. Press Space.',0
	active_inv_text du 'Invincible snake.',0
	active_tele_text du 'Teleport activated.',0
	active_split_text du 'Split activated.',0
	apple_red_text	du 'Red apple: grow.',0
	apple_gold_text du 'Gold apple: grow more.',0
	life_lost_text	du 'Collision. Life lost.',0
	inv_hit_text	du 'Invincible hit ignored.',0

	align 8
	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		cbClsExtra: 0,\
		cbWndExtra: 0,\
		hInstance: 0,\
		hIcon: 0,\
		hCursor: 0,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszMenuName: 0,\
		lpszClassName: class_name,\
		hIconSm: 0
purge app_state_data
end macro

macro app_state_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	msg		MSG
	ps		PAINTSTRUCT
	client_rect	RECT
	window_rect	RECT
	cell_rect	RECT
	overlay_rect	RECT

	board_dc	dq ?
	board_bitmap	dq ?
	board_old_bitmap dq ?
	brush_board	dq ?
	brush_snake	dq ?
	brush_red	dq ?
	brush_gold	dq ?
	brush_blue	dq ?
	brush_purple	dq ?
	brush_cyan	dq ?

	board_cols	dd ?
	board_rows	dd ?
	board_cell_count dd ?
	board_px_w	dd ?
	board_px_h	dd ?
	board_locked	dd ?
	board_cells	rb MAX_BOARD_CELLS

	snake_cells	rd MAX_BOARD_CELLS
	snake_length	dd ?
	snake_direction dd ?
	snake_pending_direction dd ?
	snake_growth_pending dd ?

	apple_cell	dd ?
	apple_type	dd ?
	bonus_queue	rb BONUS_QUEUE_MAX
	bonus_head	dd ?
	bonus_count	dd ?
	invincible_ticks dd ?

	game_state	dd ?
	score		dd ?
	lives		dd ?
	level		dd ?
	apples_eaten	dd ?
	tick_ms		dd ?
	message_ticks	dd ?
	message_text	dq ?
	rng_state	dd ?

	overlay_buffer	rw 256
purge app_state_bss
end macro
