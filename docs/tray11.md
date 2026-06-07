When writing code against the Win32 API, navigating the modern Windows 11 system tray requires a clear distinction between the legacy `Shell_NotifyIcon` subsystem and modern runtime behaviors.

Microsoft has not significantly modified the `NOTIFYICONDATA` structure since Windows 7/8. Instead, the "advanced" features of Windows 11 are governed by strict API usage rules, shell integration mechanics, and the requirement to map Win32 handles to modern COM/WinRT notification pipelines.

The following details outline what you need to implement for a high-performance, native tray application on Windows 11.

---

## 1. Permanent Identity via GUIDs (`NIF_GUID`)

Historically, tray icons were identified by a combination of the parent window handle (`hWnd`) and an application-defined ID (`uID`). In Windows 11, this approach causes major friction: if your executable is moved, recompiled, or updated, its position, visibility, and user preferences in the Settings app (`Taskbar settings -> Other system tray icons`) are reset or duplicated.

To fix this, you must assign a hardcoded, static GUID to your tray icon. Windows 11 uses this GUID to track user preferences across application updates.

```c
#include <windows.h>
#include <shellapi.h>

// {A1B2C3D4-E5F6-7A8B-9C0D-1E2F3A4B5C6D}
static const GUID MyTrayIconGuid = 
{ 0xA1B2C3D4, 0xE5F6, 0x7A8B, { 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D } };

void AddTrayIcon(HWND hWnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uFlags = NIF_GUID | NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.guidItem = MyTrayIconGuid;
    nid.uCallbackMessage = WM_USER_TRAYCALLBACK;
    
    // Load high-resolution icon asset
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
    wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), L"My Advanced C App");

    Shell_NotifyIconW(NIM_ADD, &nid);
    
    // Set version behavior to modern (Vista or later specifies rich input tracking)
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

```

## 2. Input Tracking with `NOTIFYICON_VERSION_4`

Calling `NIM_SETVERSION` with `NOTIFYICON_VERSION_4` completely re-maps the window messages routed to your `uCallbackMessage`. Instead of basic mouse button down/up tracking, the `lParam` and `wParam` unpack into explicit screen coordinates and detailed pointer metadata.

When the user interacts with your tray icon, `lParam` contains the actual event, while `wParam` can be unpacked to find precise layout coordinates, which are essential for aligning a custom context menu.

```c
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_USER_TRAYCALLBACK) {
        // Under NOTIFYICON_VERSION_4, lParam holds the structural message
        switch (LOWORD(lParam)) {
            case WM_CONTEXTMENU: {
                // Get absolute mouse pointer coordinates packed into wParam
                int xPos = GET_X_LPARAM(wParam);
                int yPos = GET_Y_LPARAM(wParam);
                
                ShowContextMenu(hWnd, xPos, yPos);
                return TRUE;
            }
            case NIN_BALLOONUSERCLICK:
                // Handle rich toast interaction
                return TRUE;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

```

## 3. High-DPI and High-Resolution Scaling

Windows 11 scaling engines aggressively target high-DPI environments. If your application provides only standard `16x16` or `32x32` icons, the OS will upscale the bitmap asset, resulting in a blurry tray rendering.

To prevent this:

* Ensure your `.ico` resource file explicitly packs images up to `256x256` (PNG-compressed inside the ICO container).
* Never use standard `LoadIcon`. Instead, use `LoadIconMetric` to fetch the exact physical pixel size requested by the specific display scale factor currently hosting the taskbar.

```c
#include <commctrl.h> // Required for LoadIconMetric

nid.hIcon = NULL;
HRESULT hr = LoadIconMetric(hInstance, MAKEINTRESOURCE(IDI_MY_ICON), LIM_SMALL, &nid.hIcon);
if (SUCCEEDED(hr)) {
    // Apply icon to NOTIFYICONDATAW
}

```

## 4. Bypassing Legacy Balloons for Native Windows 11 Toasts

If you use `Shell_NotifyIconW` with the `NIF_INFO` flag to fire an old-school balloon notification, Windows 11 intercepts it and maps it internally to a modern UI toast. However, these intercepted messages are heavily throttled, strip rich layouts, and can drop entirely if the calling window structure is a message-only queue (`HWND_MESSAGE`).

To deploy modern, highly advanced notifications, bypass `Shell_NotifyIcon` for alerts and use the Windows Runtime C-interface (`RoInitialize`) or raw COM activation to invoke `Windows.UI.Notifications`.

This requires:

1. An Application User Model ID (AppUserModelID) explicitly set on the process via `SetCurrentProcessExplicitAppUserModelID`.
2. An application manifest declaring compatibility with Windows 11.
3. Generating an XML layout string defining the notification payload.

A structural example of the XML required to tap into the modern notification engine:

```xml
<toast duration="short">
    <visual>
        <binding template="ToastGeneric">
            <text>Performance Alert</text>
            <text>SIMD Engine utilized at 94% capacity.</text>
            <image placement="appLogoOverride" hint-crop="circle" src="file:///C:/Path/To/AppIcon.png"/>
        </binding>
    </visual>
    <actions>
        <action content="Open Dashboard" arguments="action=manage" activationType="foreground"/>
        <action content="Dismiss" arguments="action=dismiss" activationType="background"/>
    </actions>
</toast>

```

To run this XML string, you can consume the low-level `IInspectable` and `IToastNotificationManagerStatics` interfaces via COM activation, or compile your runtime components using standard C++ interop files (`.cpp`) to handle the heavy WinRT type definitions while leaving the core engine loop low-level.

---

## 5. Essential Shell Recovery

