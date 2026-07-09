# Launching Elevated Processes on Windows — Methods, Mechanisms, and Trade-offs

Beyond calling `ShellExecuteEx()` with the `"runas"` verb to trigger a User Account
Control (UAC) prompt, Windows exposes several architecturally distinct ways to run a
process — or an isolated unit of code — with elevated privileges. Choosing correctly
requires understanding *which part of the security model each method actually
manipulates*: the integrity label on a token, the split-token relationship, the
Application Information Service (AIS) consent pipeline, the Service Control Manager, or
the loader's manifest handling.

This document first establishes that model, then documents each methodology against it,
with API surfaces, registry layout, error semantics, and security caveats.

---

## Part I — The Security Model You Are Negotiating With

Every elevation technique below is ultimately about obtaining a **primary token** whose
integrity level and group/privilege set are higher than the one your process was born
with. Understanding the token machinery makes the "why" of each method obvious.

### 1. Mandatory Integrity Control (integrity levels)

Since Vista, every token and every securable object carries a **mandatory integrity
label** — a SID of the form `S-1-16-x`. Access checks compare the caller's integrity
level against the object's `SYSTEM_MANDATORY_LABEL_ACE` before the DACL is even consulted
(the *no-write-up* / optional *no-read-up* policy).

| Level          | RID (SID suffix) | Typical occupant                                   |
|----------------|------------------|----------------------------------------------------|
| Untrusted      | `0x0000`         | Anonymous, restricted sandboxes                    |
| Low            | `0x1000`         | AppContainer, protected-mode browser tabs          |
| Medium         | `0x2000`         | **Standard user processes** (the shell, most apps) |
| Medium Plus    | `0x2100`         | (rare, transitional)                               |
| High           | `0x3000`         | **Elevated / administrator processes**             |
| System         | `0x4000`         | `NT AUTHORITY\SYSTEM`, services                    |
| Protected      | `0x5000`         | PPL / protected-process ceiling (not user-settable)|

"Elevation" in the everyday sense means moving from **Medium** to **High**. Gaining
**System** is a different (and stronger) operation, usually via a service.

Query the current process's integrity level with `GetTokenInformation(TokenIntegrityLevel)`
and read the RID of the last subauthority of the returned label SID.

### 2. The split-token (filtered token) model

When a member of the Administrators group logs on interactively, LSA does **not** hand
the shell a full administrator token. Instead it mints **two** linked tokens:

* A **filtered token** — Administrators is marked *deny-only*, most powerful privileges
  are stripped, integrity is **Medium**. This becomes the primary token of `explorer.exe`
  and everything it launches.
* A **full (linked / elevated) token** — the unfiltered **High**-integrity token with the
  full privilege set.

The two are joined by a link you can read (but generally not *act on* without privilege):

```c
TOKEN_LINKED_TOKEN lt;
DWORD n;
// From a Medium-IL admin process, this returns a handle to the High-IL token...
GetTokenInformation(hProcToken, TokenLinkedToken, &lt, sizeof lt, &n);
// ...but CreateProcessAsUser() with lt.LinkedToken still fails without
// SeAssignPrimaryTokenPrivilege, which a Medium-IL process does not hold.
// The link is informational, NOT a UAC bypass.
```

This is the single most important constraint to internalize: **possessing a handle to a
more powerful token is not the same as being allowed to assign it.** Every silent method
below works precisely because *something that already holds the required privilege*
(AIS, the Task Scheduler engine, a SYSTEM service) does the token assignment on your
behalf.

### 3. Elevation type — how to know what you are

`GetTokenInformation(TokenElevationType)` returns:

| Value | Constant                      | Meaning                                             |
|-------|-------------------------------|-----------------------------------------------------|
| `1`   | `TokenElevationTypeDefault`   | UAC off, or the built-in Administrator — no split   |
| `2`   | `TokenElevationTypeFull`      | Process is running with the **full** (elevated) token |
| `3`   | `TokenElevationTypeLimited`   | Process holds the **filtered** token; a linked token exists |

`TokenElevation` (`TOKEN_ELEVATION.TokenIsElevated`) gives the simpler boolean when you
only need "am I High-IL right now?".

### 4. The consent pipeline: AIS and `consent.exe`

