define __GLOBAL_DATA__ rich_edit_data

macro rich_edit_data
	; High Levenshtein-candidate-density seed for RichEdit proofing behavior.
	proofing_sample_text du 'Yesturday our teh comittee reccomended that we definately seperate ',\
		'the goverment enviroment from the privite sector, wich occured becuase ',\
		'noone beleived it was neccessary to acheive a wierd concensus. I recieved ',\
		'thier embarassing memo on the occassion of the aniversary, and it was truely ',\
		'unforgetable — a maintenence nightmare no buisness could of forseen. The ',\
		'supercalifragilisticexpialidoshus paperwork, riddled with ',\
		'pneumonoultramicroscopicsilicovolcanoconiosiss diagnoses and ',\
		'antidisestablishmentarianisim slogans, left every proffesional flabbergasted ',\
	'by its sheer floccinaucinihilipilificaton.',13,10,0
purge rich_edit_data
end macro

proc ConfigureRichEdit
	; EM_SETTEXTMODE is only accepted while the control has no text.
	mov	eax,TM_RICHTEXT or TM_MULTILEVELUNDO or TM_MULTICODEPAGE
	invoke	SendMessage,[hRich],EM_SETTEXTMODE,eax,0

	mov	eax,TO_ADVANCEDTYPOGRAPHY or TO_ADVANCEDLAYOUT
	invoke	SendMessage,[hRich],EM_SETTYPOGRAPHYOPTIONS,eax,eax

	fastcall RichEnableProofing

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

proc RichEnableProofing
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_GETLANGOPTIONS,0,0
	or	eax,RICH_LANG_OPTIONS
	invoke	SendMessage,[hRich],EM_SETLANGOPTIONS,0,eax
  .done:
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

	invoke	CreateWindowEx,WS_EX_ACCEPTFILES,'RICHEDIT50W',0,\
		RICHEDIT_STYLE,\
		0,0,0,0,[hwnd],ID_RICHEDIT,[hInstance],0
	mov	[hRich],rax
	test	rax,rax
	jz	.fail
	fastcall ConfigureRichEdit
	fastcall ApplyRichTheme
	fastcall ResizeRich
	fastcall RichInsertSample
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

proc RichAppendText textp
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

proc RichAppendProofingSample
	fastcall RichAppendText,addr proofing_sample_text
	ret
endp

proc RichInsertSample
	cmp	qword [hRich],0
	je	.done
	invoke	GetWindowTextLength,[hRich]
	test	eax,eax
	jz	.sample
	fastcall RichAppendText,<13,10,13,10>
  .sample:
	fastcall RichAppendProofingSample
  .done:
	ret
endp

proc RichInsertLink
	fastcall RichAppendText,<13,10,'https://flatassembler.net/',13,10>
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
	cmp [hRich],0
	jz .done
	iterate <function,	command_id>,\
		.undo,		ID_EDIT_UNDO,\
		.redo,		ID_EDIT_REDO,\
		.cut,		ID_EDIT_CUT,\
		.copy,		ID_EDIT_COPY,\
		.paste,		ID_EDIT_PASTE,\
		.delete,	ID_EDIT_DELETE,\
		.select_all,	ID_EDIT_SELECT_ALL,\
		.insert_link,	ID_EDIT_INSERT_LINK,\
		.clear,		ID_EDIT_CLEAR

		cmp ecx, command_id
		jz function
	end iterate
  .done:
	ret
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
	jmp	.done
endp
