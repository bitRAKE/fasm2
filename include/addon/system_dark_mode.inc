; addon/system_dark_mode.inc - Windows app-mode dark theme detection.

proc WindowsAppsUseDarkMode
    locals
	value dd ?
	size dd ?
    endl
	mov	dword [value],1
	mov	dword [size],4
	invoke	RegGetValue,HKEY_CURRENT_USER,\
		'Software\Microsoft\Windows\CurrentVersion\Themes\Personalize',\
		'AppsUseLightTheme',RRF_RT_REG_DWORD,0,addr value,addr size
	test	eax,eax
	jnz	.light
	cmp	dword [value],0
	je	.dark
  .light:
	xor	eax,eax
	ret
  .dark:
	mov	eax,1
	ret
endp