The `"runas"` verb does not itself elevate. `ShellExecute` marshals the request to the
**Application Information Service** (AIS, `appinfo.dll`, hosted in a `svchost.exe`
`netsvcs` group and running as SYSTEM). AIS performs the RPC entry `RAiLaunchAdminProcess`:

1. Validates the target (path, signature, manifest, install-detection heuristics).
2. Switches to the **secure desktop** (a separate desktop under Winlogon that
   Medium-IL processes cannot draw on or automate) and launches `consent.exe` to display
   the prompt.
3. On approval, calls `CreateProcessAsUser()` using the caller's **linked full token** —
   AIS holds `SeAssignPrimaryTokenPrivilege`, so it can do what your Medium-IL process
   cannot.

So `ShellExecute("runas")` is really "ask the SYSTEM broker (AIS) to assign my linked
token after getting user consent." Several methods below are alternative brokers.

### 5. Auto-elevation (and why bypasses exist)

At the default UAC setting (*Notify me only when apps try to make changes* — **not**
*Always notify*), a curated set of Microsoft-signed executables located in **trusted
directories** (`%SystemRoot%\System32`, etc.) that declare `autoElevate="true"` in their
manifest will elevate **without a prompt**. A parallel allow-list of **auto-approved COM
interfaces** does the same for elevation-moniker instantiation.

This whitelist is what the well-known "UAC bypass" families abuse (hijacking the
environment, registry, or DLL search path of an auto-elevating binary such as the classic
disk-cleanup / event-viewer / `fodhelper` vectors). Third-party binaries cannot opt into
`autoElevate`; the loader ignores it for anything not Microsoft-signed and trusted-path.
Treat it as background knowledge, not a supported API.

### 6. The load-bearing caveat: elevation is not a strong boundary

Microsoft's official position is that **same-desktop UAC elevation is a convenience
feature, not a security boundary.** A Medium-IL process and a High-IL process sharing
`winsta0\default` are subject to UI-message ("shatter") and automation concerns, which is
why the consent UI runs on the *secure* desktop and why the **`uiAccess`** manifest flag
(which lifts UIPI restrictions for accessibility tools) is gated behind signing and a
trusted install path. Real privilege boundaries are **process/session isolation** (a
SYSTEM service) or **integrity + separate desktop**, not elevation alone. Design brokers
accordingly.

---

## Part II — The Methods

Each method is documented as: *mechanism → API/registry surface → minimal sketch →
result → failure modes → security caveats → best fit.*

---

### Method 0 — Baseline: `ShellExecuteEx` with `"runas"` (and its scriptable twins)

The reference point. Included here in full because the alternatives are best understood as
deviations from it.

**Mechanism.** Routes the launch through the shell → AIS → secure-desktop consent →
`CreateProcessAsUser` with the linked full token (Part I §4).

**API surface.**

```c
SHELLEXECUTEINFOW sei = { sizeof sei };
sei.fMask        = SEE_MASK_NOCLOSEPROCESS;   // populate hProcess for us to wait on
sei.lpVerb       = L"runas";                  // the elevation trigger
sei.lpFile       = L"C:\\path\\helper.exe";
sei.lpParameters = L"--do-admin-thing";
sei.nShow        = SW_SHOWNORMAL;
if (ShellExecuteExW(&sei)) {
    WaitForSingleObject(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);
}
```

**Scriptable equivalents** (all wrap the same shell path):

```powershell
Start-Process -FilePath helper.exe -ArgumentList '--do-admin-thing' -Verb RunAs
```

```cmd
:: via the shell COM object, no third-party tools
powershell -c "(New-Object -ComObject Shell.Application).ShellExecute('helper.exe','','','runas',1)"
```

**Result.** A separate **High-IL** process on the interactive desktop, one prompt per
launch.

**Failure modes.** `ShellExecuteEx` returns `FALSE` / `GetLastError()==ERROR_CANCELLED`
(`1223`) when the user declines. Note you get a *process handle*, not much else — no
handle inheritance, no environment injection, no token you can inspect pre-launch.

**Caveats.** You cannot inherit handles across the integrity boundary, and stdio
redirection to your Medium-IL pipes will be refused by the elevated child unless you
loosen ACLs deliberately. If you need a data channel, establish it *after* launch over a
properly-secured IPC endpoint.

**Best fit.** One-off, user-initiated administrative actions where a prompt is acceptable.

---

### Method 1 — The COM Elevation Moniker

