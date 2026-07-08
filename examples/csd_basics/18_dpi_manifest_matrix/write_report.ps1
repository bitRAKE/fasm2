$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$variants = @("unaware", "system", "pmv1", "pmv2")

function Clean-Cell {
    param([string]$Value)
    if ($null -eq $Value) { return "" }
    return ($Value -replace "[`t`r`n]+", " ").Trim()
}

function Read-DpiField {
    param(
        [string]$Xml,
        [string]$Name
    )
    $match = [regex]::Match($Xml, "<$Name\b[^>]*>(.*?)</$Name>", "IgnoreCase,Singleline")
    if ($match.Success) { return (Clean-Cell $match.Groups[1].Value) }
    return ""
}

function Read-RuntimeSummary {
    param([string]$Variant)

    $path = Join-Path $root "reports\runtime_$Variant.tsv"
    $summary = [ordered]@{
        startup_dpi = ""
        wm_dpichanged = "not_run"
        suggested_rect_applied = ""
        note = "runtime log not present"
    }

    if (-not (Test-Path $path)) { return $summary }

    $rows = Import-Csv -Delimiter "`t" -Path $path
    $create = $rows | Where-Object { $_.event -eq "create" } | Select-Object -First 1
    if ($create) { $summary.startup_dpi = Clean-Cell $create.dpi }

    $dpiRows = @($rows | Where-Object { $_.event -eq "WM_DPICHANGED" })
    if ($dpiRows.Count -gt 0) {
        $summary.wm_dpichanged = "yes"
        if ($dpiRows | Where-Object { $_.suggested_rect_applied -eq "1" }) {
            $summary.suggested_rect_applied = "yes"
        } else {
            $summary.suggested_rect_applied = "no"
        }
    } else {
        $summary.wm_dpichanged = "no"
        $summary.suggested_rect_applied = "no"
    }
    $summary.note = "runtime log captured"
    return $summary
}

$reportPath = Join-Path $root "reports\matrix.tsv"
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("variant`tmanifest_dpiAware`tmanifest_dpiAwareness`tsubsystem`tstartup_dpi`twm_dpichanged`tsuggested_rect_applied`tnotes")

foreach ($variant in $variants) {
    $sourceManifest = Join-Path $root "manifests\$variant.manifest"
    $extractedManifest = Join-Path $root "extracted\18_$variant.manifest"
    $manifestPath = if (Test-Path $extractedManifest) { $extractedManifest } else { $sourceManifest }
    $xml = Get-Content -Raw -Path $manifestPath

    $dpiAware = Read-DpiField $xml "dpiAware"
    $dpiAwareness = Read-DpiField $xml "dpiAwareness"

    $dumpbinPath = Join-Path $root "extracted\18_$variant.dumpbin.txt"
    $subsystem = ""
    $notes = New-Object System.Collections.Generic.List[string]
    if (Test-Path $extractedManifest) {
        $notes.Add("mt extracted manifest")
    } else {
        $notes.Add("mt unavailable; source manifest used")
    }

    if (Test-Path $dumpbinPath) {
        $subsystemLines = Get-Content $dumpbinPath |
            Where-Object { $_ -match "(?i)subsystem" } |
            ForEach-Object { Clean-Cell $_ }
        $subsystem = Clean-Cell (($subsystemLines | Select-Object -First 3) -join "; ")
        $notes.Add("dumpbin headers captured")
    } else {
        $notes.Add("dumpbin skipped")
    }

    $runtime = Read-RuntimeSummary $variant
    $notes.Add($runtime.note)

    $cells = @(
        $variant,
        $dpiAware,
        $dpiAwareness,
        $subsystem,
        $runtime.startup_dpi,
        $runtime.wm_dpichanged,
        $runtime.suggested_rect_applied,
        (($notes | ForEach-Object { Clean-Cell $_ }) -join "; ")
    )
    $lines.Add(($cells | ForEach-Object { Clean-Cell $_ }) -join "`t")
}

Set-Content -Encoding ASCII -Path $reportPath -Value $lines
Write-Host "wrote reports\matrix.tsv"
