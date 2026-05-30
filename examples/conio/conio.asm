; build:
;	fasm2 conio.asm
;	link @conio.response

; console example
;	+ non-blocking event loop
;	+ console event display

include 'console.inc'

; Todo:
;	- mouse wheel scroll direction/count
;	- shift-click selection overlay disable
;
; Notes:
;
; The only thing expecting volatile registers to be intact is the function that called mainCRTStartup! ExitProcess enables the code to own all volatile registers in this scope.
;
; The Main frame is used globally - RBP is kept constant throughout.
;
; An RBP frame is used to pass global variables to other functions.
; Initial stack commit should exceed global frame size.
; Code size can be reduced by putting common variables close to `.old_RBP`.
;
; Reducing the number of WriteFile calls is the best way to improve console
; display speed.

fastcall?.frame = 0 ; fastcall will determine max parameter depth

; Handling control signals in the terminal allows restoring the terminal state
; existing prior to program start. Happy terminal users.

□	hInput		dq ?
□	hQuitEvent	dq ? ; non-inheritable, manual reset, nonsignaled, unnamed event

assert hQuitEvent = 8+hInput ; adjacency required for WaitForMultipleObjects


; Note: This really only effects CTRL_BREAK_EVENT. CTRL_C_EVENT is prevented by
; ReadConsoleInputA and CTRL_CLOSE_EVENT does not need terminal state restored.

ConsoleCtrlHandler: ; ctrlType
;	0	CTRL_C_EVENT
;	1	CTRL_BREAK_EVENT
;	2	CTRL_CLOSE_EVENT
	cmp ecx, 3
	jnc .not_handled

	enter 32, 0
	:kernel32:SetEvent [hQuitEvent] ; BOOL
	; forward result to gate event handling on ablity to set
	leave
	retn

.not_handled:
	xor eax, eax
	retn



public Main as 'mainCRTStartup'
Main:
	virtual at rbp - .local
		.bbuff		rb 256	; line buffer

		.oldInMode	dd ?
		.oldOutMode	dd ?

		.oldInCP	dd ?
		.oldOutCP	dd ?

		.mainBuffInfo	CONSOLE_SCREEN_BUFFER_INFO
		.altBuffInfo	CONSOLE_SCREEN_BUFFER_INFO

		.iBuffer	INPUT_RECORD

	align.assume rbp, 16
	align 16
	.local = $ - $$
		.old_RBP	dq ?	; unused
		.RET		dq ?	; unused

		; parent shadow space
		.hOutput	dq ?

		.columns	dd ?
		.lines		dd ?

		.last_up_vk	dd ?
		.ctlkey_state	dd ?

		.result		dd ?
		.nBuffer	dd ?

		; Do not exceed shadow space size:
		assert $ - .hOutput < 33
	end virtual
	enter .frame + .local, 0
	mov [.result], 1
	mov [.ctlkey_state], -1

	:kernel32:GetStdHandle dword STD_INPUT_HANDLE
	mov [hInput], rax
	::GetStdHandle dword STD_OUTPUT_HANDLE
	mov [.hOutput], rax

	::GetConsoleCP
	mov [.oldInCP], eax
	::GetConsoleOutputCP
	mov [.oldOutCP], eax

	::SetConsoleCP CP_UTF8
	::SetConsoleOutputCP CP_UTF8

; Note: `GetFileType(?) != FILE_TYPE_CHAR` can also detect redirected handles

	::GetConsoleMode [hInput], addr .oldInMode
	test eax, eax ; BOOL
	jz .fatal_bad_handle ; don't support redirected handles

	::GetConsoleMode [.hOutput], addr .oldOutMode
	test eax, eax ; BOOL
	jz .fatal_bad_handle ; don't support redirected handles

	; reduce input processing, get more messages
	::SetConsoleMode [hInput], ENABLE_EXTENDED_FLAGS\
		or ENABLE_MOUSE_INPUT or ENABLE_WINDOW_INPUT