Package the privileged code as a **COM class** instead of a standalone EXE, and let COM
instantiate it elevated inside a surrogate. The elevation boundary becomes an
**interface**, not a process launch.

**Mechanism.** `CoGetObject` with an `Elevation:Administrator!new:` moniker asks the COM
runtime to create the object in a **High-IL `dllhost.exe` surrogate**. AIS supplies the
consent prompt; thereafter your Medium-IL client calls methods across an RPC/marshalling
boundary.

**Registry surface.** The class must opt in and provide a display string for the prompt:

```
HKCR\CLSID\{Your-CLSID}\                       (@ = friendly name)
HKCR\CLSID\{Your-CLSID}\LocalServer32          (@ = path to your out-of-proc server)
HKCR\CLSID\{Your-CLSID}\Elevation
    Enabled       REG_DWORD = 1
    IconReference REG_SZ (optional, "@path,-resid")
HKCR\CLSID\{Your-CLSID}\  LocalizedString REG_SZ = "@path,-resid"   ; prompt caption
```

For clean cross-integrity marshalling, register the interface and its proxy/stub
(or use a type-library-marshaled `oleautomation`/`dual` interface) so the runtime can
build the proxy in the Medium-IL client.

**Minimal sketch.**

```c
BIND_OPTS3 bo = { sizeof bo };
bo.hwnd          = hwndParent;              // parents the consent UI
bo.dwClassContext = CLSCTX_LOCAL_SERVER;

IMyAdmin *p = NULL;
WCHAR mon[128];
StringCchPrintfW(mon, 128,
    L"Elevation:Administrator!new:{%s}", L"20000000-0000-0000-0000-000000000001");

HRESULT hr = CoGetObject(mon, (BIND_OPTS*)&bo, &IID_IMyAdmin, (void**)&p);
if (SUCCEEDED(hr)) {
    p->lpVtbl->DoPrivilegedThing(p, ...);   // executes at High IL in dllhost.exe
    p->lpVtbl->Release(p);
}
```

**Result.** A single consent prompt; the object lives at **High IL** for its lifetime,
callable repeatedly without re-prompting.

**Failure modes.** `CoGetObject` returns `E_ACCESSDENIED` when consent is declined,
`REGDB_E_CLASSNOTREG` (`0x80040154`) when the `Elevation\Enabled` key or LocalServer32 is
missing, and `CO_E_ELEVATION_DISABLED` when UAC/moniker elevation is off.

**Caveats.** The elevated object is now an attack surface reachable by any Medium-IL
process that knows the CLSID. **Validate every call**: in `CoInitializeSecurity` set an
appropriate authentication level, and on the server side check the caller's integrity /
identity (`CoImpersonateClient` → inspect the impersonation token) before performing
destructive work. Keep the elevated interface *narrow and verb-specific* — never expose a
generic "run this command" method.

**Best fit.** A standard-user app that needs to perform a small, fixed set of isolated
admin operations (a restricted registry write, service (re)configuration, driver install)
without elevating its own UI.

---

### Method 2 — Task Scheduler (`ITaskService`, `TASK_RUNLEVEL_HIGHEST`)

The Task Scheduler engine holds the privilege to assign the highest available token. If an
**administrator pre-registers** a task at highest run level, a **standard user can start
it silently** — the consent was banked at registration time.

**Mechanism.** Two phases:

1. **Register (once, elevated — e.g. during install):** define a task with a
   `Principal.RunLevel = TASK_RUNLEVEL_HIGHEST`, an on-demand (or event) trigger, and an
   `IExecAction`. Registration of a highest-run-level task requires elevation.
2. **Invoke (any time, standard user):** call `IRegisteredTask::Run` / `RunEx`, or shell
   out to `schtasks /Run /TN "\MyApp\Elevated"`. The engine launches the action at High IL
   with no prompt.

**Minimal sketch (registration).**

