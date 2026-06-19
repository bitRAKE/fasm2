proc ConfigureRichEdit
	; EM_SETTEXTMODE is only accepted while the control has no text.
	mov	eax,TM_RICHTEXT or TM_MULTILEVELUNDO or TM_MULTICODEPAGE
	invoke	SendMessage,[hRich],EM_SETTEXTMODE,eax,0

	mov	eax,TO_ADVANCEDTYPOGRAPHY or TO_ADVANCEDLAYOUT
	invoke	SendMessage,[hRich],EM_SETTYPOGRAPHYOPTIONS,eax,eax

	mov	eax,IMF_AUTOKEYBOARD or IMF_AUTOFONT or IMF_DUALFONT or\
		IMF_NORTFFONTSUBSTITUTE or IMF_SPELLCHECKING or\
		IMF_TKBPREDICTION or IMF_IMEUIINTEGRATION
	invoke	SendMessage,[hRich],EM_SETLANGOPTIONS,0,eax

	mov	eax,SES_EXTENDBACKCOLOR or SES_USECTF or\
		SES_CTFALLOWEMBED or SES_CTFALLOWSMARTTAG or SES_CTFALLOWPROOFING or\
		SES_HYPERLINKTOOLTIPS or SES_NOFOCUSLINKNOTIFY or\
		SES_WORDDRAGDROP or SES_SMARTDRAGDROP
	invoke	SendMessage,[hRich],EM_SETEDITSTYLE,eax,eax
	invoke	SendMessage,[hRich],EM_SETEDITSTYLEEX,\
		SES_EX_HANDLEFRIENDLYURL,SES_EX_HANDLEFRIENDLYURL

	mov	eax,ECO_AUTOWORDSELECTION or ECO_NOHIDESEL or ECO_SAVESEL or ECO_SELECTIONBAR
	invoke	SendMessage,[hRich],EM_SETOPTIONS,ECOOP_OR,eax

	invoke	SendMessage,[hRich],EM_SETUNDOLIMIT,512,0

	invoke	SendMessage,[hRich],EM_AUTOURLDETECT,AURL_ENABLEURL,\
		"callto:file:ftp:gopher:http:https:mailto:news:notes:nntp:onenote:outlook:prospero:tel:telnet:wais:webcal:"

	mov	eax,ENM_CHANGE or ENM_UPDATE or ENM_SCROLL or ENM_SCROLLEVENTS or\
		ENM_DRAGDROPDONE or ENM_KEYEVENTS or ENM_MOUSEEVENTS or\
		ENM_REQUESTRESIZE or ENM_SELCHANGE or ENM_DROPFILES or ENM_LINK
	invoke	SendMessage,[hRich],EM_SETEVENTMASK,0,eax

	ret
endp

proc ApplyRichTheme
    locals
	cf CHARFORMATW
	color dd ?
    endl
	cmp	qword [hRich],0
	je	.done
	invoke	RtlZeroMemory,addr cf,sizeof.CHARFORMATW
	fastcall ThemeText
	mov	[cf.cbSize],sizeof.CHARFORMATW
	mov	[cf.dwMask],CFM_COLOR
	mov	[cf.crTextColor],eax
	invoke	SendMessage,[hRich],EM_SETCHARFORMAT,SCF_ALL,addr cf
	fastcall ThemeBg
	invoke	SendMessage,[hRich],EM_SETBKGNDCOLOR,0,eax
	invoke	InvalidateRect,[hRich],0,1
  .done:
	ret
endp

proc ResizeRich
	cmp	qword [hRich],0
	je	.done
	invoke	GetClientRect,[hMain],addr client_rect
	invoke	MoveWindow,[hRich],0,0,\
		dword [client_rect.right],dword [client_rect.bottom],1
  .done:
	ret
endp