; from Windows 10 version 1511 (November 2015 update) and Windows Server 2016

	; no wrapping nor scrolling, maximum control characters
	::SetConsoleMode [.hOutput], ENABLE_PROCESSED_OUTPUT\
		or ENABLE_VIRTUAL_TERMINAL_PROCESSING

; The cursor position should be captured from the emulation with: <27,'[6n'>;
; and then parse response from the input stream: <27,'[{row};{col}R'>.
	::GetConsoleScreenBufferInfo [.hOutput], addr .mainBuffInfo

	; non-inheritable, manual reset, nonsignaled, unnamed event
	:kernel32:CreateEventW 0, TRUE, FALSE, 0
	mov [hQuitEvent], rax

	:kernel32:SetConsoleCtrlHandler addr ConsoleCtrlHandler, TRUE


; Note: Much functionality is not recommended for new console applications.
; https://learn.microsoft.com/en-us/windows/console/ecosystem-roadmap
; In some cases there are replacement methods:
;	::SetConsoleTitleA <'Input Debug Panel',0>
	<< 27,"]0;","Input Debug Panel",7 >>

	;  new alternate screen buffer
	<< 27,'[?1049h' >>
	jmp .main_loop

.main_ctrlkey_update_and_bbuff:
	; ECX : new dwControlKeyState value
	call dwControlKeyState__Update

.main_output: ; Clear string build-up buffer ...
	lea rdx, [.bbuff]
	mov r8, rdi
	sub r8, rdx
	::WriteFile [.hOutput], rdx, r8, 0, 0

.main_loop: ; Wait for quit signal or input event:
	::WaitForMultipleObjects 2, addr hInput, FALSE, -1 ; INFINITE milliseconds
	cmp eax, WAIT_OBJECT_0+1
	jz .finish
	cmp eax, WAIT_OBJECT_0
	jnz .fail_GetLastError ; WAIT_FAILED?

	lea rdi, [.bbuff] ; String build-up buffer.

	::ReadConsoleInputA [hInput], addr .iBuffer, 1, addr .nBuffer
	test eax, eax ; BOOL
	jz .read_error
	cmp [.nBuffer], 1
	jz .process_record
.read_error:
	<| 27,'[91m','Error',27,'[m',': unknown read.' |>
	jmp .main_output
.process_record:

	; parse structure and display

	iterate EVT, KEY_EVENT,MOUSE_EVENT,WINDOW_BUFFER_SIZE_EVENT,MENU_EVENT,FOCUS_EVENT
		cmp [.iBuffer.EventType], EVT
		jz .process_#EVT
	end iterate

	; TODO: dump unknown event?
	<| 27,'[91m','Error',27,'[m',': unknown event.' |>
	jmp .main_output



.process_KEY_EVENT:
	<| 27,'[95m','KEY_EVENT',27,'[m',': ' |>
	test [.iBuffer.KeyEvent.bKeyDown], -1
	jnz .key_down
	<| '<',27,'[31m','up',27,'[m','>' |>
	jmp .key_upped
.key_down:
	<| '<',27,'[32m','down',27,'[m','>' |>
.key_upped:
	cmp [.iBuffer.KeyEvent.wRepeatCount], 1
	jz .key_one
	<| ', wRepeatCount = 0x' |>
	movzx eax, [.iBuffer.KeyEvent.wRepeatCount]
	mov ecx, 16
	call u64__ToBaseForm
.key_one:
	<| ', wVirtualKeyCode = 0x' |>
	movzx eax, [.iBuffer.KeyEvent.wVirtualKeyCode]
	mov ecx, 16
	call u64__ToBaseForm

	<| ', wVirtualScanCode = 0x' |>
	movzx eax, [.iBuffer.KeyEvent.wVirtualScanCode]
	mov ecx, 16
	call u64__ToBaseForm

	test [.iBuffer.KeyEvent.UnicodeChar], -1 ; control character?
	jz @F
	<| ', UnicodeChar = 0x' |>
	movzx eax, [.iBuffer.KeyEvent.UnicodeChar]
	mov ecx, 16
	call u64__ToBaseForm
