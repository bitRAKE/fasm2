; msgflood.asm - Win32 message-discovery harness with an edit-control log.
;
; The main window subscribes to several Win32 notification sources and writes
; formatted message traffic into a multiline EDIT. Each new line is inserted at
; character zero, so newest messages remain at the top while older messages are
; pushed down and eventually trimmed.

include 'windows.inc'

include 'constants.inc'
include 'app_state.inc'
include 'message_names.inc'
include 'log_edit.inc'
include 'sources.inc'
include 'menu.inc'
include 'ui_main.inc'

proc start
	fastcall LoadOptionalApis
	cmp	qword [pSetProcessDpiAwarenessContext],0
	je	start_dpi_ready
	invoke	pSetProcessDpiAwarenessContext,DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
  start_dpi_ready:

	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[wc.hInstance],rax

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[wc.hCursor],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[wc.hIcon],rax
	mov	[wc.hIconSm],rax

	invoke	RegisterClassExW,addr wc
	test	rax,rax
	jz	start_fatal_register

	invoke	CreateWindowEx,WS_EX_ACCEPTFILES,'Fasm2MsgFloodEditLog','msgflood edit log - V toggles noise, Q quits',\
		WS_OVERLAPPEDWINDOW,\
		CW_USEDEFAULT,CW_USEDEFAULT,720,460,\
		0,0,[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	start_fatal_window

	invoke	ShowWindow,[hMain],SW_SHOW
	invoke	UpdateWindow,[hMain]

  start_message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	start_shutdown
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	start_message_loop

  start_shutdown:
	invoke	ExitProcess,[msg.wParam]

  start_fatal_register:
	invoke	MessageBox,0,'RegisterClassExW failed.','MsgFlood Edit Log',MB_ICONERROR
	invoke	ExitProcess,1

  start_fatal_window:
	invoke	MessageBox,0,'CreateWindowExW failed.','MsgFlood Edit Log',MB_ICONERROR
	invoke	ExitProcess,1
endp
