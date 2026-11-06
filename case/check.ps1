# MadBuddy Gehaeuse: alle Pruefungen. Exitcode 0 = alles gruen, 1 = mindestens eine Pruefung fehlgeschlagen.
# Aufruf:  powershell -File case\check.ps1 [-Define window_shrink=2] [-NoPng]
param(
  [string[]]$Define = @(),   # zusaetzliche -D name=wert (nur Zahlen/Boolsche), z. B. window_shrink=2 fuer die Gegenprobe
  [switch]$NoPng
)
$ErrorActionPreference = 'Stop'
$here   = Split-Path -Parent $MyInvocation.MyCommand.Path
$scad   = Join-Path $here 'madbuddy_case.scad'
$oscad  = 'C:\Program Files\OpenSCAD\openscad.com'
$custom = $Define.Count -gt 0
$stlDir = if ($custom) { Join-Path $env:TEMP 'madbuddy_check_stl' } else { Join-Path $here 'stl' }
$pngDir = Join-Path $here 'render'
New-Item -ItemType Directory -Force $stlDir, $pngDir | Out-Null
$script:fails = 0

. (Join-Path $here 'tools/check_lib.ps1')   # Ok, Bad, Note, Run-Scad, PartArg, FacetCount

$emptyMsg = 'Current top level object is empty'

Write-Host "== madbuddy_case check ==  Define: $($Define -join ' ')"

