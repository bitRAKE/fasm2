
; ICoreWebView2_3, Core Objects:
;	CompositionController2;
;	Controller3;
;	Cookie;
;	CookieList;
;	CookieManager;
;	Deferral;
;	DevToolsProtocolEventReceiver;
;	Environment4;
;	EnvironmentOptions;
;	FrameInfo;
;	FrameInfoCollection;
;	FrameInfoCollectionIterator;
;	HttpHeadersCollectionIterator;
;	HttpRequestHeaders;
;	HttpResponseHeaders;
;	Interop;
;	PointerInfo;
;	Settings3;
;	WebResourceRequest;
;	WebResourceResponse;
;	WebResourceResponseView;
;	WindowFeatures;

; Syncronous Event Handlers:			(Invoke with args?)
;	AcceleratorKeyPressed				Args
;	ContentLoading					Args
;	ContainsFullScreenElementChanged
;	CursorChanged
;	DocumentTitleChanged
;	DOMContentLoaded				Args
;	DevToolsProtocolEventReceived			Args
;	FocusChanged
;	HistoryChanged
;	MoveFocusRequested				Args
;	NavigationCompleted				Args
;	NavigationStarting				Args
;	NewBrowserVersionAvailable
;	NewWindowRequested				Args
;	PermissionRequested				Args
;	ProcessFailed					Args2
;	RasterizationScaleChanged
;	ScriptDialogOpening				Args
;	SourceChanged					Args
;	TrySuspendCompleted
;	WebMessageReceived				Args
;	WebResourceRequested				Args
;	WebResourceResponseReceived			Args
;	WindowCloseRequested
;	ZoomFactorChanged

; Asynchronous Completion Handlers:
;	AddScriptToExecuteOnDocumentCreated
;	CallDevToolsProtocolMethod
;	CapturePreview
;	CreateCoreWebView2CompositionController
;	CreateCoreWebView2Controller
;	CreateCoreWebView2Environment
;	ExecuteScript
;	GetCookies
;	WebResourceResponseViewGetContent


; enum COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT
COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG:=0
COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_JPEG:=1

; enum COREWEBVIEW2_COOKIE_SAME_SITE_KIND
COREWEBVIEW2_COOKIE_SAME_SITE_KIND_NONE:=00h
COREWEBVIEW2_COOKIE_SAME_SITE_KIND_LAX:=01h
COREWEBVIEW2_COOKIE_SAME_SITE_KIND_STRICT:=02h

; enum COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND
COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY:=00h
COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW:=01h
COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS:=02h

; enum COREWEBVIEW2_SCRIPT_DIALOG_KIND
COREWEBVIEW2_SCRIPT_DIALOG_KIND_ALERT:=00h
COREWEBVIEW2_SCRIPT_DIALOG_KIND_CONFIRM:=01h
COREWEBVIEW2_SCRIPT_DIALOG_KIND_PROMPT:=02h
COREWEBVIEW2_SCRIPT_DIALOG_KIND_BEFOREUNLOAD:=03h

; enum COREWEBVIEW2_PROCESS_FAILED_KIND
COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED:=00h
COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED:=01h
COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_UNRESPONSIVE:=02h
COREWEBVIEW2_PROCESS_FAILED_KIND_FRAME_RENDER_PROCESS_EXITED:=03h
COREWEBVIEW2_PROCESS_FAILED_KIND_UTILITY_PROCESS_EXITED:=04h
COREWEBVIEW2_PROCESS_FAILED_KIND_SANDBOX_HELPER_PROCESS_EXITED:=05h
COREWEBVIEW2_PROCESS_FAILED_KIND_GPU_PROCESS_EXITED:=06h
COREWEBVIEW2_PROCESS_FAILED_KIND_PPAPI_PLUGIN_PROCESS_EXITED:=07h
COREWEBVIEW2_PROCESS_FAILED_KIND_PPAPI_BROKER_PROCESS_EXITED:=08h
COREWEBVIEW2_PROCESS_FAILED_KIND_UNKNOWN_PROCESS_EXITED:=09h

