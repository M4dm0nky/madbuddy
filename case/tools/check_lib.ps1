# Gemeinsame Hilfsfunktionen fuer check.ps1 und check_roboter.ps1 (per Dot-Source eingebunden: . (Join-Path $here 'tools\check_lib.ps1')).
# Erwartet im aufrufenden Skript: $Define, $scad, $oscad und $script:fails.
function Ok($m)   { Write-Host "[ OK ] $m" -ForegroundColor Green }
function Bad($m)  { Write-Host "[FAIL] $m" -ForegroundColor Red; $script:fails++ }
function Note($m) { Write-Host "[ -- ] $m" -ForegroundColor Yellow }

# OpenSCAD aufrufen (mit Zeitlimit), liefert @{Code; Text}
function Run-Scad([string[]]$ScadArgs, [int]$TimeoutSec = 600) {
  $argList = @()
  foreach ($d in $Define) { $argList += '-D'; $argList += $d }
  $argList += $ScadArgs
  $argList += $scad
  $errFile = [IO.Path]::GetTempFileName(); $outFile = [IO.Path]::GetTempFileName()
  $p = Start-Process -FilePath $oscad -ArgumentList $argList -NoNewWindow -PassThru `
        -RedirectStandardError $errFile -RedirectStandardOutput $outFile
  $null = $p.Handle   # Handle halten, sonst ist ExitCode leer
  if (-not $p.WaitForExit($TimeoutSec * 1000)) {
    try { & taskkill.exe /PID $p.Id /T /F 2>$null | Out-Null } catch {}
    try { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } catch {}
    return @{ Code = -999; Text = "TIMEOUT nach $TimeoutSec s" }
  }
  $p.WaitForExit()
  $txt = (Get-Content $errFile -Raw) + (Get-Content $outFile -Raw)
  Remove-Item $errFile, $outFile -ErrorAction SilentlyContinue
  return @{ Code = $p.ExitCode; Text = $txt }
}
function PartArg([string]$name) { return @('-D', ('part=\"{0}\"' -f $name)) }
function FacetCount([string]$file) {
  if (-not (Test-Path $file)) { return 0 }
  return ([regex]::Matches((Get-Content $file -Raw), 'facet normal')).Count
}
