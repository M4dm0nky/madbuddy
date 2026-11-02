# MadBuddy Roboter: alle Pruefungen. Exitcode 0 = alles gruen, 1 = mindestens eine Pruefung fehlgeschlagen.
# Aufruf (im PowerShell-Tool):  .\case\check_roboter.ps1 [-Define window_shrink=2] [-NoPng] [-Only '1','3','3b','3i','3j']
param(
  [string[]]$Define = @(),   # zusaetzliche -D name=wert (nur Zahlen/Boolsche), z. B. window_shrink=2 fuer die Gegenprobe
  [switch]$NoPng,
  [string[]]$Only = @()   # nur diese Abschnitte, z. B. -Only '1','3','3b','3i','3j' (3j braucht die STL aus Abschnitt 1); ohne Angabe laufen alle
)
$ErrorActionPreference = 'Stop'
$here   = Split-Path -Parent $MyInvocation.MyCommand.Path
$scad   = Join-Path $here 'madbuddy_roboter.scad'
$oscad  = 'C:\Program Files\OpenSCAD\openscad.com'
$custom = $Define.Count -gt 0
$stlDir = if ($custom) { Join-Path $env:TEMP 'madbuddy_roboter_check_stl' } else { Join-Path $here 'stl' }
$pngDir = Join-Path $here 'render'
New-Item -ItemType Directory -Force $stlDir, $pngDir | Out-Null
$script:fails = 0

# Druckteile. Spaetere Tasks haengen ihre Teile hier an (Task 3: neck, torso_front, torso_back, arm, hand; Task 4: seat, bottom, foot, jetzt enthalten).
$parts = @("head_front", "bezel", "head_back", "ear", "deco", "torso_front", "torso_back", "neck", "arm", "hand", "seat", "bottom", "foot", "cam_test")

function Want([string]$k) { return ($Only.Count -eq 0 -or $Only -contains $k) }
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
$emptyMsg = 'Current top level object is empty'
$inv = [Globalization.CultureInfo]::InvariantCulture   # einmal fuer alle Abschnitte (Zahlen immer mit Punkt lesen)
# Is-EmptyRun: ein Lauf zaehlt nur dann als "leer", wenn er wirklich ausgewertet wurde (Exitcode 0 oder Leer-Meldung). Timeout (-999) oder Absturz ohne Text ist nie "leer".
function Is-EmptyRun($r, [int]$n) { return ($n -eq 0 -and ($r.Text -match $emptyMsg -or $r.Code -eq 0)) }

# -Only: nur bekannte Abschnittsnamen zulassen (Tippfehler duerfen nicht still "alles gruen" ergeben)
$knownSections = @('1', '1b', '1c', '2', '2b', '3', '3b', '3c', '3d', '3e', '3e2', '3g', '3f', '3h', '3i', '3j', '4')
$unknown = @($Only | Where-Object { $knownSections -notcontains $_ })
if ($unknown.Count -gt 0) {
  Write-Host "[FAIL] -Only: unbekannter Abschnitt: $($unknown -join ', '). Gueltig: $($knownSections -join ', ')" -ForegroundColor Red
  exit 1
}

Write-Host "== madbuddy_roboter check ==  Define: $($Define -join ' ')"

