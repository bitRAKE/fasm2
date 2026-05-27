; taskdialog.inc - small helpers for packed TASKDIALOGCONFIG samples.

S_OK	= 0
S_FALSE = 1

PBST_NORMAL = 1
PBST_ERROR  = 2
PBST_PAUSED = 3

TDN_COUNT = TDN_EXPANDO_BUTTON_CLICKED + 1

struct TASKDIALOG_CONTEXT
	cfg TASKDIALOGCONFIG
	msgOnCreate		dq ?
	msgOnNavigate		dq ?
	msgOnButton		dq ?
	msgOnHyperlink		dq ?
	msgOnTimer		dq ?
	msgOnDestroy		dq ?
	msgOnRadio		dq ?
	msgOnConstructed	dq ?
	msgOnVerification	dq ?
	msgOnHelp		dq ?
	msgOnExpand		dq ?
	_tailPadding		dq ?
ends

assert sizeof.TASKDIALOG_CONTEXT mod 8 = 0

macro td_set_ptr field*, label*
	lea	rax,[label]
	mov	[field],rax
end macro

macro td_set_handler field*, label*
	lea	rax,[label]
	mov	[field],rax
end macro

macro td_local source*, alias*, base*
	label alias:(sizeof source) at base + source - td.msgOnCreate
end macro

macro TD_BUTTON_ARRAY name*, entries&
	local total
	total = 0
	align 4
	name:
	iterate <id,text>, entries
		dd id
		dq text
		total = total + 1
	end iterate
	name.count = total
end macro

define __GLOBAL_BSS__ taskdialog_bss

macro taskdialog_bss
	align 8
	hInstance	dq ?
	hOwner		dq ?
purge taskdialog_bss
end macro

proc TaskDialogInit uses rdi, ctx
	mov	rdi,rcx
	mov	rdx,rcx
	xor	eax,eax
	mov	ecx,sizeof.TASKDIALOG_CONTEXT shr 3
	rep	stosq

	mov	dword [rdx + TASKDIALOG_CONTEXT.cfg.cbSize],sizeof.TASKDIALOGCONFIG
	mov	rax,[hInstance]
	mov	[rdx + TASKDIALOG_CONTEXT.cfg.hInstance],rax
	mov	rax,[hOwner]
	mov	[rdx + TASKDIALOG_CONTEXT.cfg.hwndParent],rax
	mov	dword [rdx + TASKDIALOG_CONTEXT.cfg.dwFlags],\
		TDF_POSITION_RELATIVE_TO_WINDOW or TDF_ALLOW_DIALOG_CANCELLATION
	lea	rax,[TaskDialogCallback]
	mov	[rdx + TASKDIALOG_CONTEXT.cfg.pfCallback],rax
	lea	rax,[rdx + TASKDIALOG_CONTEXT.msgOnCreate]
	mov	[rdx + TASKDIALOG_CONTEXT.cfg.lpCallbackData],rax

	lea	rdi,[rdx + TASKDIALOG_CONTEXT.msgOnCreate]
	lea	rax,[TaskDialogCallback.S_OK]
	mov	ecx,TDN_COUNT
	rep	stosq
	ret
endp

align 16
TaskDialogCallback:
	; Handlers may call WinAPI before touching local aliases again, so keep the
	; callback-data base in nonvolatile rbx for the whole handler call.
	virtual at rsp
			rq 4	; handler home space, reserved by this callback
		.savedRbx	dq ?
			dq ?	; return address
			rq 4	; register-argument home space from comctl32
		.lpRefData	dq ?
	end virtual
	push	rbx
	sub	rsp,20h
	mov	rbx,[.lpRefData]
	mov	rax,rbx
	cmp	edx,TDN_COUNT
	jae	.default
	call	qword [rbx+rdx*8]
  .return:
	add	rsp,20h
	pop	rbx
	retn

  .default:
	xor	eax,eax
	jmp	.return

.S_OK:
	xor	eax,eax
	retn