; enum COREWEBVIEW2_PROCESS_FAILED_REASON
COREWEBVIEW2_PROCESS_FAILED_REASON_UNEXPECTED:=00h
COREWEBVIEW2_PROCESS_FAILED_REASON_UNRESPONSIVE:=01h
COREWEBVIEW2_PROCESS_FAILED_REASON_TERMINATED:=02h
COREWEBVIEW2_PROCESS_FAILED_REASON_CRASHED:=03h
COREWEBVIEW2_PROCESS_FAILED_REASON_LAUNCH_FAILED:=04h
COREWEBVIEW2_PROCESS_FAILED_REASON_OUT_OF_MEMORY:=05h

; enum COREWEBVIEW2_PERMISSION_KIND
COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION:=00h
COREWEBVIEW2_PERMISSION_KIND_MICROPHONE:=01h
COREWEBVIEW2_PERMISSION_KIND_CAMERA:=02h
COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION:=03h
COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS:=04h
COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS:=05h
COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ:=06h

; enum COREWEBVIEW2_PERMISSION_STATE
COREWEBVIEW2_PERMISSION_STATE_DEFAULT:=00h
COREWEBVIEW2_PERMISSION_STATE_ALLOW:=01h
COREWEBVIEW2_PERMISSION_STATE_DENY:=02h

; enum COREWEBVIEW2_WEB_ERROR_STATUS
COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN:=00h
COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT:=01h
COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED:=02h
COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS:=03h
COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED:=04h
COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID:=05h
COREWEBVIEW2_WEB_ERROR_STATUS_SERVER_UNREACHABLE:=06h
COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT:=07h
COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE:=08h
COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED:=09h
COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET:=0Ah
COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED:=0Bh
COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT:=0Ch
COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED:=0Dh
COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED:=0Eh
COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED:=0Fh
COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR:=010h

; enum COREWEBVIEW2_WEB_RESOURCE_CONTEXT
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL:=00h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT:=01h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_STYLESHEET:=02h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE:=03h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_MEDIA:=04h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FONT:=05h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_SCRIPT:=06h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST:=07h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FETCH:=08h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_TEXT_TRACK:=09h
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_EVENT_SOURCE:=0Ah
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_WEBSOCKET:=0Bh
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_MANIFEST:=0Ch
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_SIGNED_EXCHANGE:=0Dh
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_PING:=0Eh
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_CSP_VIOLATION_REPORT:=0Fh
COREWEBVIEW2_WEB_RESOURCE_CONTEXT_OTHER:=010h

; enum COREWEBVIEW2_MOVE_FOCUS_REASON
COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC:=00h
COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT:=01h
COREWEBVIEW2_MOVE_FOCUS_REASON_PREVIOUS:=02h

; enum COREWEBVIEW2_KEY_EVENT_KIND
COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN:=00h
COREWEBVIEW2_KEY_EVENT_KIND_KEY_UP:=01h
COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN:=02h
COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_UP:=03h

; enum COREWEBVIEW2_MOUSE_EVENT_KIND
COREWEBVIEW2_MOUSE_EVENT_KIND_HORIZONTAL_WHEEL:=0020Eh
COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOUBLE_CLICK:=00203h
COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOWN:=00201h
COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP:=00202h
COREWEBVIEW2_MOUSE_EVENT_KIND_LEAVE:=002A3h
COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOUBLE_CLICK:=00209h
COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOWN:=00207h
COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP:=00208h
COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE:=00200h
COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOUBLE_CLICK:=00206h
COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOWN:=00204h
COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP:=00205h
COREWEBVIEW2_MOUSE_EVENT_KIND_WHEEL:=0020Ah
COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_DOUBLE_CLICK:=0020Dh
COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_DOWN:=0020Bh
COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_UP:=0020Ch

; enum COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE:=00h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_LEFT_BUTTON:=00001h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_RIGHT_BUTTON:=00002h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_SHIFT:=00004h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_CONTROL:=00008h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_MIDDLE_BUTTON:=00010h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_X_BUTTON1:=00020h
COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_X_BUTTON2:=00040h

