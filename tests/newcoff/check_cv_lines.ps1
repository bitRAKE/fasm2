param(
    [Parameter(Mandatory = $true)]
    [string] $DumpPath
)

$ErrorActionPreference = 'Stop'
$dump = Get-Content -LiteralPath $DumpPath
$entries = @()
$currentFile = ''
$currentSection = ''

foreach ($text in $dump) {
    if ($text -match '^(.+?) \((?:SHA-256|None):') {
        $currentFile = [IO.Path]::GetFileName($Matches[1].Trim())
        continue
    }
    if ($text -match '^\s*([0-9A-Fa-f]{4}):[0-9A-Fa-f]{8}-[0-9A-Fa-f]{8}, line/addr entries') {
        $currentSection = $Matches[1].ToUpperInvariant()
        continue
    }
    foreach ($match in [regex]::Matches($text, '(?<![:0-9A-Fa-f-])(\d+)\s+([0-9A-Fa-f]{8})(?![-0-9A-Fa-f])')) {
        $entries += [pscustomobject]@{
            File = $currentFile
            Section = $currentSection
            Line = [int] $match.Groups[1].Value
            Offset = $match.Groups[2].Value.ToUpperInvariant()
        }
    }
}

if (-not ($entries | Where-Object File -eq 'cv_lines.inc')) {
    throw 'included source cv_lines.inc has no PDB line entry'
}

$duplicates = $entries | Group-Object Section, Offset | Where-Object Count -gt 1
if ($duplicates) {
    throw "duplicate PDB line offsets remain: $($duplicates.Name -join ', ')"
}

$source = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'cv.asm')
$entryLine = 1 + [array]::FindIndex([string[]] $source, [Predicate[string]] { param($line) $line -match '^\s*sub\s+rsp,\s*40\s*$' })
if ($entryLine -le 0 -or -not ($entries | Where-Object { $_.File -eq 'cv.asm' -and $_.Section -eq '0001' -and $_.Offset -eq '00000000' -and $_.Line -eq $entryLine })) {
    throw 'entry RVA is not mapped to the byte-emitting sub rsp, 40 line'
}

$included = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'cv_lines.inc')
$nopLine = 1 + [array]::FindIndex([string[]] $included, [Predicate[string]] { param($line) $line -match '^\s*nop\s*$' })
if ($nopLine -le 0 -or -not ($entries | Where-Object { $_.File -eq 'cv_lines.inc' -and $_.Line -eq $nopLine })) {
    throw 'included nop is not mapped to its source line'
}

Write-Output "[ok]   cv lines are byte-backed, coalesced, and include contributing sources"