# 1) STL-Export jedes Teils ohne Fehler/Warnungen
if (Want '1') {
foreach ($p in $parts) {
  $out = Join-Path $stlDir "roboter_$p.stl"
  if (Test-Path $out) { Remove-Item $out }
  $r = Run-Scad ((PartArg $p) + @('-o', $out))
  $n = FacetCount $out
  if ($r.Code -ne 0)                { Bad "STL $p : Exitcode $($r.Code) $($r.Text.Trim())" }
  elseif ($r.Text -match 'WARNING') { Bad "STL $p : WARNING in Ausgabe: $(($r.Text -split "`n" | Select-String WARNING | Select-Object -First 3) -join ' | ')" }
  elseif ($r.Text -match 'ERROR')   { Bad "STL $p : ERROR in Ausgabe" }
  elseif ($n -lt 100) { Bad "STL $p : zu wenige Facetten ($n)" }
  else                              { Ok "STL $p ($n Facetten)" }
}
}

# 1b) Zusatz-STL fuer die 3MF: AMS-Kopf (Schale, Blende, Zierschrauben in gleicher Lage) und linker Arm (gespiegelt, -D side=-1)
if (Want '1b') {
foreach ($e in @(@{ n = 'head_front_ams'; part = 'head_front_ams'; d = @() }, @{ n = 'bezel_inlay'; part = 'bezel_inlay'; d = @() }, @{ n = 'screws_inlay'; part = 'screws_inlay'; d = @() }, @{ n = 'arm_l'; part = 'arm'; d = @('-D', 'side=-1') })) {
  $out = Join-Path $stlDir "roboter_$($e.n).stl"
  if (Test-Path $out) { Remove-Item $out }
  $r = Run-Scad ($e.d + (PartArg $e.part) + @('-o', $out))
  $n = FacetCount $out
  if ($r.Code -ne 0)                { Bad "STL $($e.n) : Exitcode $($r.Code) $($r.Text.Trim())" }
  elseif ($r.Text -match 'WARNING|ERROR') { Bad "STL $($e.n) : WARNING/ERROR in Ausgabe" }
  elseif ($n -lt 100)               { Bad "STL $($e.n) : zu wenige Facetten ($n)" }
  else                              { Ok "STL $($e.n) ($n Facetten)" }
}
}

# 1c) 3MF (Einzelteile + AMS) aus den STL erzeugen und pruefen (Zip lesbar, > 10 kB, Anzahl Objekte und Bauteile)
if (Want '1c') {
if (-not $custom) {
  & (Join-Path $here 'tools\make_3mf.ps1') -StlDir $stlDir -OutDir (Join-Path $here '3mf')
  if ($LASTEXITCODE -ne 0) { Bad "3MF: make_3mf.ps1 meldet Fehler" }
}
}

# 2) Standsicherheit: nur, wenn das SCAD eine Sitzflaeche (seat_poly) kennt
if (Want '2') {
$r = Run-Scad ((PartArg 'info') + @('-o', (Join-Path $env:TEMP 'madbuddy_roboter_info.stl')))
if ($r.Text -match 'WARNING|ERROR') { Bad "Info: WARNING/ERROR: $($r.Text.Trim())" }
if ($r.Text -match 'STAND_MIN_MARGIN=([-0-9.e]+) STAND_MIN=([-0-9.e]+)') {
  $m = [double]::Parse($Matches[1], $inv); $mn = [double]::Parse($Matches[2], $inv)
  if ($m -ge $mn) { Ok ("Stand: Lot des Schwerpunkts {0:N1} mm innerhalb der Sitzflaeche (verlangt >= {1})" -f $m, $mn) }
  else { Bad ("Stand: Lot des Schwerpunkts nur {0:N1} mm innerhalb der Sitzflaeche, verlangt >= {1}" -f $m, $mn) }
} elseif ($r.Text -match 'STAND_SKIPPED') {
  Bad "Stand: seat_poly ist leer, die Standpruefung ist nicht aktiv"
} else { Bad "Stand: weder STAND_MIN_MARGIN noch STAND_SKIPPED aus info gelesen" }
}

# 2b) Positivfall Standsicherheit: 400 g weit hinter dem Po muessen die Pruefung FAIL liefern (Margin < STAND_MIN), sonst taugt sie nichts
if (Want '2b') {
if (-not $custom) {
  $r = Run-Scad ((PartArg 'info_selftest') + @('-o', (Join-Path $env:TEMP 'madbuddy_roboter_info_selftest.stl')))
  if ($r.Text -match 'STAND_SELFTEST_MARGIN=([-0-9.e]+) STAND_MIN=([-0-9.e]+)') {
    $m = [double]::Parse($Matches[1], $inv); $mn = [double]::Parse($Matches[2], $inv)
    if ($m -lt $mn) { Ok ("Selbsttest Stand: Masse weit hinten wird als FAIL erkannt (Margin {0:N1} < {1})" -f $m, $mn) }
    else { Bad ("Selbsttest Stand: Masse weit hinten bleibt im Stand (Margin {0:N1}), die Pruefung erkennt Kippen nicht" -f $m) }
  } else { Bad "Selbsttest Stand: STAND_SELFTEST_MARGIN nicht gelesen" }
}
}

# 3) Kollisionspruefung: Gehaeuse gegen alle Pruefkoerper (Platine, Glas, Rueckseite, Buchsen, Stecker, aktive Flaeche + Sichtkegel)
if (Want '3') {
$col = Join-Path $env:TEMP 'madbuddy_roboter_collision.stl'
if (Test-Path $col) { Remove-Item $col }
$r = Run-Scad ((PartArg 'collision') + @('-o', $col))
$n = FacetCount $col
# Die bekannte Leer-Meldung ("WARNING: Current top level object is empty") ist erwartet und wird vorher entfernt. Alles andere mit WARNING/ERROR zaehlt immer.
$rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
if ($rest -match 'WARNING|ERROR') { Bad "Kollision: WARNING/ERROR: $($rest.Trim())" }
elseif ($r.Text -match $emptyMsg) { Ok "Kollision: Schnittmenge leer (kollisionsfrei, Fenster gibt aktive Flaeche + Sichtkegel frei)" }
elseif ($r.Code -eq 0 -and $n -eq 0) { Ok "Kollision: STL ohne Facetten (kollisionsfrei)" }
elseif ($r.Code -eq 0) { Bad "Kollision: Gehaeuse schneidet Pruefkoerper ($n Facetten Schnittvolumen) -> $col" }
else { Bad "Kollision: Exitcode $($r.Code) $($r.Text.Trim())" }
}

# 3b) Positive-Kontrolle: die Pruefung muss einen absichtlichen Treffer erkennen (nur ohne -Define)
if (Want '3b') {
if (-not $custom) {
  $st = Join-Path $env:TEMP 'madbuddy_roboter_selftest.stl'
  if (Test-Path $st) { Remove-Item $st }
  $r = Run-Scad ((PartArg 'selftest') + @('-o', $st))
  if ((FacetCount $st) -gt 0) { Ok "Selbsttest: absichtliche Kollision wird erkannt" }
  else { Bad "Selbsttest: absichtliche Kollision NICHT erkannt (Pruefung taugt nichts)" }
}
}

# 3c) Kernloecher des Halssockels muessen im Material stecken: Bohrvolumen gegen head_back ohne Kernloecher, Y-Ausdehnung >= 7,5 mm
if (Want '3c') {
$pt = Join-Path $env:TEMP 'madbuddy_roboter_pilot_test.stl'
if (Test-Path $pt) { Remove-Item $pt }
$r = Run-Scad ((PartArg 'pilot_test') + @('-o', $pt))
if ((FacetCount $pt) -eq 0) { Bad "Halssockel-Kernloecher: Bohrvolumen trifft kein Material (Loch liegt in der Luft)" }
else {
  $ys = [regex]::Matches((Get-Content $pt -Raw), 'vertex\s+\S+\s+(\S+)\s+\S+') | ForEach-Object { [double]::Parse($_.Groups[1].Value, $inv) }
  $ext = ($ys | Measure-Object -Maximum).Maximum - ($ys | Measure-Object -Minimum).Minimum
  if ($ext -ge 7.5) { Ok ("Halssockel-Kernloecher: {0:N2} mm im Material (>= 7,5)" -f $ext) }
  else { Bad ("Halssockel-Kernloecher: nur {0:N2} mm im Material, verlangt >= 7,5" -f $ext) }
}
}

# 3d) Kernloecher im Oberkoerper (Hals-Mulde: Z, Sitz: Z, Dome: Y): Ausdehnung des Bohrvolumens im Material >= 7,5 mm
if (Want '3d') {
foreach ($t in @(@{ n = 'pilot_neck'; ax = 2; l = 'Oberkoerper Hals-Kernloecher (Z)' }, @{ n = 'pilot_seat'; ax = 2; l = 'Oberkoerper Sitz-Kernloecher (Z)' }, @{ n = 'pilot_dome'; ax = 1; l = 'Oberkoerper Dom-Kernloecher (Y)' })) {
  $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $t.n)
  if (Test-Path $f) { Remove-Item $f }
  $null = Run-Scad ((PartArg $t.n) + @('-o', $f))
  if ((FacetCount $f) -eq 0) { Bad "$($t.l): Bohrvolumen trifft kein Material (Loch liegt in der Luft)"; continue }
  $vs = [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)') | ForEach-Object { [double]::Parse($_.Groups[1 + $t.ax].Value, $inv) }
  $ext = ($vs | Measure-Object -Maximum).Maximum - ($vs | Measure-Object -Minimum).Minimum
  if ($ext -ge 7.5) { Ok ("{0}: {1:N2} mm im Material (>= 7,5)" -f $t.l, $ext) } else { Bad ("{0}: nur {1:N2} mm im Material, verlangt >= 7,5" -f $t.l, $ext) }
}
}

# 3e) Koerper-Kollisionen: Arm/Oberkoerper, Arm/Hand, Hals/Oberkoerper+Kopf, Oberkoerper/Kopf (Schnittmenge muss leer sein)
if (Want '3e') {
$cb = Join-Path $env:TEMP 'madbuddy_roboter_collision_body.stl'
if (Test-Path $cb) { Remove-Item $cb }
$r = Run-Scad ((PartArg 'collision_body') + @('-o', $cb)) 900
$rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
if ($rest -match 'WARNING|ERROR') { Bad "Koerper-Kollision: WARNING/ERROR: $($rest.Trim())" }
elseif (Is-EmptyRun $r (FacetCount $cb)) { Ok "Koerper-Kollision: Arm/Hand/Hals/Oberkoerper/Kopf kollisionsfrei" }
else { Bad "Koerper-Kollision: Teile schneiden sich ($(FacetCount $cb) Facetten) -> $cb" }
}

# 3e2) Positive-Kontrolle Koerper-Kollision: Arm 3 mm in den Oberkoerper geschoben MUSS Treffer liefern (nur ohne -Define)
if (Want '3e2') {
if (-not $custom) {
  $cs = Join-Path $env:TEMP 'madbuddy_roboter_cbody_selftest.stl'
  if (Test-Path $cs) { Remove-Item $cs }
  $null = Run-Scad ((PartArg 'collision_body_selftest') + @('-o', $cs))
  if ((FacetCount $cs) -gt 0) { Ok "Selbsttest Koerper-Kollision: Arm 3 mm im Oberkoerper wird erkannt" }
  else { Bad "Selbsttest Koerper-Kollision: Arm im Oberkoerper NICHT erkannt" }
}
}

# 3g) Montage: untere Halsschrauben nur in torso_front, jede Schraube mit Schraubendreher Ø6 x 60 entlang der Achse erreichbar (Phase 1 und 2), plus Positiv-Kontrollen
if (Want '3g') {
$pb = Join-Path $env:TEMP 'madbuddy_roboter_pilot_neck_back.stl'
if (Test-Path $pb) { Remove-Item $pb }
$r = Run-Scad ((PartArg 'pilot_neck_back') + @('-o', $pb))
if (Is-EmptyRun $r (FacetCount $pb)) { Ok "Montage: Kernloecher der unteren Halsschrauben liegen nicht in torso_back (nur in torso_front)" }
else { Bad "Montage: untere Halsschraube greift in torso_back" }
foreach ($t in @(@{ n = 'drv_p1'; l = 'Montage Phase 1 (Hals von oben, 2 Schrauben)' }, @{ n = 'drv_p2'; l = 'Montage Phase 2 (Kopfschrauben von unten, 2 Schrauben)' })) {
  $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $t.n)
  if (Test-Path $f) { Remove-Item $f }
  $r = Run-Scad ((PartArg $t.n) + @('-o', $f)) 900
  $rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
  if ($rest -match 'WARNING|ERROR') { Bad "$($t.l): WARNING/ERROR: $($rest.Trim())" }
  elseif (Is-EmptyRun $r (FacetCount $f)) { Ok "$($t.l): Schraubendreher Ø6 x 60 kollisionsfrei erreichbar" }
  else { Bad "$($t.l): Schraubendreher kollidiert ($(FacetCount $f) Facetten) -> $f" }
}
if (-not $custom) {
  foreach ($n in @('drv_selftest_p1', 'drv_selftest_p2')) {
    $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $n)
    if (Test-Path $f) { Remove-Item $f }
    $null = Run-Scad ((PartArg $n) + @('-o', $f)) 900
    if ((FacetCount $f) -gt 0) { Ok "Selbsttest $n : versetzter Schraubendreher wird als Kollision erkannt" }
    else { Bad "Selbsttest $n : versetzter Schraubendreher NICHT erkannt" }
  }
}
}

# 3f) Positive-Kontrolle Oberkoerper: zu grosse XIAO-Platte muss die Schale treffen (nur ohne -Define)
if (Want '3f') {
if (-not $custom) {
  $ts = Join-Path $env:TEMP 'madbuddy_roboter_torso_selftest.stl'
  if (Test-Path $ts) { Remove-Item $ts }
  $null = Run-Scad ((PartArg 'torso_selftest') + @('-o', $ts))
  if ((FacetCount $ts) -gt 0) { Ok "Selbsttest Oberkoerper: absichtliche Kollision wird erkannt" }
  else { Bad "Selbsttest Oberkoerper: absichtliche Kollision NICHT erkannt" }
}
}

# 3h) Task 4: Kernloecher der Bodendome im Material, Sitz gegen Oberkoerper/Arme/Haende/Fuesse, Bodenplatte gegen Sitz, Handauflage
if (Want '3h') {
$f = Join-Path $env:TEMP 'madbuddy_roboter_pilot_seat_dome.stl'
if (Test-Path $f) { Remove-Item $f }
$null = Run-Scad ((PartArg 'pilot_seat_dome') + @('-o', $f))
if ((FacetCount $f) -eq 0) { Bad "Sitz-Dom-Kernloecher: Bohrvolumen trifft kein Material (Loch liegt in der Luft)" }
else {
  $vs = [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)') | ForEach-Object { [double]::Parse($_.Groups[3].Value, $inv) }
  $ext = ($vs | Measure-Object -Maximum).Maximum - ($vs | Measure-Object -Minimum).Minimum
  if ($ext -ge 7.5) { Ok ("Sitz-Dom-Kernloecher (Z): {0:N2} mm im Material (>= 7,5)" -f $ext) } else { Bad ("Sitz-Dom-Kernloecher: nur {0:N2} mm im Material, verlangt >= 7,5" -f $ext) }
}
# Sitzschrauben M3x10: muessen mindestens 6 mm in den Oberkoerper greifen (Schraube Ø3 gegen torso_front/back mit Kernloechern, Z-Ausdehnung)
$f = Join-Path $env:TEMP 'madbuddy_roboter_screw_seat.stl'
if (Test-Path $f) { Remove-Item $f }
$null = Run-Scad ((PartArg 'screw_seat') + @('-o', $f)) 900
if ((FacetCount $f) -eq 0) { Bad "Sitzschrauben: greifen nicht in den Oberkoerper" }
else {
  $vs = [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)') | ForEach-Object { [double]::Parse($_.Groups[3].Value, $inv) }
  $ext = ($vs | Measure-Object -Maximum).Maximum - ($vs | Measure-Object -Minimum).Minimum
  if ($ext -ge 6) { Ok ("Sitzschrauben M3x10: greifen {0:N2} mm in den Oberkoerper (>= 6)" -f $ext) } else { Bad ("Sitzschrauben: greifen nur {0:N2} mm in den Oberkoerper, verlangt >= 6" -f $ext) }
}
$cs = Join-Path $env:TEMP 'madbuddy_roboter_collision_seat.stl'
if (Test-Path $cs) { Remove-Item $cs }
$r = Run-Scad ((PartArg 'collision_seat') + @('-o', $cs)) 900
$rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
if ($rest -match 'WARNING|ERROR') { Bad "Sitz-Kollision: WARNING/ERROR: $($rest.Trim())" }
elseif (Is-EmptyRun $r (FacetCount $cs)) { Ok "Sitz-Kollision: seat/bottom/Fuesse gegen Oberkoerper, Hals, Kopf, Arme, Haende kollisionsfrei" }
else { Bad "Sitz-Kollision: Teile schneiden sich ($(FacetCount $cs) Facetten) -> $cs" }
if (-not $custom) {
  $f = Join-Path $env:TEMP 'madbuddy_roboter_collision_seat_selftest.stl'
  if (Test-Path $f) { Remove-Item $f }
  $null = Run-Scad ((PartArg 'collision_seat_selftest') + @('-o', $f))
  if ((FacetCount $f) -gt 0) { Ok "Selbsttest Sitz-Kollision: Oberkoerper 3 mm im Sitz wird erkannt" } else { Bad "Selbsttest Sitz-Kollision: NICHT erkannt" }
}
# Handauflage: Grundriss der Handflaeche muss ganz auf dem Material von seat liegen (Differenz leer); Hand 14 mm nach aussen muss eine Luecke melden
$f = Join-Path $env:TEMP 'madbuddy_roboter_hand_support.stl'
if (Test-Path $f) { Remove-Item $f }
$r = Run-Scad ((PartArg 'hand_support') + @('-o', $f)) 900
$rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
if ($rest -match 'WARNING|ERROR') { Bad "Handauflage: WARNING/ERROR: $($rest.Trim())" }
elseif (Is-EmptyRun $r (FacetCount $f)) { Ok "Handauflage: Handflaeche liegt ganz auf dem Oberschenkel (seat)" }
else { Bad "Handauflage: Handflaeche ragt ueber den Oberschenkel ($(FacetCount $f) Facetten) -> $f" }
if (-not $custom) {
  $f = Join-Path $env:TEMP 'madbuddy_roboter_hand_support_selftest.stl'
  if (Test-Path $f) { Remove-Item $f }
  $null = Run-Scad ((PartArg 'hand_support_selftest') + @('-o', $f)) 900
  if ((FacetCount $f) -gt 0) { Ok "Selbsttest Handauflage: versetzte Hand (ueber der Kante) wird erkannt" } else { Bad "Selbsttest Handauflage: NICHT erkannt" }
}
# Montage Schritt 4 und 5: Schraubendreher Ø6 x 60 entlang der Achse gegen alle bis dahin montierten Teile
foreach ($t in @(@{ n = 'drv_p4'; l = 'Montage Schritt 4 (Sitzschrauben von unten, 4 Schrauben)' }, @{ n = 'drv_p5'; l = 'Montage Schritt 5 (Bodenschrauben von unten, 4 Schrauben)' })) {
  $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $t.n)
  if (Test-Path $f) { Remove-Item $f }
  $r = Run-Scad ((PartArg $t.n) + @('-o', $f)) 900
  $rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
  if ($rest -match 'WARNING|ERROR') { Bad "$($t.l): WARNING/ERROR: $($rest.Trim())" }
  elseif (Is-EmptyRun $r (FacetCount $f)) { Ok "$($t.l): Schraubendreher Ø6 x 60 kollisionsfrei erreichbar" }
  else { Bad "$($t.l): Schraubendreher kollidiert ($(FacetCount $f) Facetten) -> $f" }
}
if (-not $custom) {
  foreach ($n in @('drv_selftest_p4', 'drv_selftest_p5')) {
    $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $n)
    if (Test-Path $f) { Remove-Item $f }
    $null = Run-Scad ((PartArg $n) + @('-o', $f)) 900
    if ((FacetCount $f) -gt 0) { Ok "Selbsttest $n : versetzter Schraubendreher wird als Kollision erkannt" }
    else { Bad "Selbsttest $n : versetzter Schraubendreher NICHT erkannt" }
  }
}
}

# 3i) Kamera-Tasche, Wiege, Anschlag, Montagewege (Kamera v2). Jede "muss leer sein"-Pruefung hat eine Positiv-Kontrolle ("muss treffen"),
if (Want '3i') {
#     damit eine leere/falsch sitzende Geometrie nicht still als "gruen" durchgeht. Laeuft auch mit -Define (Gegenproben).
function Scad-Facets([string]$part, [string[]]$extra = @(), [int]$TimeoutSec = 900) {
  $o = Join-Path $env:TEMP "madbuddy_roboter_chk_$part.stl"
  if (Test-Path $o) { Remove-Item $o }
  $r = Run-Scad ($extra + (PartArg $part) + @('-o', $o)) $TimeoutSec
  return @{ N = (FacetCount $o); R = $r }
}
# Eval-Empty: liefert die Bewertung einer "muss leer sein"-Pruefung (Ok = $true/$false, Msg). WARNING/ERROR (ausser der bekannten Leer-Meldung) zaehlt immer als Fehler.
function Eval-Empty([string]$part, [string[]]$extra = @(), [int]$TimeoutSec = 900) {
  $x = Scad-Facets $part $extra $TimeoutSec
  $rest = ($x.R.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
  if ($rest -match 'WARNING|ERROR') { return @{ Ok = $false; Msg = "WARNING/ERROR $($rest.Trim())" } }
  elseif ($x.N -eq 0 -and ($x.R.Text -match $emptyMsg -or $x.R.Code -eq 0)) { return @{ Ok = $true; Msg = '' } }   # leer zaehlt nur nach echter Auswertung, nie nach Timeout/Absturz
  elseif ($x.N -eq 0) { return @{ Ok = $false; Msg = "nicht ausgewertet, Exitcode $($x.R.Code) $($x.R.Text.Trim())" } }
  else { return @{ Ok = $false; Msg = "($($x.N) Facetten)" } }
}
function Chk-Empty([string]$part, [string]$okMsg, [string]$badMsg, [string[]]$extra = @()) {
  $e = Eval-Empty $part $extra
  if ($e.Ok) { Ok $okMsg } else { Bad "$badMsg : $($e.Msg)" }
}
function Chk-Hit([string]$part, [string]$okMsg, [string]$badMsg, [string[]]$extra = @()) {
  $x = Scad-Facets $part $extra
  if ($x.R.Text -match 'WARNING: (?!Current top level object is empty)|ERROR') { Bad "$badMsg : WARNING/ERROR $($x.R.Text.Trim())"; return }
  if ($x.R.Code -ne 0) { Bad "$badMsg : Exitcode $($x.R.Code) $($x.R.Text.Trim())"; return }
  if ($x.N -gt 0) { Ok "$okMsg ($($x.N) Facetten)" } else { Bad $badMsg }
}
# Selbsttest der "muss leer sein"-Pruefung selbst: leere Geometrie MIT WARNING (unbekannte Variable) darf NICHT als gruen durchgehen, leere Geometrie ohne WARNING schon.
$w = Eval-Empty 'warn_ctrl'
if (-not $w.Ok -and $w.Msg -match 'WARNING') { Ok "Selbsttest Chk-Empty: leeres Ergebnis mit WARNING wird als Fehler erkannt" } else { Bad "Selbsttest Chk-Empty: WARNING bei leerem Ergebnis NICHT erkannt (Pruefung taugt nichts)" }
$w = Eval-Empty 'empty_ctrl'
if ($w.Ok) { Ok "Selbsttest Chk-Empty: sauber leeres Ergebnis wird als gruen gewertet" } else { Bad "Selbsttest Chk-Empty: sauber leeres Ergebnis faelschlich als Fehler: $($w.Msg)" }
# Selbsttest Timeout: ein erzwungener Timeout (1 s) darf bei einer "muss leer sein"-Pruefung NIE als gruen gelten.
$w = Eval-Empty 'asm_close' @() 1
if (-not $w.Ok -and $w.Msg -match 'TIMEOUT') { Ok "Selbsttest Chk-Empty: Timeout wird als Fehler erkannt (nie als leer = gruen)" } else { Bad "Selbsttest Chk-Empty: Timeout NICHT als Fehler erkannt (Pruefung taugt nichts): Ok=$($w.Ok) Msg=$($w.Msg)" }
Chk-Hit   'cam_selftest' "Kamera-Selbsttest: Kamerakoerper +0,6 mm trifft die Tasche (Pruefung wirkt)" "Kamera-Selbsttest: zu grosser Kamerakoerper NICHT erkannt"
Chk-Hit   'cam_collision_ctrl' "Kontrolle Kollision: um 0,6 mm nach vorn geschobener Kamera-Pruefkoerper trifft den Taschenboden" "Kontrolle Kollision: verschobener Kamera-Pruefkoerper NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'lens_tip'     "Linse: Spitze liegt in der Achse hinter der Aussenhuelle (Wand 3,9 = Linse 3,6 + 0,3)" "Linse: Spitze ragt ueber die Aussenhuelle (Wand zu duenn)"
Chk-Hit   'lens_tip_ctrl' "Kontrolle Linse: um 0,5 mm laengere Linse ragt ueber die Aussenhuelle und wird erkannt" "Kontrolle Linse: zu lange Linse NICHT erkannt (Pruefung taugt nichts)"
Chk-Hit   'cradle_fit'   "Wiege: Pfosten/Lippen umschliessen den Block (Positiv-Kontrolle)" "Wiege: beruehrt den aufgeweiteten Block NICHT (leer oder falsche Lage)"
Chk-Empty 'cradle_fit'   "Gegenprobe Wiege: ohne Wiege (cam_dbg_nocradle) beruehrt die Positiv-Kontrolle nichts (Kontrolle misst wirklich die Wiege)" "Gegenprobe Wiege: auch ohne Wiege trifft die Kontrolle (Pruefung taugt nichts)" @('-D', 'cam_dbg_nocradle=true')
Chk-Empty 'cam_inside'   "Rahmen, Wiege und Anschlag liegen innerhalb der Brusthaut (Abstand >= 0,2 mm)" "Rahmen/Wiege/Anschlag ragt aus der Brusthaut"
Chk-Hit   'inside_ctrl_z' "Kontrolle Brusthaut: um 30 mm nach oben verschobene Wiege wird erkannt" "Kontrolle Brusthaut (Z): verschobene Wiege NICHT erkannt (Pruefung taugt nichts)"
Chk-Hit   'inside_ctrl_x' "Kontrolle Brusthaut: um 30 mm zur Seite verschobene Wiege wird erkannt" "Kontrolle Brusthaut (X): verschobene Wiege NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'cam_present'  "Rahmen, Wiege und Anschlag sind in torso_front/torso_back vollstaendig vorhanden (nichts weggeschnitten, Rundungs-Splitter < 0,08 mm ausgenommen)" "Rahmen/Wiege/Anschlag fehlt teilweise in torso_front/torso_back"
Chk-Hit   'cam_present_ctrl' "Kontrolle Vollstaendigkeit: frei im Raum stehende Wiege wird als fehlend erkannt" "Kontrolle Vollstaendigkeit: fehlende Wiege NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'stack_inside' "XIAO-Block liegt ganz im Innenraum der Brust" "XIAO-Block ragt aus der Brusthaut"
Chk-Hit   'stack_inside_ctrl' "Kontrolle Block: um 40 mm nach vorn verschobener Block wird erkannt" "Kontrolle Block: verschobener Block NICHT erkannt (Pruefung taugt nichts)"
Chk-Hit   'cone_ctrl'    "Kontrolle Sichtkegel: um 8 mm nach hinten verschobener Kegel trifft die Brustwand" "Kontrolle Sichtkegel: verschobener Kegel NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'cam_channels' "Werkzeugkanaele (X +-8, Y -1...-7) und Kabelloch Ø10 sind frei von Tasche, Rahmen, Wiege und Anschlag" "Tasche/Rahmen/Wiege/Anschlag ragt in Werkzeugkanal oder Kabelloch"
Chk-Hit   'cam_channels_ctrl' "Kontrolle Kanaele: um 8 mm zur Mitte verschobene Kanaele treffen Wiege/Anschlag" "Kontrolle Kanaele: verschobene Kanaele NICHT erkannt (Pruefung taugt nichts)"
# Montagewege (der ganze Block wird bei abgenommener Rueckwand von hinten eingeschoben, dann torso_back schliessen); jede Reihe Zwischenstellungen + Kontrolle
Chk-Empty 'asm_block'    "Montage: Block (Kamera + XIAO) laesst sich von hinten durch Tasche und zwischen die Pfosten schieben (Zwischenstellungen bis 30 mm; Lippen weichen um 0,7 mm je Seite aus)" "Montage: Block stoesst beim Einsetzen an"
Chk-Hit   'asm_block_ctrl' "Kontrolle Montage Block: um 0,6 mm groesserer Kamerakoerper stoesst an" "Kontrolle Montage Block: zu grosser Kamerakoerper NICHT erkannt (Pruefung taugt nichts)"
Chk-Hit   'asm_block_lip' "Kontrolle Montage Block: voller Block trifft die Pfostenlippen (Schnapp-Ueberdeckung wird gemessen)" "Kontrolle Montage Block: Lippen werden NICHT getroffen (Pruefung taugt nichts)"
Chk-Empty 'asm_close'    "Montage: torso_back (Anschlag) laesst sich bei sitzendem Block aufschieben" "Montage: torso_back stoesst beim Schliessen an Block/Kamera"
Chk-Hit   'asm_close_ctrl' "Kontrolle Montage Schliessen: um 1 mm zum Anschlag verschobener Block wird getroffen" "Kontrolle Montage Schliessen: verschobener Block NICHT erkannt (Pruefung taugt nichts)"
}

# 3j) Druckbarkeit der Kamera-Geometrie in torso_front/torso_back (Druckausrichtung, ohne Stuetzen): neue Ueberhaenge > 45 Grad und Bruecken > 15 mm (tools\overhang.ps1).
if (Want '3j') {
#     Vergleich mit demselben Teil ohne Kamera (-D cam_off=true). Positiv-Kontrollen: steile Flanke und 20-mm-Decke muessen erkannt werden.
. (Join-Path $here 'tools\overhang.ps1')
foreach ($p in 'torso_front', 'torso_back') {
  $A = Join-Path $stlDir "roboter_$p.stl"
  $B = Join-Path $env:TEMP "madbuddy_roboter_${p}_nocam.stl"
  if (Test-Path $B) { Remove-Item $B }
  $rB = Run-Scad (@('-D', 'cam_off=true') + (PartArg $p) + @('-o', $B)) 900
  if ($rB.Code -ne 0) { Bad "Ueberhang $p : Vergleichsteil ohne Kamera nicht erzeugt, Exitcode $($rB.Code) $($rB.Text.Trim())"; continue }
  if (-not (Test-Path $A) -or -not (Test-Path $B)) { Bad "Ueberhang $p : STL fehlt"; continue }
  # Ausnahme nur fuer torso_front: Linsenloch-Fase (45 Grad zur Achse, Achse 15 Grad zur Druckrichtung geneigt = bis ca. 60 Grad Ueberhang), Entscheidung 2026-10-28:
  # Stuetzen am Linsenloch akzeptiert. Druck-Koordinaten: Achse durch (0, -cam_z, 0), nach innen (0, sin15, cos15); Zylinder R 6,5 (Fase aussen R ca. 5,1) bis 4 mm ueber dem Bett.
  if ($p -eq 'torso_front') { $o = Eval-Overhang $A $B 15 @(0, -27, 0) @(0, [math]::Sin(15 * [math]::PI / 180), [math]::Cos(15 * [math]::PI / 180)) 6.5 4.0 }
  else { $o = Eval-Overhang $A $B }
  if ($o.Steep -gt 0) { Bad "Ueberhang $p : $($o.Steep) neue Facetten mit > 45 Grad Ueberhang bei$($o.SteepAt)" }
  elseif ($o.Worst -gt 15) { Bad "Ueberhang $p : Bruecke $([math]::Round($o.Worst,1)) mm > 15 mm: $($o.WorstBox)" }
  else { Ok ("Ueberhang {0}: Kamera-Geometrie ohne Flanke > 45 Grad, {1} Decken, laengste Bruecke {2:N1} mm (<= 15), {3} Facetten der Linsenloch-Fase ausgenommen" -f $p, $o.Ceilings, $o.Worst, $o.Excluded) }
  if ($p -eq 'torso_front') { $Bfront = $B }
}
foreach ($c in @(@{ n = 'ovh_ctrl_steep'; l = 'steile Flanke (55 Grad)'; t = 'steep' }, @{ n = 'ovh_ctrl_flat'; l = '20-mm-Decke'; t = 'flat' })) {
  $f = Join-Path $env:TEMP "madbuddy_roboter_$($c.n).stl"
  if (Test-Path $f) { Remove-Item $f }
  $rC = Run-Scad ((PartArg $c.n) + @('-o', $f)) 900
  if ($rC.Code -ne 0) { Bad "Kontrolle Ueberhang ($($c.l)): Exitcode $($rC.Code) $($rC.Text.Trim())"; continue }
  if (-not (Test-Path $f)) { Bad "Kontrolle Ueberhang ($($c.l)): STL fehlt"; continue }
  $o = Eval-Overhang $f $Bfront
  if (($c.t -eq 'steep' -and $o.Steep -gt 0) -or ($c.t -eq 'flat' -and $o.Worst -gt 15)) { Ok "Kontrolle Ueberhang: $($c.l) wird erkannt" }
  else { Bad "Kontrolle Ueberhang: $($c.l) NICHT erkannt (Pruefung taugt nichts)" }
}

# Kontrollen zur Ausnahme: (1) ohne Ausnahme muss die Fase als steil erkannt werden (die Ausnahme blendet also wirklich nur die Fase aus), (2) ein steiles Merkmal
# ausserhalb der Fase (ovh_ctrl_steep_low: ganz unten, innerhalb der Hoehengrenze, aber weit von der Linsenachse) wird trotz Ausnahme erkannt.
$Af = Join-Path $stlDir 'roboter_torso_front.stl'
$o0 = Eval-Overhang $Af $Bfront
if ($o0.Steep -gt 0) { Ok "Kontrolle Ausnahme: ohne Ausnahme wird die Linsenloch-Fase erkannt ($($o0.Steep) steile Facetten)" } else { Bad "Kontrolle Ausnahme: Fase ohne Ausnahme NICHT erkannt (Ausnahme ist wirkungslos oder Pruefung blind)" }
$fl = Join-Path $env:TEMP 'madbuddy_roboter_ovh_ctrl_steep_low.stl'
if (Test-Path $fl) { Remove-Item $fl }
$rL = Run-Scad ((PartArg 'ovh_ctrl_steep_low') + @('-o', $fl)) 900
if ($rL.Code -ne 0 -or -not (Test-Path $fl)) { Bad "Kontrolle Ausnahme: ovh_ctrl_steep_low nicht erzeugt, Exitcode $($rL.Code) $($rL.Text.Trim())" }
else {
$o1 = Eval-Overhang $fl $Bfront 15 @(0, -27, 0) @(0, [math]::Sin(15 * [math]::PI / 180), [math]::Cos(15 * [math]::PI / 180)) 6.5 4.0
if ($o1.Steep -gt 0) { Ok "Kontrolle Ausnahme: steiles Merkmal ausserhalb der Fase wird trotz Ausnahme erkannt" } else { Bad "Kontrolle Ausnahme: steiles Merkmal ausserhalb der Fase wird von der Ausnahme verschluckt" }
}
}

# 4) PNG-Renders
if (Want '4') {
if (-not $NoPng) {
  $views = @(
    @{ n = 'assembly'; cam = '0,0,0,72,0,330,0'; part = 'assembly'; nodum = $true },
    @{ n = 'exploded'; cam = '0,0,0,70,0,320,0'; part = 'exploded'; nodum = $true },
    @{ n = 'section';  cam = '0,0,0,90,0,90,0';  part = 'section'; render = $true },
    @{ n = 'kamera_schnitt'; cam = '0,0,0,0,0,0,0'; part = 'cam_section'; ortho = $true }
  )
  foreach ($v in $views) {
    $png = Join-Path $pngDir ("roboter_{0}.png" -f $v.n)
    if (Test-Path $png) { Remove-Item $png }
    $a = (PartArg $v.part) + @('-o', $png, '--camera', $v.cam, '--viewall', '--autocenter', '--imgsize', '1400,1400', '--colorscheme', 'Tomorrow')
    if ($v.nodum) { $a = @('-D', 'show_dummies=false') + $a }   # Ansicht ohne Pruefkoerper, damit die Figur das Bild fuellt
    if ($v.ortho) { $a += '--projection'; $a += 'ortho' }
    if ($v.render) { $a += '--render' }   # CGAL statt Vorschau: Schnitt wird in der Vorschau nicht abgezogen
    $r = Run-Scad $a 400
    if ($r.Code -ne 0)                           { Bad "PNG $($v.n): Exitcode $($r.Code) $($r.Text.Trim())" }
    elseif ($r.Text -match 'WARNING')            { Bad "PNG $($v.n): WARNING: $($r.Text.Trim())" }
    elseif (-not (Test-Path $png) -or (Get-Item $png).Length -lt 5000) { Bad "PNG $($v.n): Datei fehlt/leer" }
    else { Ok "PNG $($v.n) -> $png" }
  }
}
}

# Aufraeumen: keine OpenSCAD-Reste
Get-CimInstance Win32_Process -Filter "Name like 'openscad%'" | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

if ($script:fails -gt 0) { Write-Host "== FEHLGESCHLAGEN: $($script:fails) Pruefung(en) ==" -ForegroundColor Red; exit 1 }
Write-Host "== ALLES GRUEN ==" -ForegroundColor Green
exit 0
