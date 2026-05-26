; PSDK COM interface inquiry: IFileOpenDialog
; SDK 10.0.26100.0 at C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0
; source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\ShObjIdl_core.idl

; Method signatures from IDL
; IModalWindow : IUnknown
;   HRESULT Show(HWND hwndOwner)
;   HRESULT RemoteShow(HWND hwndOwner)
; IFileDialog : IModalWindow
;   HRESULT SetFileTypes(UINT cFileTypes, const COMDLG_FILTERSPEC *rgFilterSpec)
;   HRESULT SetFileTypeIndex(UINT iFileType)
;   HRESULT GetFileTypeIndex(UINT *piFileType)
;   HRESULT Advise(IFileDialogEvents *pfde, DWORD *pdwCookie)
;   HRESULT Unadvise(DWORD dwCookie)
;   HRESULT SetOptions(FILEOPENDIALOGOPTIONS fos)
;   HRESULT GetOptions(FILEOPENDIALOGOPTIONS *pfos)
;   HRESULT SetDefaultFolder(IShellItem *psi)
;   HRESULT SetFolder(IShellItem *psi)
;   HRESULT GetFolder(IShellItem **ppsi)
;   HRESULT GetCurrentSelection(IShellItem **ppsi)
;   HRESULT SetFileName(LPCWSTR pszName)
;   HRESULT GetFileName(LPWSTR *pszName)
;   HRESULT SetTitle(LPCWSTR pszTitle)
;   HRESULT SetOkButtonLabel(LPCWSTR pszText)
;   HRESULT SetFileNameLabel(LPCWSTR pszLabel)
;   HRESULT GetResult(IShellItem **ppsi)
;   HRESULT AddPlace(IShellItem *psi, FDAP fdap)
;   HRESULT SetDefaultExtension(LPCWSTR pszDefaultExtension)
;   HRESULT Close(HRESULT hr)
;   HRESULT SetClientGuid(REFGUID guid)
;   HRESULT ClearClientData(void)
;   HRESULT SetFilter(IShellItemFilter *pFilter)
; IFileOpenDialog : IFileDialog
;   HRESULT GetResults(IShellItemArray **ppenum)
;   HRESULT GetSelectedItems(IShellItemArray **ppsai)

; interface IModalWindow : IUnknown
;   IUnknown: QueryInterface, AddRef, Release
;   IModalWindow: Show, RemoteShow
IID_IModalWindow db 57h, 16h, DBh, B4h, D7h, 70h, 5Eh, 48h, 8Eh, 3Eh, 6Fh, CBh, 5Ah, 5Ch, 18h, 02h
interface IModalWindow,\
	QueryInterface,\
	AddRef,\
	Release,\
	Show,\
	RemoteShow

; interface IFileDialog : IModalWindow
;   IUnknown: QueryInterface, AddRef, Release
;   IModalWindow: Show, RemoteShow
;   IFileDialog: SetFileTypes, SetFileTypeIndex, GetFileTypeIndex, Advise, Unadvise, SetOptions, GetOptions, SetDefaultFolder, SetFolder, GetFolder, GetCurrentSelection, SetFileName, GetFileName, SetTitle, SetOkButtonLabel, SetFileNameLabel, GetResult, AddPlace, SetDefaultExtension, Close, SetClientGuid, ClearClientData, SetFilter
IID_IFileDialog db 36h, 51h, F8h, 42h, 7Eh, DBh, 9Ch, 43h, 85h, F1h, E4h, 07h, 5Dh, 13h, 5Fh, C8h
interface IFileDialog,\
	QueryInterface,\
	AddRef,\
	Release,\
	Show,\
	RemoteShow,\
	SetFileTypes,\
	SetFileTypeIndex,\
	GetFileTypeIndex,\
	Advise,\
	Unadvise,\
	SetOptions,\
	GetOptions,\
	SetDefaultFolder,\
	SetFolder,\
	GetFolder,\
	GetCurrentSelection,\
	SetFileName,\
	GetFileName,\
	SetTitle,\
	SetOkButtonLabel,\
	SetFileNameLabel,\
	GetResult,\
	AddPlace,\
	SetDefaultExtension,\
	Close,\
	SetClientGuid,\
	ClearClientData,\
	SetFilter

; interface IFileOpenDialog : IFileDialog
;   IUnknown: QueryInterface, AddRef, Release
;   IModalWindow: Show, RemoteShow
;   IFileDialog: SetFileTypes, SetFileTypeIndex, GetFileTypeIndex, Advise, Unadvise, SetOptions, GetOptions, SetDefaultFolder, SetFolder, GetFolder, GetCurrentSelection, SetFileName, GetFileName, SetTitle, SetOkButtonLabel, SetFileNameLabel, GetResult, AddPlace, SetDefaultExtension, Close, SetClientGuid, ClearClientData, SetFilter
;   IFileOpenDialog: GetResults, GetSelectedItems
IID_IFileOpenDialog db 88h, 72h, 7Ch, D5h, ADh, D4h, 68h, 47h, BEh, 02h, 9Dh, 96h, 95h, 32h, D9h, 60h
interface IFileOpenDialog,\
	QueryInterface,\
	AddRef,\
	Release,\
	Show,\
	RemoteShow,\
	SetFileTypes,\
	SetFileTypeIndex,\
	GetFileTypeIndex,\
	Advise,\
	Unadvise,\
	SetOptions,\
	GetOptions,\
	SetDefaultFolder,\
	SetFolder,\
	GetFolder,\
	GetCurrentSelection,\
	SetFileName,\
	GetFileName,\
	SetTitle,\
	SetOkButtonLabel,\
	SetFileNameLabel,\
	GetResult,\
	AddPlace,\
	SetDefaultExtension,\
	Close,\
	SetClientGuid,\
	ClearClientData,\
	SetFilter,\
	GetResults,\
	GetSelectedItems
