; high_scores.inc - arcade score table, entry dialog, and signed storage.

define __GLOBAL_DATA__ high_scores_data
define __GLOBAL_BSS__ high_scores_bss

macro high_scores_data
	score_class_name du 'Fasm2GdiSnakeScoreEntry',0
	score_entry_title du 'New High Score',0
	score_entry_prompt du 'Enter initials',0
	score_entry_ok du 'OK',0
	score_entry_cancel du 'Skip',0
	score_title_text du 'HIGH SCORES',0
	score_line_format du '%u   %ls   %u',0
	score_path_env du 'LOCALAPPDATA',0
	score_dir_suffix du '\Fasm2Examples',0
	score_file_suffix du '\gdi_snake.hsc',0
	score_font_face du 'Consolas',0

	default_score_values dd 50,30,20,10,5,2,1,0
	default_score_names du 'CPU',0,'BOT',0,'ASM',0,'GDI',0
			    du 'F2 ',0,'WIN',0,'PSK',0,'---',0

	align 8
	score_wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: ScoreEntryProc,\
		cbClsExtra: 0,\
		cbWndExtra: 0,\
		hInstance: 0,\
		hIcon: 0,\
		hCursor: 0,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszMenuName: 0,\
		lpszClassName: score_class_name,\
		hIconSm: 0
purge high_scores_data
end macro

macro high_scores_bss
	align 8
	high_scores	rd HIGH_SCORE_COUNT
	high_names	rw HIGH_SCORE_COUNT * HIGH_NAME_STRIDE
	score_path	rw MAX_PATH
	score_store	rb HIGH_SCORE_STORE_BYTES
	score_io_done	dd ?
	score_hash	dd ?
	score_seed	dd ?
	score_page_mode dd ?
	score_next_page_tick dd ?
	score_entry_rank dd ?
	score_entry_value dd ?
	score_entry_committed dd ?
	hScoreDlg	dq ?
	hScoreEdit	dq ?
	hScoreOk	dq ?
	hScoreCancel	dq ?
	score_font_title dq ?
	score_font_top dq ?
	score_font_second dq ?
	score_font_third dq ?
	score_font_normal dq ?
	score_entry_name rw HIGH_NAME_STRIDE
	score_line_buffer rw 96
purge high_scores_bss
end macro

proc MixHash value
	mov	eax,[score_hash]
	xor	eax,ecx
	imul	eax,01000193h
	add	eax,9E3779B9h
	mov	[score_hash],eax
	ret
endp

proc HashHighScores uses rbx rsi
	mov	dword [score_hash],0811C9DC5h
	xor	ebx,ebx
  .entry_loop:
	cmp	ebx,HIGH_SCORE_COUNT
	jae	.done
	fastcall MixHash,dword [high_scores+rbx*4]
	mov	eax,ebx
	shl	eax,3
	lea	rsi,[high_names+rax]
	movzx	ecx,word [rsi]
	fastcall MixHash,rcx
	movzx	ecx,word [rsi+2]
	fastcall MixHash,rcx
	movzx	ecx,word [rsi+4]
	fastcall MixHash,rcx
	inc	ebx
	jmp	.entry_loop
  .done:
	mov	eax,[score_hash]
	xor	eax,HIGH_SCORE_KEY
	ret
endp

proc InitDefaultHighScores uses rbx rsi rdi
	xor	ebx,ebx
  .loop:
	cmp	ebx,HIGH_SCORE_COUNT
	jae	.done
	mov	eax,[default_score_values+rbx*4]
	mov	[high_scores+rbx*4],eax
	mov	eax,ebx
	shl	eax,3
	lea	rsi,[default_score_names+rax]
	lea	rdi,[high_names+rax]
	movsq
	inc	ebx
	jmp	.loop
  .done:
	ret
endp

proc BuildHighScorePath
	invoke	GetEnvironmentVariableW,score_path_env,score_path,MAX_PATH
	test	eax,eax
	jz	.fail
	invoke	lstrcatW,score_path,score_dir_suffix
	invoke	CreateDirectoryW,score_path,0
	invoke	lstrcatW,score_path,score_file_suffix
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc NextStoreKey keyp
	mov	eax,ecx
	rol	eax,5
	add	eax,7F4A7C15h
	xor	eax,0A53C9E21h
	ret