proc CreateRichEdit hwnd
	mov	[hwnd],rcx
	invoke	LoadLibrary,'msftedit.dll'
	mov	[hMsftEdit],rax
	test	rax,rax
	jz	.fail

	invoke	CreateWindowEx,WS_EX_CLIENTEDGE,'RICHEDIT50W',0,\
		WS_CHILD or WS_VISIBLE or WS_VSCROLL or WS_HSCROLL or WS_TABSTOP or\
		WS_CLIPCHILDREN or WS_CLIPSIBLINGS or\
		ES_MULTILINE or ES_AUTOVSCROLL or ES_AUTOHSCROLL or\
		ES_NOHIDESEL or ES_WANTRETURN,\
		0,0,0,0,[hwnd],ID_RICHEDIT,[hInstance],0
	mov	[hRich],rax
	test	rax,rax
	jz	.fail
	fastcall ConfigureRichEdit
	fastcall ApplyRichTheme
	fastcall ResizeRich
	fastcall RichPasteProofingSample
	mov	eax,1
	ret

  .fail:
	xor	eax,eax
	ret
endp

proc CleanupRichEdit
	mov	qword [hRich],0
	mov	rcx,[hMsftEdit]
	jrcxz	.done
	invoke	FreeLibrary,rcx
	mov	qword [hMsftEdit],0
  .done:
	ret
endp

proc RichInsertText textp
    locals
	length dd ?
    endl
	mov	[textp],rcx
	cmp	qword [hRich],0
	je	.done
	invoke	GetWindowTextLength,[hRich]
	mov	[length],eax
	invoke	SendMessage,[hRich],EM_SETSEL,dword [length],dword [length]
	invoke	SendMessage,[hRich],EM_REPLACESEL,1,[textp]
  .done:
	ret
endp

proc RichPasteProofingSample
	fastcall RichInsertText,<\
		'Thiss proofing sampple is intensionally wrtten with severl speling errrors.',13,10,\
		'The recieve queue shoud notice mispelled words, awkward capitilization, and a fwe typoes.',13,10,\
		13,10,\
		'Right-click a misspeled word to exercise the RichEdit proofing popup. The same UAH painter themes that native menu.',13,10,\
		13,10,\
		'URL detection is also active: https://flatassembler.net/ and mail@example.com',13,10\
	>
	ret
endp

proc RichInsertSample
	fastcall RichInsertText,<13,10,\
		'Anothr sentance was inserted thru the resource-backed menu with delibrate proofing mistakes.',13,10>
	ret
endp

proc RichInsertLink
	fastcall RichInsertText,<13,10,'https://flatassembler.net/',13,10>
	ret
endp

proc RichClear
	cmp	qword [hRich],0
	je	.done
	invoke	SetWindowTextW,[hRich],''
	fastcall ApplyRichTheme
  .done:
	ret
endp

proc RichExecuteEditCommand command
	mov	dword [command],ecx
	cmp	qword [hRich],0
	je	.done
	cmp	ecx,ID_EDIT_UNDO
	je	.undo
	cmp	ecx,ID_EDIT_REDO
	je	.redo
	cmp	ecx,ID_EDIT_CUT
	je	.cut
	cmp	ecx,ID_EDIT_COPY
	je	.copy
	cmp	ecx,ID_EDIT_PASTE
	je	.paste
	cmp	ecx,ID_EDIT_DELETE
	je	.delete
	cmp	ecx,ID_EDIT_SELECT_ALL
	je	.select_all
	cmp	ecx,ID_EDIT_INSERT_LINK
	je	.insert_link
	cmp	ecx,ID_EDIT_CLEAR
	je	.clear
	jmp	.done
  .undo:
	invoke	SendMessage,[hRich],EM_UNDO,0,0
	jmp	.done
  .redo:
	invoke	SendMessage,[hRich],EM_REDO,0,0
	jmp	.done
  .cut:
	invoke	SendMessage,[hRich],WM_CUT,0,0
	jmp	.done
  .copy:
	invoke	SendMessage,[hRich],WM_COPY,0,0
	jmp	.done
  .paste:
	invoke	SendMessage,[hRich],WM_PASTE,0,0
	jmp	.done
  .delete:
	invoke	SendMessage,[hRich],WM_CLEAR,0,0
	jmp	.done
  .select_all:
	invoke	SendMessage,[hRich],EM_SETSEL,0,-1
	jmp	.done
  .insert_link:
	fastcall RichInsertLink
	jmp	.done
  .clear:
	fastcall RichClear
  .done:
	ret
endp