; enum COREWEBVIEW2_POINTER_EVENT_KIND
COREWEBVIEW2_POINTER_EVENT_KIND_ACTIVATE:=0024Bh
COREWEBVIEW2_POINTER_EVENT_KIND_DOWN:=00246h
COREWEBVIEW2_POINTER_EVENT_KIND_ENTER:=00249h
COREWEBVIEW2_POINTER_EVENT_KIND_LEAVE:=0024Ah
COREWEBVIEW2_POINTER_EVENT_KIND_UP:=00247h
COREWEBVIEW2_POINTER_EVENT_KIND_UPDATE:=00245h

; enum COREWEBVIEW2_BOUNDS_MODE
COREWEBVIEW2_BOUNDS_MODE_USE_RAW_PIXELS:=0
COREWEBVIEW2_BOUNDS_MODE_USE_RASTERIZATION_SCALE:=1

struct COREWEBVIEW2_PHYSICAL_KEY_STATUS
	RepeatCount	dd ?	; u32
	ScanCode	dd ?	; u32
	IsExtendedKey	dd ?	; BOOL
	IsMenuKeyDown	dd ?	; BOOL
	WasKeyDown	dd ?	; BOOL
	IsKeyReleased	dd ?	; BOOL
ends

struct COREWEBVIEW2_COLOR
	A	db ?
	R	db ?
	G	db ?
	B	db ?
ends

interface ICoreWebView2::IUnknown,\
	76ECEACB-0462-4D94-AC83-423A6793775E,\
	get_Settings,\
	get_Source,\
	Navigate,\
	NavigateToString,\
	add_NavigationStarting,			remove_NavigationStarting,\
	add_ContentLoading,			remove_ContentLoading,\
	add_SourceChanged,			remove_SourceChanged,\
	add_HistoryChanged,			remove_HistoryChanged,\
	add_NavigationCompleted,		remove_NavigationCompleted,\
	add_FrameNavigationStarting,		remove_FrameNavigationStarting,\
	add_FrameNavigationCompleted,		remove_FrameNavigationCompleted,\
	add_ScriptDialogOpening,		remove_ScriptDialogOpening,\
	add_PermissionRequested,		remove_PermissionRequested,\
	add_ProcessFailed,			remove_ProcessFailed,\
	AddScriptToExecuteOnDocumentCreated,	RemoveScriptToExecuteOnDocumentCreated,\
	ExecuteScript,\
	CapturePreview,\
	Reload,\
	PostWebMessageAsJson,\
	PostWebMessageAsString,\
	add_WebMessageReceived,			remove_WebMessageReceived,\
	CallDevToolsProtocolMethod,\
	get_BrowserProcessId,\
	get_CanGoBack,\
	get_CanGoForward,\
	GoBack,\
	GoForward,\
	GetDevToolsProtocolEventReceiver,\
	Stop,\
	add_NewWindowRequested,			remove_NewWindowRequested,\
	add_DocumentTitleChanged,		remove_DocumentTitleChanged,\
	get_DocumentTitle,\
	AddHostObjectToScript,			RemoveHostObjectFromScript,\
	OpenDevToolsWindow,\
	add_ContainsFullScreenElementChanged,	remove_ContainsFullScreenElementChanged,\
	get_ContainsFullScreenElement,\
	add_WebResourceRequested,		remove_WebResourceRequested,\
	AddWebResourceRequestedFilter,		RemoveWebResourceRequestedFilter,\
	add_WindowCloseRequested,		remove_WindowCloseRequested

interface ICoreWebView2_2::ICoreWebView2,\
	9E8F0CF8-E670-4B5E-B2BC-73E061E3184C,\
	add_WebResourceResponseReceived,	remove_WebResourceResponseReceived,\
	NavigateWithWebResourceRequest,\
	add_DOMContentLoaded,			remove_DOMContentLoaded,\
	get_CookieManager,\
	get_Environment

interface ICoreWebView2_3::ICoreWebView2_2,\
	A0D6DF20-3B92-416D-AA0C-437A9C727857,\
	TrySuspend,\
	Resume,\
	get_IsSuspended,\
	SetVirtualHostNameToFolderMapping,\
	ClearVirtualHostNameToFolderMapping


interface ICoreWebView2TrySuspendCompletedHandler::IUnknown,\
	00F206A7-9D17-4605-91F6-4E8E4DE192E3,\
	Invoke