endp

proc EncodeHighScoreStore uses rbx
    locals
	key dd ?
    endl

	fastcall HashHighScores
	mov	[score_hash],eax
	invoke	GetTickCount
	test	eax,eax
	jnz	.have_seed
	mov	eax,13579BDFh
  .have_seed:
	mov	[score_seed],eax

	mov	dword [score_store],HIGH_SCORE_MAGIC
	mov	dword [score_store+4],HIGH_SCORE_VERSION
	mov	eax,[score_hash]
	xor	eax,[score_seed]
	mov	dword [score_store+8],eax
	mov	eax,[score_seed]
	mov	dword [score_store+12],eax

	mov	eax,[score_seed]
	xor	eax,HIGH_SCORE_KEY
	mov	[key],eax
	xor	ebx,ebx
  .entry_loop:
	cmp	ebx,HIGH_SCORE_COUNT
	jae	.done
	mov	eax,[high_scores+rbx*4]
	xor	eax,[key]
	mov	ecx,ebx
	imul	ecx,HIGH_SCORE_ENTRY_DWORDS * 4
	mov	dword [score_store+16+rcx],eax
	fastcall NextStoreKey,dword [key]
	mov	[key],eax

	mov	eax,ebx
	shl	eax,3
	mov	edx,dword [high_names+rax]
	xor	edx,[key]
	mov	ecx,ebx
	imul	ecx,HIGH_SCORE_ENTRY_DWORDS * 4
	mov	dword [score_store+16+rcx+4],edx
	fastcall NextStoreKey,dword [key]
	mov	[key],eax

	mov	eax,ebx
	shl	eax,3
	mov	edx,dword [high_names+rax+4]
	xor	edx,[key]
	mov	ecx,ebx
	imul	ecx,HIGH_SCORE_ENTRY_DWORDS * 4
	mov	dword [score_store+16+rcx+8],edx
	fastcall NextStoreKey,dword [key]
	mov	[key],eax

	inc	ebx
	jmp	.entry_loop
  .done:
	ret
endp

proc DecodeHighScoreStore uses rbx
    locals
	key dd ?
    endl

	cmp	dword [score_store],HIGH_SCORE_MAGIC
	jne	.invalid
	cmp	dword [score_store+4],HIGH_SCORE_VERSION
	jne	.invalid
	mov	eax,dword [score_store+12]
	mov	[score_seed],eax
	xor	eax,HIGH_SCORE_KEY
	mov	[key],eax

	xor	ebx,ebx
  .entry_loop:
	cmp	ebx,HIGH_SCORE_COUNT
	jae	.validate
	mov	ecx,ebx
	imul	ecx,HIGH_SCORE_ENTRY_DWORDS * 4
	mov	eax,dword [score_store+16+rcx]
	xor	eax,[key]
	mov	[high_scores+rbx*4],eax
	fastcall NextStoreKey,dword [key]
	mov	[key],eax

	mov	ecx,ebx
	imul	ecx,HIGH_SCORE_ENTRY_DWORDS * 4
	mov	edx,dword [score_store+16+rcx+4]
	xor	edx,[key]
	mov	eax,ebx
	shl	eax,3
	mov	dword [high_names+rax],edx
	fastcall NextStoreKey,dword [key]
	mov	[key],eax

	mov	ecx,ebx
	imul	ecx,HIGH_SCORE_ENTRY_DWORDS * 4
	mov	edx,dword [score_store+16+rcx+8]
	xor	edx,[key]
	mov	eax,ebx
	shl	eax,3
	mov	dword [high_names+rax+4],edx
	fastcall NextStoreKey,dword [key]
	mov	[key],eax
	inc	ebx
	jmp	.entry_loop

  .validate:
	fastcall ValidateHighScores
	test	eax,eax
	jz	.invalid
	fastcall HashHighScores
	xor	eax,dword [score_store+12]
	cmp	eax,dword [score_store+8]
	jne	.invalid
	mov	eax,1
	ret
  .invalid:
	xor	eax,eax
	ret
endp

