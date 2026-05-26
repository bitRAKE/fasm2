; modern.asm - modular Win64 GUI example with resources and MRU state.
;
; This example mirrors the structure used by larger applications:
;   * windows.inc owns the executable format and local Win32 policy.
;   * common.inc contains small reusable helper procs.
;   * feature modules own their data, BSS, and procedures.
;   * start stays mostly orchestration.

include 'windows.inc'
include 'resource.h'

include 'common.inc'
include 'app_state.inc'
include 'mru_api.inc'
include 'mru_recent.inc'
include 'dialogs.inc'
include 'file_io.inc'
include 'find_dialog.inc'
include 'ui_main.inc'

proc start
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
	jz	.fatal_register

	fastcall LoadMainMenu
	test	rax,rax
	jz	.fatal_menu

	invoke	LoadAccelerators,[hInstance],IDR_ACCELERATORS
	mov	[hAccel],rax
	test	rax,rax
	jz	.fatal_accelerators

	call	LoadMRUAPI
	test	eax,eax
	jz	.no_mru
	fastcall MRUOpen,addr recent_files
	test	eax,eax
	jnz	.create_window
  .no_mru:
	invoke	MessageBox,0,'The comctl32 MRU ordinals could not be bound. The editor will run without recent-file persistence.',app_name,MB_ICONWARNING

  .create_window:
	invoke	CreateWindowEx,0,class_name,app_name,\
		WS_OVERLAPPEDWINDOW,\
		CW_USEDEFAULT,CW_USEDEFAULT,980,680,\
		0,[hMenubar],[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	.fatal_window

	invoke	ShowWindow,[hMain],SW_SHOWDEFAULT
	invoke	UpdateWindow,[hMain]

  .message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	.shutdown
	invoke	TranslateAcceleratorW,[hMain],[hAccel],addr msg
	test	eax,eax
	jnz	.message_loop
	cmp	qword [hFindDlg],0
	je	.translate
	invoke	IsDialogMessage,[hFindDlg],addr msg
	test	eax,eax
	jnz	.message_loop
  .translate:
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	.message_loop

  .shutdown:
	fastcall MRUClose,addr recent_files
	invoke	ExitProcess,[msg.wParam]

  .fatal_register:
	invoke	MessageBox,0,'RegisterClassExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  .fatal_menu:
	invoke	MessageBox,0,'Menu construction failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  .fatal_accelerators:
	invoke	MessageBox,0,'Accelerator table load failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  .fatal_window:
	invoke	MessageBox,0,'CreateWindowExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1
endp
