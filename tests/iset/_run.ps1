# _run.ps1 -- instruction-set regression tests, see readme.md
#
#   powershell -ExecutionPolicy Bypass -File tests\iset\_run.ps1 [name ...]
#   ... _run.ps1 -Root <other fasm2 tree> [name ...]
#
# -Root runs the same tests against another checkout (its fasmg.exe and
# include\); use it to confirm a test fails on the tree before its fix.
#
# For every *.asm here (or only the named ones):
#   1. assemble it as is; it must succeed (its byte checks run inside fasmg);
#   2. for every "; FAIL n: 'message'" line in its header, assemble it again
#      with -i"FAIL=n"; it must fail and the output must contain the message.
# Exit code is the number of failed checks.

[CmdletBinding(PositionalBinding = $false)]
param([string]$Root, [Parameter(ValueFromRemainingArguments)][string[]]$Name)

$here = $PSScriptRoot
$root = if ($Root) { Resolve-Path $Root } else { Resolve-Path "$here\..\.." }
# fasmg is called the way fasm2.cmd calls it; the batch file does not pass
# fasmg's exit code through, and the failure cases depend on it
$fasmg = Join-Path $root 'fasmg.exe'
$env:INCLUDE = "$root\include;$env:INCLUDE"
$out = Join-Path ([IO.Path]::GetTempPath()) 'fasm2-iset-tests'
New-Item -ItemType Directory -Force $out | Out-Null

$files = Get-ChildItem "$here\*.asm" | Where-Object { -not $Name -or $Name -contains $_.BaseName }
$failed = 0

function Assemble([string]$src, [string[]]$extra) {
	$bin = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($src) + '.bin')
	$text = & $fasmg "-iInclude('fasm2.inc')" @extra $src $bin 2>&1 | ForEach-Object { "$_" } | Out-String
	[pscustomobject]@{ Ok = ($LASTEXITCODE -eq 0); Text = $text }
}

foreach ($f in $files) {
	$r = Assemble $f.FullName @()
	if ($r.Ok) {
		"[pass] $($f.BaseName)"
	} else {
		"[FAIL] $($f.BaseName): assembly failed"
		$r.Text.TrimEnd() -split "`n" | ForEach-Object { "         $_" }
		$failed++
	}
	foreach ($m in Select-String -Path $f.FullName -Pattern "^;\s*FAIL\s+(\d+):\s*'([^']*)'") {
		$n = $m.Matches[0].Groups[1].Value
		$want = $m.Matches[0].Groups[2].Value
		$r = Assemble $f.FullName @("-iFAIL=$n")
		if (-not $r.Ok -and $r.Text.Contains($want)) {
			"[pass] $($f.BaseName) FAIL $n  ($want)"
		} else {
			$why = if ($r.Ok) { 'assembled, but must fail' } else { 'wrong error' }
			"[FAIL] $($f.BaseName) FAIL $n  expected '$want': $why"
			$r.Text.TrimEnd() -split "`n" | Select-Object -Last 3 | ForEach-Object { "         $_" }
			$failed++
		}
	}
}

if ($failed) { "$failed check(s) failed" } else { 'all checks passed' }
exit $failed