proc ValidateHighScores uses rbx rsi r12
	xor	ebx,ebx
	mov	r12d,0FFFFFFFFh
  .entry_loop:
	cmp	ebx,HIGH_SCORE_COUNT
	jae	.valid
	mov	eax,[high_scores+rbx*4]
	cmp	eax,9999999
	ja	.invalid
	cmp	eax,r12d
	ja	.invalid
	mov	r12d,eax
	mov	eax,ebx
	shl	eax,3
	lea	rsi,[high_names+rax]
	cmp	word [rsi+6],0
	jne	.invalid
	movzx	ecx,word [rsi]
	fastcall IsScoreNameChar,rcx
	test	eax,eax
	jz	.invalid
	movzx	ecx,word [rsi+2]
	fastcall IsScoreNameChar,rcx
	test	eax,eax
	jz	.invalid
	movzx	ecx,word [rsi+4]
	fastcall IsScoreNameChar,rcx
	test	eax,eax
	jz	.invalid
	inc	ebx
	jmp	.entry_loop
  .valid:
	mov	eax,1
	ret
  .invalid:
	xor	eax,eax
	ret
endp

proc IsScoreNameChar char_value
	mov	eax,ecx
	cmp	eax,'A'
	jb	.check_digit
	cmp	eax,'Z'
	jbe	.yes
  .check_digit:
	cmp	eax,'0'
	jb	.check_space
	cmp	eax,'9'
	jbe	.yes
  .check_space:
	cmp	eax,' '
	je	.yes
	cmp	eax,'-'
	je	.yes
	xor	eax,eax
	ret
  .yes:
	mov	eax,1
	ret
endp

proc SaveHighScores
    locals
	hfile dq ?
    endl

	fastcall BuildHighScorePath
	test	eax,eax
	jz	.done
	fastcall EncodeHighScoreStore
	invoke	CreateFileW,score_path,GENERIC_WRITE,0,0,CREATE_ALWAYS,\
		FILE_ATTRIBUTE_NORMAL,0
	cmp	rax,INVALID_HANDLE_VALUE
	je	.done
	mov	[hfile],rax
	invoke	WriteFile,[hfile],score_store,HIGH_SCORE_STORE_BYTES,addr score_io_done,0
	invoke	CloseHandle,[hfile]
  .done:
	ret
endp

proc LoadHighScores
    locals
	hfile dq ?
    endl

	fastcall InitDefaultHighScores
	fastcall BuildHighScorePath
	test	eax,eax
	jz	.done
	invoke	CreateFileW,score_path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,\
		FILE_ATTRIBUTE_NORMAL,0
	cmp	rax,INVALID_HANDLE_VALUE
	je	.save_defaults
	mov	[hfile],rax
	invoke	ReadFile,[hfile],score_store,HIGH_SCORE_STORE_BYTES,addr score_io_done,0
	invoke	CloseHandle,[hfile]
	cmp	dword [score_io_done],HIGH_SCORE_STORE_BYTES
	jne	.reset
	fastcall DecodeHighScoreStore
	test	eax,eax
	jnz	.done
  .reset:
	fastcall InitDefaultHighScores
  .save_defaults:
	fastcall SaveHighScores
  .done:
	ret
endp

proc CreateScoreFont height_value,weight_value
	mov	[height_value],rcx
	mov	[weight_value],rdx
	invoke	CreateFontW,dword [height_value],0,0,0,dword [weight_value],\
		0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
		DEFAULT_QUALITY,DEFAULT_PITCH,score_font_face
	ret
endp