Because your tray application is decoupled from the Windows Shell graphics process (`explorer.exe`), if the shell restarts due to a crash or a configuration change, your tray icon will vanish completely from the system tray while your process continues to run in the background.

You must register for the `TaskbarCreated` broadcast message during runtime initialization to re-register your icon if the shell environment tears down.

```c
static UINT WM_TASKBARCREATED = 0;

// Inside your main/initialization function:
WM_TASKBARCREATED = RegisterWindowMessageW(L"TaskbarCreated");

// Inside your window procedure (WndProc):
if (msg == WM_TASKBARCREATED) {
    // The shell restarted; re-run your NIM_ADD routines here
    AddTrayIcon(hWnd);
    return 0;
}

```

---

To be a "good citizen" in the Windows 11 system tray, your unmanaged C application needs to respect the asynchronous design of the modern Explorer shell, adapt instantly to user accessibility settings, and handle memory and window lifetimes cleanly.

The following architectural rules will ensure your application feels like a native part of the OS rather than a legacy port.

---

## 6. Respect the Single-Click vs. Double-Click Contract

A common anti-pattern is executing a primary action (like opening the main window) immediately upon receiving `WM_LBUTTONDOWN`. This breaks standard Windows ergonomics and blocks the user's ability to trigger double-clicks.

Windows 11 expects your tray icon callback loop to distinguish between interactions smoothly:

* **Single Left Click:** Open a lightweight status flyout or prepare for a double-click.
* **Double Left Click:** Open the primary application window.
* **Right Click:** Safely render the context menu.

To implement this without lag, you must track mouse timing manually using `GetDoubleClickTime()` if you want to respond to a single click *only* if a second one doesn't follow.

```c
#define TIMER_SINGLE_CLICK 1001

// Inside your window message loop / tray callback:
case WM_LBUTTONUP: {
    // Postpone the single-click action to see if it becomes a double-click
    SetTimer(hWnd, TIMER_SINGLE_CLICK, GetDoubleClickTime(), NULL);
    break;
}
case WM_LBUTTONDBLCLK: {
    // Cancel the pending single-click action immediately
    KillTimer(hWnd, TIMER_SINGLE_CLICK);
    OpenPrimaryWindow(hWnd);
    break;
}

// Inside your WM_TIMER handler:
case WM_TIMER: {
    if (wParam == TIMER_SINGLE_CLICK) {
        KillTimer(hWnd, TIMER_SINGLE_CLICK);
        ShowLightweightFlyout(hWnd);
    }
    break;
}

```

## 7. Correct Menu Focus Management (`SetForegroundWindow`)

If you display a standard Win32 popup menu (`TrackPopupMenuEx`), you will quickly notice a bug: clicking outside the menu onto the desktop won't dismiss it. The menu hangs on screen like a zombie.

To make the context menu behave natively, you must bring your hidden helper window to the foreground right before displaying the menu, and then pump a null message immediately after it returns to force a focus yield.

```c
void ShowContextMenu(HWND hWnd, int xPos, int yPos) {
    HMENU hMenu = LoadMenuW(g_hInstance, MAKEINTRESOURCE(IDR_TRAY_MENU));
    HMENU hSubMenu = GetSubMenu(hMenu, 0);

    // CRITICAL STEP 1: Bring your window to the foreground
    SetForegroundWindow(hWnd);

    // Track the menu selection
    TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos, yPos, hWnd, NULL);

    // CRITICAL STEP 2: Force a context switch to prevent the menu sticky-focus bug
    PostMessageW(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

```

## 8. Handle System Power and Session Changes

Good tray applications do not waste CPU cycles or lock up file handles when the user walks away from their machine or puts it to sleep. You should register for power and session management messages via `RegisterPowerSettingNotification` or standard window message intercepts.

* **`WM_POWERBROADCAST` (`PBT_APMSUSPEND`):** Suspend heavy polling loops, network sockets, or file logging.
* **`WM_WTSSESSIONCHANGE` (`WTS_SESSION_LOCK`):** The user locked the workstation. Stop updating internal UI metrics or animating any icons until `WTS_SESSION_UNLOCK` is received.

## 9. Hide the Taskbar Button, Keep the Message Loop

A tray application should not clutter the user's main taskbar area with an active application tab while minimized. To achieve a clean "run in background" state:

1. Use an extended window style of `WS_EX_TOOLWINDOW` to prevent the window from appearing on the taskbar.
2. Alternatively, when minimizing, explicitly hide the primary window handle using `ShowWindow(hWnd, SW_HIDE)`. To bring it back via a double-click, use `ShowWindow(hWnd, SW_SHOW)` followed by `SetForegroundWindow(hWnd)`.

```c
// When user clicks the "Minimize to Tray" or "X" button
case WM_CLOSE: {
    ShowWindow(hWnd, SW_HIDE); // Disappears from taskbar, message loop stays alive
    return 0; // Prevent the window from destroying itself
}

```

## 10. Clean Teardown on Exit

If your process crashes or exits without explicitly removing its icon from the system tray, the icon remains drawn in the taskbar corner. It will only disappear when the user hovers their mouse over it, causing the OS to realize the underlying process handle is dead.

Always hook the exit lifecycle of your C runtime (`WM_DESTROY`, unhandled exception filters, or signals) to remove the icon cleanly before the process terminates.

```c
void RemoveTrayIcon(HWND hWnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uFlags = NIF_GUID;
    nid.guidItem = MyTrayIconGuid; // Must match the GUID used to create it

    Shell_NotifyIconW(NIM_DELETE, &nid);
}

```