interface ICoreWebView2Controller::IUnknown,\
	4D00C0D1-9434-4EB6-8078-8697A560334F,\
	get_IsVisible,				put_IsVisible,\
	get_Bounds,				put_Bounds,\
	get_ZoomFactor,				put_ZoomFactor,\
	add_ZoomFactorChanged,			remove_ZoomFactorChanged,\
	SetBoundsAndZoomFactor,\
	MoveFocus,\
	add_MoveFocusRequested,			remove_MoveFocusRequested,\
	add_GotFocus,				remove_GotFocus,\
	add_LostFocus,				remove_LostFocus,\
	add_AcceleratorKeyPressed,		remove_AcceleratorKeyPressed,\
	get_ParentWindow,			put_ParentWindow,\
	NotifyParentWindowPositionChanged,\
	Close,\
	get_CoreWebView2

interface ICoreWebView2Controller2::ICoreWebView2Controller,\
	C979903E-D4CA-4228-92EB-47EE3FA96EAB,\
	get_DefaultBackgroundColor,		put_DefaultBackgroundColor

interface ICoreWebView2Controller3::ICoreWebView2Controller2,\
	F9614724-5D2B-41DC-AEF7-73D62B51543B,\
	get_RasterizationScale,			put_RasterizationScale,\
	get_ShouldDetectMonitorScaleChanges,	put_ShouldDetectMonitorScaleChanges,\
	add_RasterizationScaleChanged,		remove_RasterizationScaleChanged,\
	get_BoundsMode,				put_BoundsMode


interface ICoreWebView2CompositionController::IUnknown,\
	3DF9B733-B9AE-4A15-86B4-EB9EE9826469,\
	get_RootVisualTarget,			put_RootVisualTarget,\
	SendMouseInput,\
	SendPointerInput,\
	get_Cursor,\
	get_SystemCursorId,\
	add_CursorChanged,			remove_CursorChanged

interface ICoreWebView2CompositionController2::ICoreWebView2CompositionController,\
	0B6A3D24-49CB-4806-BA20-B5E0734A7B26,\
	get_UIAProvider


interface ICoreWebView2Deferral::IUnknown,\
	C10E7F7B-B585-46F0-A623-8BEFBF3E4EE0,\
	Complete


interface ICoreWebView2Settings::IUnknown,\
	E562E4F0-D7FA-43AC-8D71-C05150499F00,\
	get_IsScriptEnabled,			put_IsScriptEnabled,\
	get_IsWebMessageEnabled,		put_IsWebMessageEnabled,\
	get_AreDefaultScriptDialogsEnabled,	put_AreDefaultScriptDialogsEnabled,\
	get_IsStatusBarEnabled,			put_IsStatusBarEnabled,\
	get_AreDevToolsEnabled,			put_AreDevToolsEnabled,\
	get_AreDefaultContextMenusEnabled,	put_AreDefaultContextMenusEnabled,\
	get_AreHostObjectsAllowed,		put_AreHostObjectsAllowed,\
	get_IsZoomControlEnabled,		put_IsZoomControlEnabled,\
	get_IsBuiltInErrorPageEnabled,		put_IsBuiltInErrorPageEnabled

interface ICoreWebView2Settings2::ICoreWebView2Settings,\
	EE9A0F68-F46C-4E32-AC23-EF8CAC224D2A,\
	get_UserAgent,				put_UserAgent

interface ICoreWebView2Settings3::ICoreWebView2Settings2,\
	FDB5AB74-AF33-4854-84F0-0A631DEB5EBA,\
	get_AreBrowserAcceleratorKeysEnabled,	put_AreBrowserAcceleratorKeysEnabled


interface ICoreWebView2ProcessFailedEventArgs::IUnknown,\
	8155A9A4-1474-4A86-8CAE-151B0FA6B8CA,\
	get_ProcessFailedKind

interface ICoreWebView2ProcessFailedEventHandler::IUnknown,\
	79E0AEA4-990B-42D9-AA1D-0FCC2E5BC7F1,\
	Invoke

interface ICoreWebView2ZoomFactorChangedEventHandler::IUnknown,\
	B52D71D6-C4DF-4543-A90C-64A3E60F38CB,\
	Invoke