@@:
	; track key up events for exit on double ESC

	test [.iBuffer.KeyEvent.bKeyDown], -1
	jnz .finish_KEY_EVENT
	movzx eax, [.iBuffer.KeyEvent.wVirtualKeyCode]
	cmp [.last_up_vk], 27
	jnz @F
	cmp eax, 27 ; VK_ESC
	jz .finish
@@:	mov [.last_up_vk], eax

.finish_KEY_EVENT:
	<| 27,'[m','.',10 |>
	mov ecx, [.iBuffer.KeyEvent.dwControlKeyState]
	jmp .main_ctrlkey_update_and_bbuff



.process_MOUSE_EVENT:
	; Thin event stream by consuming MOUSE_MOVED:
	test [.iBuffer.MouseEvent.dwEventFlags], MOUSE_MOVED
	jz .not_MOUSE_MOVED

	<|	27,'[?25l',27,'7',	\; hide & save position of cursor
		27,'[;999H',		\; first line, right edge
		27,'[6D'		\; (CUB) coordinate space, 7 chars
	|>
	movsx rax, [.iBuffer.MouseEvent.dwMousePosition.X]
	call s64__ToString
	mov al, ','
	stosb
	movsx rax, [.iBuffer.MouseEvent.dwMousePosition.Y]
	call s64__ToString

	<|	27,'[K',		\; erase caret to end of line
		27,'8',27,'[?25h'	\; restore position & show cursor
	|>
; Note: not all keys supported! So, we grab some bits from the global state.
	mov ecx, not 0x1F
	and ecx, [.ctlkey_state]
	or ecx, [.iBuffer.MouseEvent.dwControlKeyState]
	jmp .main_ctrlkey_update_and_bbuff


