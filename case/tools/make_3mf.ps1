# Erzeugt die beiden Roboter-3MF aus den STL-Dateien und prueft sie. Exitcode 0 = ok, 1 = Fehler.
# Aufruf (PowerShell):  .\case\tools\make_3mf.ps1 -StlDir case\stl -OutDir case\3mf [-ExpectGroupComponents 5]
# Wird von check_roboter.ps1 aufgerufen. -ExpectGroupComponents ist nur fuer die Gegenprobe da (falscher Wert muss FAIL liefern).
param(
  [Parameter(Mandatory)][string]$StlDir,
  [Parameter(Mandatory)][string]$OutDir,
  [int]$ExpectGroupComponents = 5
)
$ErrorActionPreference = 'Stop'
$py  = 'C:\Users\madma\AppData\Local\Python\pythoncore-3.14-64\python.exe'
$tool = Join-Path $PSScriptRoot 'stl2_3mf.py'
New-Item -ItemType Directory -Force $OutDir | Out-Null
$fails = 0
function Ok($m)  { Write-Host "[ OK ] $m" -ForegroundColor Green }
function Bad($m) { Write-Host "[FAIL] $m" -ForegroundColor Red; $script:fails++ }
function S([string]$n) { Join-Path $StlDir "roboter_$n.stl" }

# Einzelteile mit Menge. Name=Datei; Arm links ist die gespiegelte Datei roboter_arm_l.stl (-D side=-1).
$single = @(
  @('head_back', 'head_back'), @('ear_r', 'ear'), @('ear_l', 'ear'), @('deco', 'deco'),
  @('torso_front', 'torso_front'), @('torso_back', 'torso_back'), @('neck', 'neck'),
  @('arm_r', 'arm'), @('arm_l', 'arm_l'), @('hand_r', 'hand'), @('hand_l', 'hand'),
  @('seat', 'seat'), @('bottom', 'bottom'), @('foot_r', 'foot'), @('foot_l', 'foot'), @('cam_test', 'cam_test')
) | ForEach-Object { "{0}={1}" -f $_[0], (S $_[1]) }

function Build([string]$out, [string[]]$specs, [int]$expItems, [int]$expComp) {
  if (Test-Path $out) { Remove-Item $out }
  $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
  $log = (& $py -I $tool $out --arrange '250,10' @specs 2>&1 | Out-String)
  $ErrorActionPreference = $old
  if ($LASTEXITCODE -ne 0) { Bad "$(Split-Path $out -Leaf): stl2_3mf.py Exitcode $LASTEXITCODE $log"; return }
  $len = (Get-Item $out).Length
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $z = [IO.Compression.ZipFile]::OpenRead($out)
  try { $sr = New-Object IO.StreamReader($z.GetEntry('3D/3dmodel.model').Open()); $x = $sr.ReadToEnd(); $sr.Close() } finally { $z.Dispose() }
  $items = [regex]::Matches($x, '<item ').Count
  $comp  = [regex]::Matches($x, '<component ').Count
  $name  = Split-Path $out -Leaf
  if ($len -le 10240)           { Bad "$name : nur $len Byte (verlangt > 10 kB)" }
  elseif ($items -ne $expItems) { Bad "$name : $items <item>, erwartet $expItems" }
  elseif ($comp -ne $expComp)   { Bad "$name : $comp <component>, erwartet $expComp" }
  else { Ok "$name : $len Byte, $items Bauobjekte, $comp Bauteile in Gruppen" }
}

foreach ($f in @('head_front', 'bezel', 'head_back', 'ear', 'deco', 'torso_front', 'torso_back', 'neck', 'arm', 'arm_l', 'hand', 'seat', 'bottom', 'foot', 'cam_test', 'head_front_ams', 'bezel_inlay', 'screws_inlay', 'spk_grille', 'spk_grille_inlay')) {
  if (-not (Test-Path (S $f))) { Bad "3MF: Eingabe fehlt: $(S $f)" }
}
if ($fails -gt 0) { exit 1 }

# 1) Alle Einzelteile nebeneinander (inkl. Lautsprecher-Gitter): 19 Objekte, keine Gruppen
Build (Join-Path $OutDir 'roboter_teile.3mf') (@("head_front=$(S 'head_front')", "bezel=$(S 'bezel')", "spk_grille=$(S 'spk_grille')") + $single) 19 0
# 2) AMS: Gruppe Kopf (Kopfschale + Blende + 4 Zierschrauben), Gruppe Brust (torso_front + Lautsprecher-Gitter als eigenes Bauteil/Farbe) und die uebrigen 15 Teile einzeln
$ams = @('--group', 'kopf', "head_front=$(S 'head_front_ams')", "bezel=$(S 'bezel_inlay')", "screws=$(S 'screws_inlay')", '--end', '--group', 'brust', "torso_front=$(S 'torso_front')", "spk_grille=$(S 'spk_grille_inlay')", '--end') + ($single | Where-Object { $_ -notlike 'torso_front=*' })
Build (Join-Path $OutDir 'roboter_ams.3mf') $ams 17 $ExpectGroupComponents

if ($fails -gt 0) { exit 1 }
exit 0