interface ICoreWebView2HttpHeadersCollectionIterator::IUnknown,\
	0702FC30-F43B-47BB-AB52-A42CB552AD9F,\
	GetCurrentHeader,\
	get_HasCurrentHeader,\
	MoveNext

interface ICoreWebView2HttpRequestHeaders::IUnknown,\
	E86CAC0E-5523-465C-B536-8FB9FC8C8C60,\
	GetHeader,\
	GetHeaders,\
	Contains,\
	SetHeader,\
	RemoveHeader,\
	GetIterator

interface ICoreWebView2HttpResponseHeaders::IUnknown,\
	03C5FF5A-9B45-4A88-881C-89A9F328619C,\
	AppendHeader,\
	Contains,\
	GetHeader,\
	GetHeaders,\
	GetIterator

interface ICoreWebView2WebResourceRequest::IUnknown,\
	97055CD4-512C-4264-8B5F-E3F446CEA6A5,\
	get_Uri,				put_Uri,\
	get_Method,				put_Method,\
	get_Content,				put_Content,\
	get_Headers

interface ICoreWebView2WebResourceResponse::IUnknown,\
	AAFCC94F-FA27-48FD-97DF-830EF75AAEC9,\
	get_Content,				put_Content,\
	get_Headers,\
	get_StatusCode,				put_StatusCode,\
	get_ReasonPhrase,			put_ReasonPhrase

interface ICoreWebView2NavigationStartingEventArgs::IUnknown,\
	5B495469-E119-438A-9B18-7604F25F2E49,\
	get_Uri,\
	get_IsUserInitiated,\
	get_IsRedirected,\
	get_RequestHeaders,\
	get_Cancel,				put_Cancel,\
	get_NavigationId

interface ICoreWebView2NavigationStartingEventHandler::IUnknown,\
	9ADBE429-F36D-432B-9DDC-F8881FBD76E3,\
	Invoke

interface ICoreWebView2ContentLoadingEventArgs::IUnknown,\
	0C8A1275-9B6B-4901-87AD-70DF25BAFA6E,\
	get_IsErrorPage,\
	get_NavigationId

interface ICoreWebView2ContentLoadingEventHandler::IUnknown,\
	364471E7-F2BE-4910-BDBA-D72077D51C4B,\
	Invoke

interface ICoreWebView2SourceChangedEventArgs::IUnknown,\
	31E0E545-1DBA-4266-8914-F63848A1F7D7,\
	get_IsNewDocument

interface ICoreWebView2SourceChangedEventHandler::IUnknown,\
	3C067F9F-5388-4772-8B48-79F7EF1AB37C,\
	Invoke

interface ICoreWebView2HistoryChangedEventHandler::IUnknown,\
	C79A420C-EFD9-4058-9295-3E8B4BCAB645,\
	Invoke

interface ICoreWebView2ScriptDialogOpeningEventArgs::IUnknown,\
	7390BB70-ABE0-4843-9529-F143B31B03D6,\
	get_Uri,\
	get_Kind,\
	get_Message,\
	Accept,\
	get_DefaultText,\
	get_ResultText,				put_ResultText,\
	GetDeferral

interface ICoreWebView2ScriptDialogOpeningEventHandler::IUnknown,\
	EF381BF9-AFA8-4E37-91C4-8AC48524BDFB,\
	Invoke

interface ICoreWebView2NavigationCompletedEventArgs::IUnknown,\
	30D68B7D-20D9-4752-A9CA-EC8448FBB5C1,\
	get_IsSuccess,\
	get_WebErrorStatus,\
	get_NavigationId

interface ICoreWebView2NavigationCompletedEventHandler::IUnknown,\
	D33A35BF-1C49-4F98-93AB-006E0533FE1C,\
	Invoke

interface ICoreWebView2PermissionRequestedEventArgs::IUnknown,\
	973AE2EF-FF18-4894-8FB2-3C758F046810,\
	get_Uri,\
	get_PermissionKind,\
	get_IsUserInitiated,\
	get_State,				put_State,\
	GetDeferral

interface ICoreWebView2PermissionRequestedEventHandler::IUnknown,\
	15E1C6A3-C72A-4DF3-91D7-D097FBEC6BFD,\
	Invoke

interface ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler::IUnknown,\
	B99369F3-9B11-47B5-BC6F-8E7895FCEA17,\
	Invoke