proc CreateHighScoreFonts
	iterate <type,	values>,\
		title,	<-44,	FW_BOLD>,\
		top,	<-50,	FW_BOLD>,\
		second,	<-30,	FW_BOLD>,\
		third,	<-22,	FW_BOLD>,\
		normal,	<-16,	FW_BOLD>

		fastcall CreateScoreFont,values
		mov [score_font_#type], rax
	end iterate
	ret
endp

proc DestroyHighScoreFonts
	iterate type, title,top,second,third,normal
		mov rcx, [score_font_#type]
		jrcxz .%
		invoke	DeleteObject,rcx
	.%:
	end iterate
	ret
endp

proc InitHighScores
	fastcall CreateHighScoreFonts
	fastcall LoadHighScores
	mov	dword [score_page_mode],0
	invoke	GetTickCount
	add	eax,SCORE_PAGE_MS
	mov	[score_next_page_tick],eax
	ret
endp

proc BeginScorePage show_scores
	mov	[score_page_mode],ecx
	invoke	GetTickCount
	add	eax,SCORE_PAGE_MS
	mov	[score_next_page_tick],eax
	ret
endp

proc UpdateScorePage
	cmp	dword [game_state],STATE_PLAYING
	je	.done
	invoke	GetTickCount
	sub	eax,[score_next_page_tick]
	js	.done
	xor	dword [score_page_mode],1
	invoke	GetTickCount
	add	eax,SCORE_PAGE_MS
	mov	[score_next_page_tick],eax
  .done:
	ret
endp

proc FindHighScoreRank score_value
	mov	edx,ecx
	xor	ecx,ecx
  .loop:
	cmp	ecx,HIGH_SCORE_COUNT
	jae	.no_rank
	cmp	edx,[high_scores+rcx*4]
	ja	.found
	inc	ecx
	jmp	.loop
  .found:
	mov	eax,ecx
	ret
  .no_rank:
	mov	eax,-1
	ret
endp

proc PrepareEntryName uses rbx
	cmp	word [score_entry_name],0
	jne	.normalize
	mov	word [score_entry_name],'Y'
	mov	word [score_entry_name+2],'O'
	mov	word [score_entry_name+4],'U'
	mov	word [score_entry_name+6],0
	ret
  .normalize:
	xor	ebx,ebx
  .loop:
	cmp	ebx,HIGH_NAME_CHARS
	jae	.terminate
	movzx	eax,word [score_entry_name+rbx*2]
	test	eax,eax
	jnz	.have_char
	mov	eax,' '
	jmp	.store
  .have_char:
	cmp	eax,'a'
	jb	.check_allowed
	cmp	eax,'z'
	ja	.check_allowed
	sub	eax,32
  .check_allowed:
	mov	ecx,eax
	fastcall IsScoreNameChar,rcx
	test	eax,eax
	jnz	.store_original
	mov	eax,' '
	jmp	.store
  .store_original:
	movzx	eax,word [score_entry_name+rbx*2]
	cmp	eax,'a'
	jb	.store
	cmp	eax,'z'
	ja	.store
	sub	eax,32
  .store:
	mov	word [score_entry_name+rbx*2],ax
	inc	ebx
	jmp	.loop
  .terminate:
	mov	word [score_entry_name+6],0
	ret
endp

proc InsertHighScore uses rbx rsi rdi, rank_value,score_value,namep
	mov	r9d,ecx
	mov	r10d,edx
	mov	r11,r8
	mov	ebx,HIGH_SCORE_COUNT - 1
  .shift_loop:
	cmp	ebx,r9d
	jle	.insert
	mov	eax,[high_scores+rbx*4-4]
	mov	[high_scores+rbx*4],eax
	mov	eax,ebx
	shl	eax,3
	lea	rdi,[high_names+rax]
	sub	rax,8
	lea	rsi,[high_names+rax]
	movsq
	dec	ebx
	jmp	.shift_loop
  .insert:
	mov	ebx,r9d
	mov	eax,r10d
	mov	[high_scores+rbx*4],eax
	mov	eax,ebx
	shl	eax,3
	lea	rdi,[high_names+rax]
	mov	rsi,r11
	movsq
	fastcall SaveHighScores
	ret
endp

proc CommitScoreEntry
	cmp	dword [score_entry_committed],0
	jne	.close
	invoke	GetWindowTextW,[hScoreEdit],score_entry_name,HIGH_NAME_STRIDE
	fastcall PrepareEntryName
	mov	dword [score_entry_committed],1
	fastcall InsertHighScore,dword [score_entry_rank],dword [score_entry_value],score_entry_name
  .close:
	invoke	DestroyWindow,[hScoreDlg]
	ret
endp

proc SkipScoreEntry
	cmp	dword [score_entry_committed],0
	jne	.close
	mov	word [score_entry_name],'Y'
	mov	word [score_entry_name+2],'O'
	mov	word [score_entry_name+4],'U'
	mov	word [score_entry_name+6],0
	mov	dword [score_entry_committed],1
	fastcall InsertHighScore,dword [score_entry_rank],dword [score_entry_value],score_entry_name
  .close:
	invoke	DestroyWindow,[hScoreDlg]
	ret
endp

proc ShowHighScoreEntry rank_value,score_value
    locals
	xpos dd ?
	ypos dd ?
    endl

	cmp	qword [hScoreDlg],0
	jne	.focus_existing
	mov	[score_entry_rank],ecx
	mov	[score_entry_value],edx
	mov	dword [score_entry_committed],0
	mov	word [score_entry_name],0

	invoke	GetWindowRect,[hMain],addr window_rect
	mov	eax,[window_rect.right]
	sub	eax,[window_rect.left]
	sub	eax,SCORE_ENTRY_W
	sar	eax,1
	add	eax,[window_rect.left]
	mov	[xpos],eax
	mov	eax,[window_rect.bottom]
	sub	eax,[window_rect.top]
	sub	eax,SCORE_ENTRY_H
	sar	eax,1
	add	eax,[window_rect.top]
	mov	[ypos],eax

	invoke	EnableWindow,[hMain],0
	invoke	CreateWindowEx,WS_EX_DLGMODALFRAME or WS_EX_CONTROLPARENT,\
		score_class_name,score_entry_title,WS_POPUP or WS_CAPTION or WS_SYSMENU,\
		dword [xpos],dword [ypos],SCORE_ENTRY_W,SCORE_ENTRY_H,[hMain],0,[hInstance],0
	mov	[hScoreDlg],rax
	test	rax,rax
	jnz	.show
	invoke	EnableWindow,[hMain],1
	ret
  .show:
	invoke	ShowWindow,[hScoreDlg],SW_SHOW
	invoke	UpdateWindow,[hScoreDlg]
	invoke	SetFocus,[hScoreEdit]
	ret
  .focus_existing:
	invoke	SetFocus,[hScoreEdit]
	ret
endp

proc FinishGameWithHighScores
	fastcall FindHighScoreRank,dword [score]
	mov	[score_entry_rank],eax
	mov	dword [game_state],STATE_SCORES
	mov	dword [board_locked],0
	fastcall BeginScorePage,1
	cmp	dword [score_entry_rank],-1
	je	.done
	cmp	dword [score],0
	je	.done
	fastcall ShowHighScoreEntry,dword [score_entry_rank],dword [score]
  .done:
	invoke	InvalidateRect,[hMain],0,1
	ret
endp

proc FontForRank rank_value
	mov	eax,ecx
	test	eax,eax
	jnz	.second
	mov	rax,[score_font_top]
	ret
  .second:
	cmp	eax,1
	jne	.third
	mov	rax,[score_font_second]
	ret
  .third:
	cmp	eax,2
	jne	.normal
	mov	rax,[score_font_third]
	ret
  .normal:
	mov	rax,[score_font_normal]
	ret
endp

proc LineHeightForRank rank_value
	mov	eax,ecx
	test	eax,eax
	jnz	.second
	mov	eax,58
	ret
  .second:
	cmp	eax,1
	jne	.third
	mov	eax,38
	ret
  .third:
	cmp	eax,2
	jne	.normal
	mov	eax,30
	ret
  .normal:
	mov	eax,22
	ret
endp

proc DrawHighScores uses rbx, hdc
    locals
	ypos dd ?
	line_height dd ?
	fontp dq ?
	oldfont dq ?
	namep dq ?
    endl

	mov	[hdc],rcx
	mov	qword [oldfont],0
	invoke	GetClientRect,[hMain],addr client_rect
	invoke	SetBkMode,[hdc],TRANSPARENT
	invoke	SetTextColor,[hdc],COLORREF_ALERT
	mov	rax,[score_font_title]
	mov	[fontp],rax
	cmp	qword [fontp],0
	je	.no_title_font
	invoke	SelectObject,[hdc],[fontp]
	mov	[oldfont],rax
  .no_title_font:
	mov	eax,[client_rect.left]
	add	eax,20
	mov	[overlay_rect.left],eax
	mov	eax,[client_rect.right]
	sub	eax,20
	mov	[overlay_rect.right],eax
	mov	eax,[client_rect.top]
	add	eax,18 + HIGH_SCORE_TOP_OFFSET
	mov	[overlay_rect.top],eax
	add	eax,52
	mov	[overlay_rect.bottom],eax
	invoke	DrawText,[hdc],score_title_text,-1,addr overlay_rect,\
		DT_CENTER or DT_SINGLELINE or DT_NOPREFIX

	mov	eax,[overlay_rect.bottom]
	add	eax,8
	mov	[ypos],eax
	invoke	SetTextColor,[hdc],COLORREF_TEXT
	xor	ebx,ebx
  .entry_loop:
	cmp	ebx,HIGH_SCORE_COUNT
	jae	.done
	fastcall FontForRank,rbx
	mov	[fontp],rax
	cmp	qword [fontp],0
	je	.no_font
	invoke	SelectObject,[hdc],[fontp]
  .no_font:
	fastcall LineHeightForRank,rbx
	mov	[line_height],eax
	mov	eax,ebx
	shl	eax,3
	lea	rax,[high_names+rax]
	mov	[namep],rax
	mov	eax,ebx
	inc	eax
	invoke	wsprintf,score_line_buffer,score_line_format,eax,[namep],dword [high_scores+rbx*4]
	mov	eax,[client_rect.left]
	add	eax,28
	mov	[overlay_rect.left],eax
	mov	eax,[client_rect.right]
	sub	eax,28
	mov	[overlay_rect.right],eax
	mov	eax,[ypos]
	mov	[overlay_rect.top],eax
	add	eax,[line_height]
	mov	[overlay_rect.bottom],eax
	invoke	DrawText,[hdc],score_line_buffer,-1,addr overlay_rect,\
		DT_CENTER or DT_SINGLELINE or DT_NOPREFIX
	mov	eax,[line_height]
	add	eax,5
	add	[ypos],eax
	inc	ebx
	jmp	.entry_loop
  .done:
	cmp	qword [oldfont],0
	je	.ret
	invoke	SelectObject,[hdc],[oldfont]
  .ret:
	ret
endp

proc ScoreEntryProc hwnd,wmsg,wparam,lparam
	cmp	edx,WM_CREATE
	je	.wm_create
	cmp	edx,WM_COMMAND
	je	.wm_command
	cmp	edx,WM_CLOSE
	je	.wm_close
	cmp	edx,WM_DESTROY
	je	.wm_destroy

	invoke	DefWindowProc,rcx,edx,r8,r9
	ret

  .wm_create:
	mov	[hScoreDlg],rcx
	invoke	CreateWindowEx,0,'STATIC',score_entry_prompt,\
		WS_CHILD or WS_VISIBLE or SS_CENTER,\
		16,22,248,18,[hScoreDlg],0,[hInstance],0
	invoke	CreateWindowEx,WS_EX_CLIENTEDGE,'EDIT','',\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP or ES_UPPERCASE or ES_AUTOHSCROLL,\
		96,54,88,24,[hScoreDlg],IDC_SCORE_EDIT,[hInstance],0
	mov	[hScoreEdit],rax
	invoke	SendMessage,[hScoreEdit],EM_SETLIMITTEXT,HIGH_NAME_CHARS,0
	invoke	CreateWindowEx,0,'BUTTON',score_entry_ok,\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_DEFPUSHBUTTON,\
		70,104,62,26,[hScoreDlg],IDOK,[hInstance],0
	mov	[hScoreOk],rax
	invoke	CreateWindowEx,0,'BUTTON',score_entry_cancel,\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP or BS_PUSHBUTTON,\
		148,104,62,26,[hScoreDlg],IDCANCEL,[hInstance],0
	mov	[hScoreCancel],rax
	xor	eax,eax
	ret

  .wm_command:
	mov	eax,r8d
	movzx	ecx,ax
	cmp	ecx,IDOK
	je	.ok
	cmp	ecx,IDCANCEL
	je	.cancel
	xor	eax,eax
	ret
  .ok:
	fastcall CommitScoreEntry
	xor	eax,eax
	ret
  .cancel:
	fastcall SkipScoreEntry
	xor	eax,eax
	ret

  .wm_close:
	fastcall SkipScoreEntry
	xor	eax,eax
	ret

  .wm_destroy:
	mov	qword [hScoreDlg],0
	mov	qword [hScoreEdit],0
	invoke	EnableWindow,[hMain],1
	invoke	SetFocus,[hMain]
	invoke	InvalidateRect,[hMain],0,1
	xor	eax,eax
	ret
endp