; Since position updates on move, just show button and event flags.
.not_MOUSE_MOVED:
	<| 27,'[34m','MOUSE_EVENT',27,'[m',': ' |>
	; TODO: wheel change in .MouseEvent.dwButtonState >> 4, <undocumented>
	iterate EVENT, DOUBLE_CLICK,MOUSE_WHEELED,MOUSE_HWHEELED
		test [.iBuffer.MouseEvent.dwEventFlags], EVENT
		jz .%
		<| `EVENT |>
		jmp @F
	.%:
	end iterate

	test [.iBuffer.MouseEvent.dwButtonState], -1
	jnz .active_mouse_button
	<| '<',27,'[31m','no',27,'[m','>, buttons' |> ; what button?
	jmp .write_buildup_buffer_period

.active_mouse_button:
	<| '<',27,'[32m','active',27,'[m','>' |> ; default event
@@:
	test [.iBuffer.MouseEvent.dwButtonState], 0xF
	jz .write_buildup_buffer_period

	<| ', buttons = ' |>
	iterate <SHOW,	BUTTON>,\
		'1',	FROM_LEFT_1ST_BUTTON_PRESSED,\
		'2',	FROM_LEFT_2ND_BUTTON_PRESSED,\
		'3',	FROM_LEFT_3RD_BUTTON_PRESSED,\
		'4',	FROM_LEFT_4TH_BUTTON_PRESSED,\
		'R',	RIGHTMOST_BUTTON_PRESSED
		test [.iBuffer.MouseEvent.dwButtonState], BUTTON
		jz .but.%
		mov al, SHOW
		stosb
	.but.%:
	end iterate
	; TODO: display undocumented bits
	jmp .write_buildup_buffer_period



.process_WINDOW_BUFFER_SIZE_EVENT:
	<|	27,'[!p',\	; soft reset
		27,'[2J',\	; clear viewport to space characters
		27,'[3;f',\	; cursor position
		27,'[91m',9,'Press ESC twice to exit.',\
		27,'[4;r',\	; scroll margins
		27,'[4;H',\	; cursor position
		27,'[33m','WINDOW_BUFFER_SIZE_EVENT',27,'[m',': dwSize { '\
	|>
	movzx eax, [.iBuffer.WindowBufferSizeEvent.dwSize.X]
	mov [.columns], eax
	call s64__ToString
	mov ax, ', '
	stosw
	movzx eax, [.iBuffer.WindowBufferSizeEvent.dwSize.Y]
	mov [.lines], eax
	call s64__ToString
	mov eax, ' }.' or (10 shl 24)
	stosd

	lea rdx, [.bbuff]
	mov r8, rdi
	sub r8, rdx
	::WriteFile [.hOutput], rdx, r8, 0, 0

	mov ecx, [.ctlkey_state]
	not [.ctlkey_state] ; update all
	call dwControlKeyState__Update
	jmp .main_loop



.process_MENU_EVENT:
	<| 27,'[33m','MENU_EVENT',27,'[m',': dwCommandId = ',27,'[32m' |>
	movsxd rax, [.iBuffer.MenuEvent.dwCommandId]
	call s64__ToString
	jmp .write_buildup_buffer_period



.process_FOCUS_EVENT:
	<| 27,'[36m','FOCUS_EVENT',27,'[m',': ' |>
	test [.iBuffer.FocusEvent.bSetFocus], -1
	jz .focus_inactive
	<| 27,'[32m','active' |>
	jmp .write_buildup_buffer_period
.focus_inactive:
	<| 27,'[31m','inactive' |>

.write_buildup_buffer_period:
	<| 27,'[m','.',10 |>
	jmp .main_output



.finish:;-----------------------------------------------------------------------
	mov [.result], 0
.fatal:
	<< 27,'[?1049l' >> ;  main screen buffer
	lea rsi, [.mainBuffInfo.dwCursorPosition]
	call ANSI__H_SetCursorPosition

; Note: PowerShell requires linefeed to update the prompt.
	<< 10 >>

; be responsible terminal bros, restore initial console state prior to exit
	::SetConsoleMode [.hOutput], dword [.oldOutMode]
	::SetConsoleMode [hInput], dword [.oldInMode]

	::SetConsoleOutputCP [.oldOutCP]
	::SetConsoleCP [.oldInCP]

.fatal_bad_handle:
	::ExitProcess [.result]
	.frame := fastcall?.frame
; TODO: (doesn't happen)
.fail_GetLastError:
	int3



;	SHFT   CAPS   ENH    SCROLL
;	CTL   ALT   NUM   ALT   CTL
; ECX : new dwControlKeyState value
; Note: doesn't use RSI/RDI on purpose
dwControlKeyState__Update: fastcall?.frame = 0
	.hOutput equ Main.hOutput
	.state equ Main.ctlkey_state

	mov ebx, [.state]	; old state
	mov [.state], ecx	; preserve new state
	xor ebx, ecx		; changed bit flags, non-volatile
	and ebx, 0x1FF		; filter out NLS_* bits
	jnz @F
	retn

@@:	sub rsp, .frame
	<< 27,'[?25l',27,'7' >> ; hide & save position of cursor

	bsf ecx, ebx		; known non-zero
.more:	btr ebx, ecx		; clear delta flag
	bt [.state], ecx	; present state to show
	adc ecx, ecx
	mov edx, [.table+rcx*8]
	mov r8d, [.table+rcx*8+4]
	::WriteFile [.hOutput], rdx, r8, 0, 0
	bsf ecx, ebx
	jnz .more

	<< 27,'8',27,'[?25h' >> ; restore position & show cursor
	add rsp, .frame
	retn
	.frame := 8 + fastcall?.frame

iterate <C_X,	C_Y,	KEY>,\
	19,	2,	'ALT',\		; RIGHT_ALT_PRESSED
	7,	2,	'ALT',\		; LEFT_ALT_PRESSED
	25,	2,	'CTL',\		; RIGHT_CTRL_PRESSED
	1,	2,	'CTL',\		; LEFT_CTRL_PRESSED
	1,	1,	'SHFT',\	; SHIFT_PRESSED
	13,	2,	'NUM',\		; NUMLOCK_ON
	22,	1,	'SCROLL',\	; SCROLLLOCK_ON
	8,	1,	'CAPS',\	; CAPSLOCK_ON
	15,	1,	'ENH'		; ENHANCED_KEY

	if % = 1
	label .table:4
		repeat %%
			dd .1.%
			dd .1.%.bytes
			dd .2.%
			dd .2.%.bytes
		end repeat
	end if
	match STATE,%
	iterate COLOR,\; strings to support complex ANSI, '38;5;8'
		'90;2',\	; inactive faint grey
		'0;92'		; active green
	.%.STATE:
		db 27,'['
		if C_Y <> 1
			db `C_Y
		end if
		db ';'
		if C_X <> 1
			db `C_X
		end if
		db 'f',27,'[',COLOR,'m',KEY
		.%.STATE.bytes := $ - .%.STATE
	end iterate
	end match
end iterate


;------------------------------------------------------- ANSI support functions:
; instead of:	SetConsoleCursorPosition
; RSI : COORD for new cursor position in 1-based, character units
;	Note: Outside of range [2,32767] clamped to 1!
; interface: Win64ABI + RDI
ANSI__H_SetCursorPosition:
	lea rdi, [Main.bbuff+2]
	mov word [rdi-2], 27 or ('[' shl 8)

	movsx eax, [rsi + COORD.Y]
	cmp eax, 1
	jle @F
	call s64__ToString
@@:
	mov al, ';'
	stosb

	movsx eax, [rsi + COORD.X]
	cmp eax, 1
	jle @F
	call s64__ToString
@@:
	mov al, 'H'
	stosb

	sub rsp, 8*5
	lea rdx, [Main.bbuff]
	mov r8, rdi
	sub r8, rdx
	::WriteFile [Main.hOutput], rdx, r8, 0, 0
	add rsp, 8*5
	retn


;--------------------------------------------------------------- leaf functions:
s64__ToString:
	push 10
	pop rcx

	test rax, rax
	jns u64__ToBaseForm
	neg rax
	mov byte [rdi], '-'
	scasb

u64__ToBaseForm:
; RCX : number base [2,85]
; RAX : unsigned number to convert
; RDI : string buffer to receive digits
	push -1
@@:	xor rdx, rdx
	div rcx
	push rdx
	test rax, rax
	jnz @B

	pop rdx
@@:	mov al, [.table + rdx]
	stosb
	pop rdx
	test edx, edx
	jns @B
	retn

.table db \
	'0123456789ABCDEF',\	; hexadecimal
	'GHIJKLMNOPQRSTUV',\	; Base32, RFC 4648
	'WXYZabcdefghijkl',\
	'mnopqrstuvwxyz!#',\	; base64, unconventional
	'$%&()*+-;<=>?@^_',\
	'`{|}~'			; Base85, RFC 1924


; Additional Notes:
;	- configured Termial Actions are intercepted:
;	https://learn.microsoft.com/en-us/windows/terminal/customize-settings/actions
;
; Inconsistencies:
;	- FOCUS_EVENT can be received AFTER event causing console to gain focus!
;	- mouse wheel change is undocumented, [.MouseEvent.dwButtonState] >> 4
;	- [.MouseEvent.dwControlKeyState] is documented incorrectly
;		- partial flags compared to: [.KeyEvent.dwControlKeyState]
;	- PowerShell requires a linefeed to update the prompt.
;
; REFERENCES:
; https://en.wikipedia.org/wiki/ANSI_escape_code
; https://learn.microsoft.com/en-us/windows/console/console-functions
; https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
