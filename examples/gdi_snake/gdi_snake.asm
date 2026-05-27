; gdi_snake.asm - borderless GDI snake game example.
;
; The visible app is intentionally simple: WM_PAINT composes the board bitmap
; and overlay through a temporary frame buffer. The hidden complexity is divided
; into reusable modules for board rendering, simulation, and UI routing.

include 'windows.inc'

include 'constants.inc'
include 'app_state.inc'
include 'common.inc'
include 'board_gdi.inc'
include 'high_scores.inc'
include 'game_logic.inc'
include 'ui_main.inc'

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[wc.hInstance],rax
	mov	[score_wc.hInstance],rax

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[wc.hCursor],rax
	mov	[score_wc.hCursor],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[wc.hIcon],rax
	mov	[wc.hIconSm],rax
	mov	[score_wc.hIcon],rax
	mov	[score_wc.hIconSm],rax

	invoke	RegisterClassExW,addr wc
	test	rax,rax
	jz	.fatal_register

	invoke	RegisterClassExW,addr score_wc
	test	rax,rax
	jz	.fatal_score_register

	invoke	CreateWindowEx,WS_EX_APPWINDOW,class_name,app_name,\
		WS_POPUP or WS_VISIBLE,\
		120,120,720,504,\
		0,0,[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	.fatal_window

	invoke	ShowWindow,[hMain],SW_SHOW
	invoke	UpdateWindow,[hMain]

  .message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	.shutdown
	cmp	qword [hScoreDlg],0
	je	.translate
	invoke	IsDialogMessage,[hScoreDlg],addr msg
	test	eax,eax
	jnz	.message_loop
  .translate:
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	.message_loop

  .shutdown:
	invoke	ExitProcess,[msg.wParam]

  .fatal_register:
	invoke	MessageBox,0,'RegisterClassExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  .fatal_score_register:
	invoke	MessageBox,0,'High-score entry class registration failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  .fatal_window:
	invoke	MessageBox,0,'CreateWindowExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1
endp