# 1) STL-Export jedes Teils ohne Fehler/Warnungen
foreach ($p in 'front', 'back', 'frame_test', 'cam_test') {
  $out = Join-Path $stlDir "madbuddy_$p.stl"
  if (Test-Path $out) { Remove-Item $out }
  $r = Run-Scad ((PartArg $p) + @('-o', $out))
  $n = FacetCount $out
  if ($r.Code -ne 0)                { Bad "STL $p : Exitcode $($r.Code) $($r.Text.Trim())" }
  elseif ($r.Text -match 'WARNING') { Bad "STL $p : WARNING in Ausgabe: $(($r.Text -split "`n" | Select-String WARNING | Select-Object -First 3) -join ' | ')" }
  elseif ($r.Text -match 'ERROR')   { Bad "STL $p : ERROR in Ausgabe" }
  elseif ($n -lt 100) { Bad "STL $p : zu wenige Facetten ($n)" }
  else                              { Ok "STL $p ($n Facetten)" }
}

# 2) Standsicherheit (Lot des Schwerpunkts innerhalb der Standflaeche)
$r = Run-Scad ((PartArg 'info') + @('-o', (Join-Path $env:TEMP 'madbuddy_info.stl')))
if ($r.Text -match 'STAND_REAR_MARGIN=([-0-9.e]+) STAND_FRONT_MARGIN=([-0-9.e]+) STAND_MIN=([-0-9.e]+)') {
  $inv = [Globalization.CultureInfo]::InvariantCulture
  $mr = [double]::Parse($Matches[1], $inv); $mf = [double]::Parse($Matches[2], $inv); $mn = [double]::Parse($Matches[3], $inv)
  if ($mr -ge $mn -and $mf -ge $mn) { Ok ("Stand: Abstand Lot zu Kippkante hinten {0:N1} mm, vorn {1:N1} mm (min {2})" -f $mr, $mf, $mn) }
  else { Bad ("Stand: Abstand hinten {0:N1} mm, vorn {1:N1} mm, verlangt >= {2}" -f $mr, $mf, $mn) }
} else { Bad "Stand: keine Werte aus info gelesen" }

# 2b) Standebene: kein Punkt von Front/Rueckwand (inkl. Fuss) liegt unter der Tischebene des 15-Grad-Fusses
if ($r.Text -match 'PLANE tilt=([-0-9.e]+) c=([-0-9.e]+) zback=([-0-9.e]+)') {
  $inv = [Globalization.CultureInfo]::InvariantCulture
  $t = [double]::Parse($Matches[1], $inv) * [math]::PI / 180; $c = [double]::Parse($Matches[2], $inv); $zb = [double]::Parse($Matches[3], $inv)
  $minH = 1e9; $touch = 0
  foreach ($spec in @(@('front', $true), @('back', $false))) {
    $f = Join-Path $stlDir ("madbuddy_{0}.stl" -f $spec[0])
    foreach ($m in [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)')) {
      $y = [double]::Parse($m.Groups[2].Value, $inv); $z = [double]::Parse($m.Groups[3].Value, $inv)
      if ($spec[1]) { $y = -$y; $z = -$z } else { $z = $z + $zb }     # zurueck in Konstruktionskoordinaten
      $h = $y * [math]::Cos($t) + $z * [math]::Sin($t) - $c
      if ($h -lt $minH) { $minH = $h }
      if ([math]::Abs($h) -lt 0.01) { $touch++ }
    }
  }
  if ($minH -ge -0.01 -and $touch -gt 0) { Ok ("Standebene: nichts unter dem Fuss (tiefster Punkt {0:N3} mm ueber der Ebene, {1} Eckpunkte liegen auf der Standflaeche)" -f $minH, $touch) }
  else { Bad ("Standebene: Punkt {0:N3} mm unter der Fussebene bzw. keine Aufstandspunkte" -f $minH) }
} else { Bad "Standebene: PLANE-Werte fehlen" }

# 3) Kollisionspruefung: Gehaeuse gegen alle Pruefkoerper (inkl. Linse-Freiraum und aktive Flaeche + Sichtkegel)
$col = Join-Path $env:TEMP 'madbuddy_collision.stl'
if (Test-Path $col) { Remove-Item $col }
$r = Run-Scad ((PartArg 'collision') + @('-o', $col))
$n = FacetCount $col
if ($r.Text -match 'WARNING|ERROR' -and $r.Text -notmatch $emptyMsg) { Bad "Kollision: WARNING/ERROR: $($r.Text.Trim())" }
elseif ($r.Text -match $emptyMsg) { Ok "Kollision: Schnittmenge leer (kollisionsfrei, Fenster gibt aktive Flaeche + Sichtkegel frei)" }
elseif ($r.Code -eq 0 -and $n -eq 0) { Ok "Kollision: STL ohne Facetten (kollisionsfrei)" }
elseif ($r.Code -eq 0) {
  $vol = ''
  Bad "Kollision: Gehaeuse schneidet Pruefkoerper ($n Facetten Schnittvolumen) -> $col"
}
else { Bad "Kollision: Exitcode $($r.Code) $($r.Text.Trim())" }

# 3b) Positive-Kontrolle: die Pruefung muss einen absichtlichen Treffer erkennen (nur ohne -Define)
if (-not $custom) {
  $st = Join-Path $env:TEMP 'madbuddy_selftest.stl'
  if (Test-Path $st) { Remove-Item $st }
  $r = Run-Scad ((PartArg 'selftest') + @('-o', $st))
  if ((FacetCount $st) -gt 0) { Ok "Selbsttest: absichtliche Kollision wird erkannt" }
  else { Bad "Selbsttest: absichtliche Kollision NICHT erkannt (Pruefung taugt nichts)" }
}

# 3c) Kamera, Endwaende, Block-Montage, Kabelweg. Jede "muss leer sein"-Pruefung hat eine Positiv-Kontrolle ("muss treffen"),
#     damit eine leere/falsch sitzende Geometrie nicht still als "gruen" durchgeht. Laeuft auch mit -Define (Gegenproben).
function Scad-Facets([string]$part, [int]$TimeoutSec = 600, [string[]]$extra = @()) {
  $o = Join-Path $env:TEMP "madbuddy_chk_$part.stl"
  if (Test-Path $o) { Remove-Item $o }
  $r = Run-Scad ((PartArg $part) + $extra + @('-o', $o)) $TimeoutSec
  return @{ N = (FacetCount $o); R = $r }
}
# Eval-Empty: Bewertung einer "muss leer sein"-Pruefung (Ok = $true/$false, Msg). Gefiltert wird nur die Zeile mit der bekannten Leer-Meldung;
# jede andere WARNING/ERROR zaehlt als Fehler. "Leer" gilt nur nach echter Auswertung (Leer-Meldung oder Exitcode 0), nie nach Timeout/Absturz.
function Eval-Empty([string]$part, [int]$TimeoutSec = 600, [string[]]$extra = @()) {
  $x = Scad-Facets $part $TimeoutSec $extra
  $rest = ($x.R.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
  if ($rest -match 'WARNING|ERROR') { return @{ Ok = $false; Msg = "WARNING/ERROR $($rest.Trim())" } }
  elseif ($x.N -eq 0 -and ($x.R.Text -match $emptyMsg -or $x.R.Code -eq 0)) { return @{ Ok = $true; Msg = '' } }
  elseif ($x.N -eq 0) { return @{ Ok = $false; Msg = "nicht ausgewertet, Exitcode $($x.R.Code) $($x.R.Text.Trim())" } }
  else { return @{ Ok = $false; Msg = "($($x.N) Facetten)" } }
}
function Chk-Empty([string]$part, [string]$okMsg, [string]$badMsg, [string[]]$extra = @()) {
  $e = Eval-Empty $part 600 $extra
  if ($e.Ok) { Ok $okMsg } else { Bad "$badMsg : $($e.Msg)" }
}
function Chk-Hit([string]$part, [string]$okMsg, [string]$badMsg, [string[]]$extra = @()) {
  $x = Scad-Facets $part 600 $extra
  if ($x.R.Text -match 'WARNING: (?!Current top level object is empty)|ERROR') { Bad "$badMsg : WARNING/ERROR $($x.R.Text.Trim())"; return }
  if ($x.R.Code -ne 0) { Bad "$badMsg : Exitcode $($x.R.Code) $($x.R.Text.Trim())"; return }
  if ($x.N -gt 0) { Ok "$okMsg ($($x.N) Facetten)" } else { Bad $badMsg }
}
# Selbsttests der "muss leer sein"-Pruefung selbst: leer MIT WARNING darf nicht gruen sein, sauber leer schon, ein erzwungener Timeout (1 s) nie.
$w = Eval-Empty 'warn_ctrl'
if (-not $w.Ok -and $w.Msg -match 'WARNING') { Ok "Selbsttest Chk-Empty: leeres Ergebnis mit WARNING wird als Fehler erkannt" } else { Bad "Selbsttest Chk-Empty: WARNING bei leerem Ergebnis NICHT erkannt (Pruefung taugt nichts)" }
$w = Eval-Empty 'empty_ctrl'
if ($w.Ok) { Ok "Selbsttest Chk-Empty: sauber leeres Ergebnis wird als gruen gewertet" } else { Bad "Selbsttest Chk-Empty: sauber leeres Ergebnis faelschlich als Fehler: $($w.Msg)" }
$w = Eval-Empty 'cable_check' 1
if (-not $w.Ok -and $w.Msg -match 'TIMEOUT') { Ok "Selbsttest Chk-Empty: Timeout wird als Fehler erkannt (nie als leer = gruen)" } else { Bad "Selbsttest Chk-Empty: Timeout NICHT als Fehler erkannt (Pruefung taugt nichts): Ok=$($w.Ok) Msg=$($w.Msg)" }
Chk-Hit   'cam_selftest' "Kamera-Selbsttest: Kamerakoerper +0,6 mm trifft die Tasche (Pruefung wirkt)" "Kamera-Selbsttest: zu grosser Kamerakoerper NICHT erkannt"
Chk-Hit   'lens_tip_ctrl' "Kontrolle Linse: um 0,5 mm laengere Linse ragt ueber die Aussenhuelle und wird erkannt" "Kontrolle Linse: zu lange Linse NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'lens_tip'     "Linse: Spitze liegt 0,3 mm hinter der Aussenhuelle (Frontplatte 2,0 = Linse 3,6 + Taschenboden 1,9 - 0,3 Rueckstand)" "Linse: Spitze ragt ueber die Aussenhuelle (Frontplatte zu duenn)"
Chk-Hit   'asm_block_ctrl' "Kontrolle Montage Block: um 0,6 mm groesserer Kamerakoerper stoesst an" "Kontrolle Montage Block: zu grosser Kamerakoerper NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'asm_block'    "Montage: Block (Kamera + XIAO) laesst sich von hinten durch Tasche und Endwaende schieben (Zwischenstellungen bis 30 mm, voller Block)" "Montage: Block stoesst beim Einsetzen an"
foreach ($ts in @(@(0, 'USB-Ende (microSD)'), @(1, 'Klappstecker-Ende (Sense)'))) {
  Chk-Hit   'cradle_fit' "Endwand $($ts[1]): Bauteil um Spiel + 0,05 aufgeweitet trifft die Wand (Fuehrung <= 0,35 mm)" "Endwand $($ts[1]): KEINE Wand innerhalb Spiel + 0,05 (Block hat dort zu viel Luft oder Wand fehlt)" @('-D', "tray_side=$($ts[0])")
  Chk-Empty 'cradle_fit' "Gegenprobe Endwand $($ts[1]): um Spiel - 0,05 aufgeweitet bleibt frei (Spiel ist wirklich >= 0,25)" "Gegenprobe Endwand $($ts[1]): trifft schon bei Spiel - 0,05 (zu eng)" @('-D', "tray_side=$($ts[0])", '-D', 'tray_extra=-0.05')
}
Chk-Empty 'cradle_fit'   "Gegenprobe Wiege: ohne Endwaende (cam_dbg_nocradle) beruehrt die Positiv-Kontrolle nichts (Kontrolle misst wirklich die Wand)" "Gegenprobe Wiege: auch ohne Endwaende trifft die Kontrolle (Pruefung taugt nichts)" @('-D', 'cam_dbg_nocradle=true')
Chk-Empty 'fb_overlap'   "Wiege/Rahmen durchdringen das Rueckteil nicht" "Wiege/Rahmen durchdringen das Rueckteil"
Chk-Hit   'fb_ctrl'      "Kontrolle Rueckteil: um 6 mm verschobene Wiege wird erkannt" "Kontrolle Rueckteil: verschobene Wiege NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'cam_inside'   "Kamera-Tasche und Wiege liegen innerhalb der Kopfhaut" "Kamera-Tasche/Wiege ragt aus dem Kopf"
Chk-Hit   'inside_ctrl'  "Kontrolle Kopfhaut: um 15 mm verschobene Wiege wird erkannt" "Kontrolle Kopfhaut: verschobene Wiege NICHT erkannt (Pruefung taugt nichts)"
Chk-Empty 'cable_check'  "Kabelweg XIAO -> CN1 und P1 ist frei (kein Gehaeuse, Bauteil, Stapel im Weg)" "Kabelweg XIAO -> CN1/P1 ist blockiert"
Chk-Hit   'cable_ctrl'   "Kontrolle Kabelweg: zu hoch liegendes Kabel trifft die Rueckseitenbauteile" "Kontrolle Kabelweg: Treffer NICHT erkannt (Pruefung taugt nichts)"

# 4) PNG-Renders
if (-not $NoPng) {
  $views = @(
    @{ n = 'front';   cam = '0,0,0,90,0,0,0';    d = @('view_tilt=0', 'show_ground=false'); part = 'assembly' },
    @{ n = 'schraeg'; cam = '0,0,0,68,0,335,0';  d = @();                                    part = 'assembly' },
    @{ n = 'hinten';  cam = '0,0,0,72,0,200,0';  d = @('show_dummies=false');               part = 'assembly'; render = $true },
    @{ n = 'seite';   cam = '0,0,0,86,0,90,0';   d = @('show_dummies=false');               part = 'assembly'; render = $true },
    @{ n = 'schnitt'; cam = '0,0,0,90,0,90,0';   d = @();                                    part = 'section'  },
    @{ n = 'kamera_schnitt'; cam = '14,33.95,0,0,0,0,62'; d = @();                       part = 'cam_section'; ortho = $true }
  )
  foreach ($v in $views) {
    $png = Join-Path $pngDir ("madbuddy_{0}.png" -f $v.n)
    if (Test-Path $png) { Remove-Item $png }
    $a = (PartArg $v.part) + @('-o', $png, '--camera', $v.cam, '--imgsize', '1200,900', '--colorscheme', 'Tomorrow')
    if ($v.ortho) { $a += '--projection'; $a += 'ortho' } else { $a += '--viewall'; $a += '--autocenter' }   # ortho: fester Ausschnitt (Kamera-Naehe)
    foreach ($d in $v.d) { $a += '-D'; $a += $d }
    if ($v.render) { $a += '--render' }   # CGAL statt Vorschau: keine Darstellungsfehler bei Waenden
    $r = Run-Scad $a 400
    if ($r.Code -ne 0)                           { Bad "PNG $($v.n): Exitcode $($r.Code) $($r.Text.Trim())" }
    elseif ($r.Text -match 'WARNING')            { Bad "PNG $($v.n): WARNING: $($r.Text.Trim())" }
    elseif (-not (Test-Path $png) -or (Get-Item $png).Length -lt 5000) { Bad "PNG $($v.n): Datei fehlt/leer" }
    else { Ok "PNG $($v.n) -> $png" }
  }
}

# Aufraeumen: keine OpenSCAD-Reste
Get-CimInstance Win32_Process -Filter "Name like 'openscad%'" | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

if ($script:fails -gt 0) { Write-Host "== FEHLGESCHLAGEN: $($script:fails) Pruefung(en) ==" -ForegroundColor Red; exit 1 }
Write-Host "== ALLES GRUEN ==" -ForegroundColor Green
exit 0