interface ICoreWebView2ExecuteScriptCompletedHandler::IUnknown,\
	49511172-CC67-4BCA-9923-137112F4C4CC,\
	Invoke

interface ICoreWebView2WebResourceRequestedEventArgs::IUnknown,\
	453E667F-12C7-49D4-BE6D-DDBE7956F57A,\
	get_Request,\
	get_Response,				put_Response,\
	GetDeferral,\
	get_ResourceContext

interface ICoreWebView2WebResourceRequestedEventHandler::IUnknown,\
	AB00B74C-15F1-4646-80E8-E76341D25D71,\
	Invoke

interface ICoreWebView2CapturePreviewCompletedHandler::IUnknown,\
	697E05E9-3D8F-45FA-96F4-8FFE1EDEDAF5,\
	Invoke

interface ICoreWebView2FocusChangedEventHandler::IUnknown,\
	05EA24BD-6452-4926-9014-4B82B498135D,\
	Invoke

interface ICoreWebView2MoveFocusRequestedEventArgs::IUnknown,\
	2D6AA13B-3839-4A15-92FC-D88B3C0D9C9D,\
	get_Reason,\
	get_Handled,				put_Handled

interface ICoreWebView2MoveFocusRequestedEventHandler::IUnknown,\
	69035451-6DC7-4CB8-9BCE-B2BD70AD289F,\
	Invoke

interface ICoreWebView2WebMessageReceivedEventArgs::IUnknown,\
	0F99A40C-E962-4207-9E92-E3D542EFF849,\
	get_Source,\
	get_WebMessageAsJson,\
	TryGetWebMessageAsString

interface ICoreWebView2WebMessageReceivedEventHandler::IUnknown,\
	57213F19-00E6-49FA-8E07-898EA01ECBD2,\
	Invoke

interface ICoreWebView2DevToolsProtocolEventReceivedEventArgs::IUnknown,\
	653C2959-BB3A-4377-8632-B58ADA4E66C4,\
	get_ParameterObjectAsJson

interface ICoreWebView2DevToolsProtocolEventReceivedEventHandler::IUnknown,\
	E2FDA4BE-5456-406C-A261-3D452138362C,\
	Invoke

interface ICoreWebView2CallDevToolsProtocolMethodCompletedHandler::IUnknown,\
	5C4889F0-5EF6-4C5A-952C-D8F1B92D0574,\
	Invoke

interface ICoreWebView2CreateCoreWebView2ControllerCompletedHandler::IUnknown,\
	6C4819F3-C9B7-4260-8127-C9F5BDE7F68C,\
	Invoke

interface ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler::IUnknown,\
	02FAB84B-1428-4FB7-AD45-1B2E64736184,\
	Invoke

interface ICoreWebView2NewWindowRequestedEventArgs::IUnknown,\
	34ACB11C-FC37-4418-9132-F9C21D1EAFB9,\
	get_Uri,\
	put_NewWindow,				get_NewWindow,\
	put_Handled,				get_Handled,\
	get_IsUserInitiated,\
	GetDeferral,\
	get_WindowFeatures

interface ICoreWebView2WindowFeatures::IUnknown,\
	5EAF559F-B46E-4397-8860-E422F287FF1E,\
	get_HasPosition,\
	get_HasSize,\
	get_Left,\
	get_Top,\
	get_Height,\
	get_Width,\
	get_ShouldDisplayMenuBar,\
	get_ShouldDisplayStatus,\
	get_ShouldDisplayToolbar,\
	get_ShouldDisplayScrollBars

interface ICoreWebView2NewWindowRequestedEventHandler::IUnknown,\
	D4C185FE-C81C-4989-97AF-2D3FA7AB5651,\
	Invoke

interface ICoreWebView2DocumentTitleChangedEventHandler::IUnknown,\
	F5F2B923-953E-4042-9F95-F3A118E1AFD4,\
	Invoke

interface ICoreWebView2AcceleratorKeyPressedEventArgs::IUnknown,\
	9F760F8A-FB79-42BE-9990-7B56900FA9C7,\
	get_KeyEventKind,\
	get_VirtualKey,\
	get_KeyEventLParam,\
	get_PhysicalKeyStatus,\
	get_Handled,				put_Handled

