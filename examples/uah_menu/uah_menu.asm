; uah_menu.asm - tray/RichEdit example using UAH-themed native popup menus.
;
; The example keeps the reusable boundary visible:
;   * addon/uah.inc owns the WM_UAH* constants, structures, and uxtheme wrappers.
;   * subclass/uah_menu.inc owns CBT discovery of transient #32768 popup windows.
;   * this executable owns the theme policy and UAH draw/measure answers.

ADDON_WINDOWS_RESOURCE equ 'uah_menu.res'
include 'addon/windows.inc'
include 'resource.h'
include 'equates/richedit64.inc'

include 'addon/uah.inc'
include 'subclass/uah_menu.inc'

include 'constants.inc'
include 'app_state.inc'
include 'theme.inc'
include 'menu_painter.inc'
include 'menus.inc'
include 'richedit_ui.inc'
include 'tray.inc'
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

✓	invoke	RegisterClassExW,addr wc
	test	rax,rax
	jz	.fatal_msgbox

✓	invoke	LoadMenu,[hInstance],IDR_MAINMENU
	test	rax,rax
	jz	.fatal_msgbox
	mov	[hMenubar],rax

✓	invoke	LoadAccelerators,[hInstance],IDR_ACCELERATORS
	test	rax,rax
	jz	.fatal_msgbox
	mov	[hAccel],rax

✓	invoke	CreateWindowEx,0,class_name,'UAH Menu RichEdit',\
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,\
		CW_USEDEFAULT,CW_USEDEFAULT,900,560,\
		0,[hMenubar],[hInstance],0
	test	rax,rax
	jz	.fatal_msgbox
	mov	[hMain],rax

	invoke	ShowWindow,[hMain],SW_SHOWDEFAULT
	invoke	UpdateWindow,[hMain]

  .message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	.shutdown
	invoke	TranslateAcceleratorW,[hMain],[hAccel],addr msg
	test	eax,eax
	jnz	.message_loop
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	.message_loop

  .shutdown:
	invoke	ExitProcess,[msg.wParam]

  .fatal_msgbox:
	invoke	MessageBox,0,rdx,'UAH Menu RichEdit',MB_ICONERROR
	invoke	ExitProcess,1
endp