```c
ITaskService     *svc;  ITaskFolder *root;  ITaskDefinition *def;
IPrincipal       *prin; IActionCollection *acts; IExecAction *exec;
IRegisteredTask  *reg;   VARIANT v; VariantInit(&v);

CoCreateInstance(&CLSID_TaskScheduler, 0, CLSCTX_INPROC_SERVER,
                 &IID_ITaskService, (void**)&svc);
svc->lpVtbl->Connect(svc, v, v, v, v);                 // local, current user
svc->lpVtbl->GetFolder(svc, SysAllocString(L"\\"), &root);
svc->lpVtbl->NewTask(svc, 0, &def);

def->lpVtbl->get_Principal(def, &prin);
prin->lpVtbl->put_RunLevel(prin, TASK_RUNLEVEL_HIGHEST);
prin->lpVtbl->put_LogonType(prin, TASK_LOGON_INTERACTIVE_TOKEN);

def->lpVtbl->get_Actions(def, &acts);
acts->lpVtbl->Create(acts, TASK_ACTION_EXEC, (IAction**)&exec);
exec->lpVtbl->put_Path(exec, SysAllocString(L"C:\\MyApp\\worker.exe"));

VARIANT e; e.vt = VT_BSTR; e.bstrVal = SysAllocString(L"");   // empty creds → current user
root->lpVtbl->RegisterTaskDefinition(root, SysAllocString(L"MyApp\\Elevated"),
     def, TASK_CREATE_OR_UPDATE, v /*user*/, v /*pwd*/,
     TASK_LOGON_INTERACTIVE_TOKEN, e, &reg);
```

**Invoke (standard user):**

```cmd
schtasks /Run /TN "MyApp\Elevated"
```

**Result.** The worker launches **silently and elevated**. With
`TASK_LOGON_INTERACTIVE_TOKEN` it runs in the interactive user's session and desktop; with
a service-account principal it runs in session 0 with no visible UI.

**Failure modes.** `RegisterTaskDefinition` fails with `E_ACCESSDENIED` if the registrant
is not elevated. `IRegisteredTask::Run` returns `SCHED_E_...` HRESULTs; the task's own exit
is surfaced via `IRegisteredTask::get_LastTaskResult`.

**Caveats.** You have created a **standing, callable elevated capability**. Anyone who can
trigger the task inherits its power. Lock down: fix the action's path and arguments at
registration (never let the *caller* pass the command line), set a restrictive DACL on the
task, and consider a task that runs a *specific verb* rather than an arbitrary executable.
This exact pattern — start a pre-existing highest-run-level task from Medium IL — is also
the backbone of several documented UAC bypasses, which is the flip side of its
convenience.

**Best fit.** Background updaters, monitors, and helpers that must elevate **repeatedly
and silently** after a one-time elevated install.

---

### Method 3 — Elevated Broker Service (persistent IPC)

The canonical pattern for hardened applications (browsers, agents, management tooling).
Install a **Windows service running as `NT AUTHORITY\SYSTEM`**; the standard-user front end
requests privileged work over an authenticated IPC channel.

**Mechanism.** During elevated install, register a service with the SCM (`CreateService`,
`SERVICE_WIN32_OWN_PROCESS`, `LocalSystem`). At runtime the Medium-IL app connects over
**named pipes, RPC (ncalrpc/ALPC), or COM**; the service validates the request and either
performs the action itself or spawns a process with `CreateProcessAsUser` onto the user's
desktop.

**Spawning onto the interactive desktop from a SYSTEM service:**

```c
HANDLE hUser = NULL;
WTSQueryUserToken(dwSessionId, &hUser);          // full token of the logged-on user
STARTUPINFOW si = { sizeof si };
si.lpDesktop = (LPWSTR)L"winsta0\\default";      // must target the interactive desktop
PROCESS_INFORMATION pi;
CreateEnvironmentBlock(&env, hUser, FALSE);
CreateProcessAsUserW(hUser, path, cmd, 0, 0, FALSE,
     CREATE_UNICODE_ENVIRONMENT, env, 0, &si, &pi);   // SYSTEM holds SeAssignPrimaryToken
```

**Result.** Persistent, **silent** administrative capability with no per-action prompt.

**Failure modes.** `CreateProcessAsUser` → `ERROR_PRIVILEGE_NOT_HELD` (`1314`) if the
caller lacks `SeAssignPrimaryTokenPrivilege` (i.e., you tried it from a non-SYSTEM host);
wrong `lpDesktop` yields a process that runs but shows no window.

**Caveats — this is where security actually lives.** The IPC endpoint *is* the new trust
boundary. A weak one lets non-elevated malware drive a SYSTEM service — a full local
privilege escalation. Mandatory hardening:

* **Secure the endpoint's SD.** Named pipe: set a DACL granting only the intended
  principals; deny `NETWORK` connections (`FILE_PIPE_REJECT_REMOTE_CLIENTS`).