interface ICoreWebView2AcceleratorKeyPressedEventHandler::IUnknown,\
	B29C7E28-FA79-41A8-8E44-65811C76DCB2,\
	Invoke

interface ICoreWebView2NewBrowserVersionAvailableEventHandler::IUnknown,\
	F9A2976E-D34E-44FC-ADEE-81B6B57CA914,\
	Invoke

interface ICoreWebView2ContainsFullScreenElementChangedEventHandler::IUnknown,\
	E45D98B1-AFEF-45BE-8BAF-6C7728867F73,\
	Invoke

interface ICoreWebView2WindowCloseRequestedEventHandler::IUnknown,\
	5C19E9E0-092F-486B-AFFA-CA8231913039,\
	Invoke

interface ICoreWebView2WebResourceResponseReceivedEventHandler::IUnknown,\
	7DE9898A-24F5-40C3-A2DE-D4F458E69828,\
	Invoke

interface ICoreWebView2WebResourceResponseReceivedEventArgs::IUnknown,\
	D1DB483D-6796-4B8B-80FC-13712BB716F4,\
	get_Request,\
	get_Response

interface ICoreWebView2WebResourceResponseView::IUnknown,\
	79701053-7759-4162-8F7D-F1B3F084928D,\
	get_Headers,\
	get_StatusCode,\
	get_ReasonPhrase,\
	GetContent

interface ICoreWebView2WebResourceResponseViewGetContentCompletedHandler::IUnknown,\
	875738E1-9FA2-40E3-8B74-2E8972DD6FE7,\
	Invoke

interface ICoreWebView2DOMContentLoadedEventArgs::IUnknown,\
	16B1E21A-C503-44F2-84C9-70ABA5031283,\
	get_NavigationId

interface ICoreWebView2DOMContentLoadedEventHandler::IUnknown,\
	4BAC7E9C-199E-49ED-87ED-249303ACF019,\
	Invoke

interface ICoreWebView2Cookie::IUnknown,\
	AD26D6BE-1486-43E6-BF87-A2034006CA21,\
	get_Name,\
	get_Value,				put_Value,\
	get_Domain,\
	get_Path,\
	get_Expires,				put_Expires,\
	get_IsHttpOnly,				put_IsHttpOnly,\
	get_SameSite,				put_SameSite,\
	get_IsSecure,				put_IsSecure,\
	get_IsSession

interface ICoreWebView2CookieManager::IUnknown,\
	177CD9E7-B6F5-451A-94A0-5D7A3A4C4141,\
	CreateCookie,\
	CopyCookie,\
	GetCookies,\
	AddOrUpdateCookie,\
	DeleteCookie,\
	DeleteCookies,\
	DeleteCookiesWithDomainAndPath,\
	DeleteAllCookies

interface ICoreWebView2CookieList::IUnknown,\
	F7F6F714-5D2A-43C6-9503-346ECE02D186,\
	get_Count,\
	GetValueAtIndex

interface ICoreWebView2GetCookiesCompletedHandler::IUnknown,\
	5A4F5069-5C15-47C3-8646-F4DE1C116670,\
	Invoke

interface ICoreWebView2PointerInfo::IUnknown,\
	E6995887-D10D-4F5D-9359-4CE46E4F96B9,\
	get_PointerKind,			put_PointerKind,\
	get_PointerId,				put_PointerId,\
	get_FrameId,				put_FrameId,\
	get_PointerFlags,			put_PointerFlags,\
	get_PointerDeviceRect,			put_PointerDeviceRect,\
	get_DisplayRect,			put_DisplayRect,\
	get_PixelLocation,			put_PixelLocation,\
	get_HimetricLocation,			put_HimetricLocation,\
	get_PixelLocationRaw,			put_PixelLocationRaw,\
	get_HimetricLocationRaw,		put_HimetricLocationRaw,\
	get_Time,				put_Time,\
	get_HistoryCount,			put_HistoryCount,\
	get_InputData,				put_InputData,\
	get_KeyStates,				put_KeyStates,\
	get_PerformanceCount,			put_PerformanceCount,\
	get_ButtonChangeKind,			put_ButtonChangeKind,\
	get_PenFlags,				put_PenFlags,\
	get_PenMask,				put_PenMask,\
	get_PenPressure,			put_PenPressure,\
	get_PenRotation,			put_PenRotation,\
	get_PenTiltX,				put_PenTiltX,\
	get_PenTiltY,				put_PenTiltY,\
	get_TouchFlags,				put_TouchFlags,\
	get_TouchMask,				put_TouchMask,\
	get_TouchContact,			put_TouchContact,\
	get_TouchContactRaw,			put_TouchContactRaw,\
	get_TouchOrientation,			put_TouchOrientation,\
	get_TouchPressure,			put_TouchPressure

