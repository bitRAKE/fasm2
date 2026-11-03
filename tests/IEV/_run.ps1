# _run.ps1 -- run every IEV test with this tree's fasm2.
#
#	powershell -File tests\IEV\_run.ps1 [-v]
#
# pass\ holds byte references (instruction ; HEX), checked with iev.alm;
# fail\ holds error references (instruction ; 'message'), checked with
# fail.alm. The work is in _run.py (it maps fasmg's error reports to lines).

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
python (Join-Path $here '_run.py') @args
exit $LASTEXITCODE