* **Authenticate the caller, don't trust the request.** Use
  `GetNamedPipeClientProcessId` → open the client, resolve its image path, and
  `WinVerifyTrust` the binary (confirm it's *your* signed front end). For RPC, use
  `RpcServerRegisterAuthInfo` + `RpcImpersonateClient` and inspect the token.
* **Whitelist verbs, validate every parameter.** Canonicalize paths, reject relative /
  UNC where unexpected, bound sizes. Never expose "run arbitrary command line."
* **Assume the caller is hostile** even after PID checks (PID reuse, injection). Prefer
  message-level intent over pass-through execution.

**Best fit.** Applications needing **robust, persistent, silent** admin/SYSTEM access with
a defensible security posture.

---

### Method 4 — Application Manifest (declarative, at the loader)

Bake the elevation requirement into the binary so the **loader** enforces it.

**Mechanism.** Embed an application manifest (resource type `RT_MANIFEST`, id `1`) with a
`requestedExecutionLevel`. When the OS loader sees `requireAdministrator` on a launch, it
routes through AIS for consent before the process image ever runs.

**Manifest fragment.**

```xml
<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
  <security>
    <requestedPrivileges>
      <!-- requireAdministrator | highestAvailable | asInvoker -->
      <requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>
    </requestedPrivileges>
  </security>
</trustInfo>
```

Levels: **`asInvoker`** (inherit caller's token, no elevation), **`highestAvailable`**
(admins get elevated, standard users run un-elevated *without failing*), and
**`requireAdministrator`** (always requires the full token).

**Result.** The executable *always* elevates when launched normally.

**Critical caveat — the `CreateProcess` trap.** Launching a `requireAdministrator` binary
from a Medium-IL process with `CreateProcess()` **fails** with
`ERROR_ELEVATION_REQUIRED` (`740`). `CreateProcess` never elevates — it is a raw token
inheritance API. The documented recovery is to fall back to the shell:

```c
STARTUPINFOW si = { sizeof si };  PROCESS_INFORMATION pi;
if (!CreateProcessW(path, cmd, 0,0,FALSE,0,0,0,&si,&pi)) {
    if (GetLastError() == ERROR_ELEVATION_REQUIRED) {
        SHELLEXECUTEINFOW sei = { sizeof sei };
        sei.lpVerb = L"runas"; sei.lpFile = path;
        sei.fMask  = SEE_MASK_NOCLOSEPROCESS; sei.nShow = SW_NORMAL;
        ShellExecuteExW(&sei);          // now AIS handles consent
    }
}
```

Which is precisely why this method, though "declarative," still lands you back on the
shell API for *launching* — its real value is guaranteeing that however the binary is
started (double-click, shortcut, another app), it cannot run un-elevated.

**Related knob — suppressing elevation.** The environment variable
`__COMPAT_LAYER=RunAsInvoker` forces a manifested binary to run *as the invoker* and skip
the prompt (it will simply fail its privileged operations if it truly needed rights). Useful
for automation where you want to run a normally-elevating tool at Medium IL deliberately —
and, again, a documented ingredient in some UAC-bypass chains.

**Best fit.** Installers, admin consoles, and tools that are *meaningless* un-elevated and
should never accidentally run at Medium IL.

---

### Method 5 — Token Manipulation (`CreateProcessWithTokenW` / `CreateProcessAsUser`)

For callers that **already hold high privilege** — services, admin tooling — not standard
user apps. This is "duplicate a powerful token and spawn under it."

**Mechanism.** Open a token from an already-privileged process, duplicate it to a primary
token, and hand it to a process-creation API that can assign it.

```c
HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pidOfSystemProc);
HANDLE hTok, hDup;
OpenProcessToken(hProc, TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &hTok);
DuplicateTokenEx(hTok, MAXIMUM_ALLOWED, NULL,
                 SecurityImpersonation, TokenPrimary, &hDup);

STARTUPINFOW si = { sizeof si };  PROCESS_INFORMATION pi;
si.lpDesktop = (LPWSTR)L"winsta0\\default";
CreateProcessWithTokenW(hDup, LOGON_WITH_PROFILE, path, cmd,
                        CREATE_UNICODE_ENVIRONMENT, env, cwd, &si, &pi);
```

**Privilege requirements — the gate.**

| API                         | Required privilege(s)                                   | Runs via        |
|-----------------------------|---------------------------------------------------------|-----------------|
| `CreateProcessWithTokenW`   | `SeImpersonatePrivilege` (`SE_IMPERSONATE_NAME`)        | seclogon service|
| `CreateProcessAsUser`       | `SeAssignPrimaryTokenPrivilege` + `SeIncreaseQuotaPrivilege` | direct     |
| Opening arbitrary process tokens | `SeDebugPrivilege` to reach protected/foreign procs | —               |

**Result.** A process running under the duplicated token's identity — up to SYSTEM if you
duplicated a SYSTEM token.

**Failure modes.** `ERROR_PRIVILEGE_NOT_HELD` (`1314`) when the required privilege is
absent or not *enabled* in your token (remember: privileges must be enabled via
`AdjustTokenPrivileges`, not merely present). `ERROR_ACCESS_DENIED` opening a protected
(PPL) process without adequate rights — `SeDebugPrivilege` does **not** pierce PPL.

**Caveats.** This is the classic `getsystem` primitive. It is legitimate for services that
spawn interactive user processes, and it is a red-team lateral-movement staple; expect EDR
scrutiny on `OpenProcessToken` against `lsass.exe`/`winlogon.exe`. Never use it to work
around UAC from a standard-user context — a Medium-IL token holds none of the gating
privileges, so it simply cannot.

**Best fit.** SYSTEM services launching user-context processes; controlled administrative
tooling; security research.

---

### Method 6 — Alternate Credentials (`CreateProcessWithLogonW` / `runas.exe`)

Distinct from elevating *your* identity: this runs code as a **different** user by supplying
explicit credentials — the "Run as different user" experience.

**Mechanism.** `CreateProcessWithLogonW` performs an interactive-style logon for the named
account (via the **Secondary Logon service**, `seclogon`) and creates the process under that
account's token. No `SeAssignPrimaryTokenPrivilege` needed on the caller — seclogon does the
heavy lifting.

```c
CreateProcessWithLogonW(
    L"AdminUser", L".", L"P@ssw0rd",
    LOGON_WITH_PROFILE,
    L"C:\\Windows\\System32\\cmd.exe", NULL,
    CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
```

Command-line equivalent (`runas.exe` wraps this API):

```cmd
runas /user:Administrator /profile "cmd.exe"
```

**Result / caveats.** If the target account is an administrator, the resulting process may
still receive the *filtered* token under UAC unless the target itself elevates — supplying
admin credentials is authentication, not automatic High IL. Handling plaintext credentials
is a liability; prefer `CredUIPromptForWindowsCredentials` / the DPAPI-protected credential
store over embedding secrets. `runas` does not accept a piped password by design.

**Best fit.** Administrative jump-boxes and tooling that must act as a *specific service or
admin account* rather than elevate the current user.

---

## Part III — Comparison Matrix

| # | Method                    | Prompt?        | Silent after setup | Elevation ceiling | Caller must already hold | New trust boundary        |
|---|---------------------------|----------------|--------------------|-------------------|--------------------------|---------------------------|
| 0 | `ShellExecuteEx "runas"`  | Yes, per launch| No                 | High (linked tok) | Nothing                  | Process (separate desktop for consent) |
| 1 | COM Elevation Moniker     | Yes, per object| Object reused      | High              | Nothing                  | COM interface             |
| 2 | Task Scheduler HIGHEST     | No*            | **Yes**            | High / SYSTEM     | Elevated at *register*   | Task ACL + fixed action   |
| 3 | Broker Service (IPC)      | No*            | **Yes**            | **SYSTEM**        | Elevated at *install*    | IPC endpoint SD + verb validation |
| 4 | Manifest `requireAdmin`   | Yes (via AIS)  | No                 | High              | Nothing (but must ShellExecute) | Loader-enforced   |
| 5 | Token duplication         | No             | Yes                | up to SYSTEM      | `SeImpersonate` / `SeAssignPrimaryToken` | Token possession |
| 6 | `CreateProcessWithLogonW` | Credential entry | If creds stored  | Target account    | Valid credentials        | Credential secrecy        |

\* Prompt paid once, at the elevated install/registration step.

**How to choose, quickly:**

* One user-initiated admin action, prompt acceptable → **0** (or **4** if the tool must
  never run un-elevated).
* Small fixed set of isolated admin verbs from a standard-user app → **1**.
* Repeated *silent* elevation after a one-time install, modest hardening → **2**.
* Persistent SYSTEM capability, serious hardening budget → **3**.
* You are already a service / hold the privileges → **5**.
* Must run as a *different* account → **6**.

---

## Part IV — Security Checklist for Silent-Elevation Designs (Methods 2 & 3)

The silent methods trade a prompt for a **standing capability**. That capability is only as
safe as its gate:

1. **Fix the payload at setup time.** The action's executable path *and* arguments are
   defined by the administrator during install/registration — never derived from the
   standard-user caller's input.
2. **Authenticate the caller cryptographically, not by trust-on-first-use.** Resolve the
   caller's image and `WinVerifyTrust` it; confirm publisher and, ideally, an expected
   file identity — not merely "a process connected."
3. **Restrict the endpoint.** Named-pipe/RPC SD limits principals; reject remote clients;
   least-privilege the service account where SYSTEM is not strictly required.
4. **Validate every parameter as hostile input.** Canonicalize and bound all paths,
   handles, and buffers; reject the unexpected rather than sanitizing it.
5. **Expose intent, not mechanism.** "Apply update package X (signed)" beats "execute this
   command line." A generic executor is a privilege-escalation gift.
6. **Log and rate-limit.** Elevated brokers are prime lateral-movement targets; make abuse
   observable.

---

## Part V — Quick Reference

**Key error codes.**

| Code   | Symbol                     | Meaning / trigger                                        |
|--------|----------------------------|----------------------------------------------------------|
| `740`  | `ERROR_ELEVATION_REQUIRED` | `CreateProcess` on a `requireAdministrator` binary       |
| `1223` | `ERROR_CANCELLED`          | User declined the UAC consent prompt                     |
| `1314` | `ERROR_PRIVILEGE_NOT_HELD` | Required privilege absent or not enabled in caller token |
| `5`    | `ERROR_ACCESS_DENIED`      | Integrity/DACL denial; opening a protected process       |
| `0x80040154` | `REGDB_E_CLASSNOTREG` | COM moniker: class or `Elevation\Enabled` unregistered  |

**Privileges that gate the token APIs.**

| Privilege                        | Held by (typically)         | Enables                                    |
|----------------------------------|-----------------------------|--------------------------------------------|
| `SeAssignPrimaryTokenPrivilege`  | SYSTEM, service accounts    | `CreateProcessAsUser`                       |
| `SeImpersonatePrivilege`         | services, `LOCAL SERVICE`+  | `CreateProcessWithTokenW`, impersonation   |
| `SeIncreaseQuotaPrivilege`       | admins, SYSTEM              | assigning primary tokens (companion)       |
| `SeDebugPrivilege`               | elevated admins             | opening tokens of foreign processes        |

**Token-introspection calls worth memorizing.**

* `GetTokenInformation(TokenElevationType)` — Default / Full / Limited.
* `GetTokenInformation(TokenElevation)` — boolean "am I elevated?".
* `GetTokenInformation(TokenIntegrityLevel)` — the label SID; read the last RID.
* `GetTokenInformation(TokenLinkedToken)` — the *other* half of the split token
  (informational — cannot be assigned without privilege).

---

## References (by name — consult current MSDN/Learn for signatures)

* *ShellExecuteEx*, `SHELLEXECUTEINFO` — shell launch and the `"runas"` verb.
* *The COM Elevation Moniker* — `CoGetObject`, `BIND_OPTS3`, the `Elevation:Administrator!new:` syntax and CLSID registration.
* *Task Scheduler 2.0* — `ITaskService`, `IPrincipal::RunLevel`, `TASK_RUNLEVEL_HIGHEST`, `IRegisteredTask::Run`.
* *Service Control Manager* — `CreateService`, service account contexts; `WTSQueryUserToken`, `CreateProcessAsUser`, `CreateEnvironmentBlock`.
* *Application Manifests* — `requestedExecutionLevel`, `uiAccess`, `autoElevate` (Microsoft-signed only).
* *Access Tokens & Privileges* — `OpenProcessToken`, `DuplicateTokenEx`, `AdjustTokenPrivileges`, `CreateProcessWithTokenW`, `CreateProcessWithLogonW`.
* *Windows Integrity Mechanism* — mandatory labels, integrity levels, no-write-up policy.
* *User Account Control* — the split-token model, AIS consent flow, and the standing guidance that same-desktop elevation is not a security boundary.
