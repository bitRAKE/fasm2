; https://learn.microsoft.com/en-us/windows/win32/api/shldisp/nn-shldisp-iautocomplete

interface IAutoComplete,\
	00BB2762-6A77-11D0-A535-00C04FD7D062,\
	QueryInterface,AddRef,Release,\; IUnknown
	Init,Enable

interface IAutoComplete2,\
	EAC04BC0-3791-11d2-BB95-0060977B464C,\
	QueryInterface,AddRef,Release,\; IUnknown
	Init,Enable,\; IAutoComplete
	SetOptions,GetOptions

; enum AUTOCOMPLETEOPTIONS
ACO_NONE		:= 0h
ACO_AUTOSUGGEST		:= 1h
ACO_AUTOAPPEND		:= 2h
ACO_SEARCH		:= 4h
ACO_FILTERPREFIXES	:= 8h
ACO_USETAB		:= 10h
ACO_UPDOWNKEYDROPSLIST	:= 20h
ACO_RTLREADING		:= 40h
ACO_WORD_FILTER		:= 80h
ACO_NOPREFIXFILTERING	:= 100h


