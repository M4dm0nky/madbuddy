# Baut die XIAO-Firmware (cam/) ausserhalb von Dropbox.
# Grund: Dropbox macht die vom ESP-IDF-Komponentenmanager geladenen Ordner sofort zu Cloud-Platzhaltern und sperrt sie;
# der Build bricht dann mit WinError 32 ab. Deshalb werden cam/ und src/logic/ nach %LOCALAPPDATA%\3zoll-cam gespiegelt
# und dort gebaut. Erzeugte Dateien (managed_components, .pio, sdkconfig.cam, dependencies.lock) bleiben dort liegen.
#
# Aufruf:  & tools\cam-build.ps1                      # nur bauen
#          & tools\cam-build.ps1 -Upload -Port COM7   # bauen und flashen
# Ergebnis: %LOCALAPPDATA%\3zoll-cam\cam\.pio\build\cam\firmware.bin
param(
  [switch]$Upload,
  [string]$Port = ""
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$work = Join-Path $env:LOCALAPPDATA "3zoll-cam"
$pio = "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe"

# Spiegeln; erzeugte Dateien weder kopieren noch im Ziel loeschen
robocopy (Join-Path $repo "cam") (Join-Path $work "cam") /MIR /NFL /NDL /NJH /NJS /NP `
  /XD managed_components .pio /XF dependencies.lock sdkconfig.cam CMakeLists.txt | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy cam fehlgeschlagen ($LASTEXITCODE)" }
robocopy (Join-Path $repo "src\logic") (Join-Path $work "src\logic") /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy src/logic fehlgeschlagen ($LASTEXITCODE)" }

# sdkconfig.cam wird nur einmal aus sdkconfig.defaults erzeugt; ist defaults neuer (z. B. Version), neu erzeugen lassen
$cfg = Join-Path $work "cam\sdkconfig.cam"
$def = Join-Path $work "cam\sdkconfig.defaults"
if ((Test-Path $cfg) -and (Get-Item $def).LastWriteTime -gt (Get-Item $cfg).LastWriteTime) {
  Remove-Item $cfg
  Write-Host "sdkconfig.defaults geaendert: sdkconfig.cam wird neu erzeugt"
}

$pioArgs = @("run", "-d", (Join-Path $work "cam"))
if ($Upload) {
  $pioArgs += @("-t", "upload")
  if ($Port) { $pioArgs += @("--upload-port", $Port) }
}
$ErrorActionPreference = "Continue"   # pio schreibt Statuszeilen nach stderr (PS 5.1 werteten sie sonst als Fehler)
& $pio @pioArgs
exit $LASTEXITCODE