interface ICoreWebView2CursorChangedEventHandler::IUnknown,\
	9DA43CCC-26E1-4DAD-B56C-D8961C94C571,\
	Invoke

interface ICoreWebView2RasterizationScaleChangedEventHandler::IUnknown,\
	9C98C8B1-AC53-427E-A345-3049B5524BBE,\
	Invoke

interface ICoreWebView2Environment::IUnknown,\
	B96D755E-0319-4E92-A296-23436F46A1FC,\
	CreateCoreWebView2Controller,\
	CreateWebResourceResponse,\
	get_BrowserVersionString,\
	add_NewBrowserVersionAvailable,		remove_NewBrowserVersionAvailable

interface ICoreWebView2Environment2::ICoreWebView2Environment,\
	41F3632B-5EF4-404F-AD82-2D606C5A9A21,\
	CreateWebResourceRequest

interface ICoreWebView2Environment3::ICoreWebView2Environment2,\
	80A22AE3-BE7C-4CE2-AFE1-5A50056CDEEB,\
	CreateCoreWebView2CompositionController,\
	CreateCoreWebView2PointerInfo

interface ICoreWebView2Environment4::ICoreWebView2Environment3,\
	20944379-6DCF-41D6-A0A0-ABC0FC50DE0D,\
	GetProviderForHwnd

interface ICoreWebView2EnvironmentOptions::IUnknown,\
	2FDE08A8-1E9A-4766-8C05-95A9CEB9D1C5,\
	get_AdditionalBrowserArguments,			put_AdditionalBrowserArguments,\
	get_Language,					put_Language,\
	get_TargetCompatibleBrowserVersion,		put_TargetCompatibleBrowserVersion,\
	get_AllowSingleSignOnUsingOSPrimaryAccount,	put_AllowSingleSignOnUsingOSPrimaryAccount

interface ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler::IUnknown,\
	4E8A3389-C9D8-4BD2-B6B5-124FEE6CC14D,\
	Invoke

interface ICoreWebView2DevToolsProtocolEventReceiver::IUnknown,\
	B32CA51A-8371-45E9-9317-AF021D080367,\
	add_DevToolsProtocolEventReceived,	remove_DevToolsProtocolEventReceived

interface ICoreWebView2ProcessFailedEventArgs2::ICoreWebView2ProcessFailedEventArgs,\
	4DAB9422-46FA-4C3E-A5D2-41D2071D3680,\
	get_Reason,\
	get_ExitCode,\
	get_ProcessDescription,\
	get_FrameInfosForFailedProcess

interface ICoreWebView2FrameInfoCollection::IUnknown,\
	8F834154-D38E-4D90-AFFB-6800A7272839,\
	GetIterator

interface ICoreWebView2FrameInfoCollectionIterator::IUnknown,\
	1BF89E2D-1B2B-4629-B28F-05099B41BB03,\
	get_HasCurrent,\
	GetCurrent,\
	MoveNext

interface ICoreWebView2FrameInfo::IUnknown,\
	DA86B8A1-BDF3-4F11-9955-528CEFA59727,\
	get_Name,\
	get_Source

interface ICoreWebView2Interop::IUnknown,\
	912B34A7-D10B-49C4-AF18-7CB7E604E01A,\
	AddHostObjectToScript

interface ICoreWebView2CompositionControllerInterop::IUnknown,\
	8E9922CE-9C80-42E6-BAD7-FCEBF291A495,\
	get_UIAProvider,\
	get_RootVisualTarget,			put_RootVisualTarget

interface ICoreWebView2EnvironmentInterop::IUnknown,\
	EE503A63-C1E2-4FBF-8A4D-824E95F8BB13,\
	GetProviderForHwnd
