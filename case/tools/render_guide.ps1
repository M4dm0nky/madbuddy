# Rendert die Bilder der Einbauanleitung "Kamera + XIAO in den Oberkoerper" (Teile guide_1_pocket ... guide_5_close)
# nach case\render\einbau_kamera_1.png ... _5.png. -Only <n> rendert nur ein Bild. Abbruch (Exit 1) bei OpenSCAD-Fehler oder WARNING.
param([int]$Only = 0)
$ErrorActionPreference = 'Stop'
$case   = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$scad   = Join-Path $case 'madbuddy_roboter.scad'
$pngDir = Join-Path $case 'render'
$oscad  = 'C:\Program Files\OpenSCAD\openscad.com'
New-Item -ItemType Directory -Force $pngDir | Out-Null

# Kamera-Tasche in Weltkoordinaten: torso_origin [0,-8,29.6] + [0,-t_d/2,cam_z] = [0,-30,56.6] (per echo bestimmt); die guide_*-Teile sind per guide_c() so verschoben, dass die Tasche im Ursprung liegt.
# --camera = Gimbal: Zielpunkt x,y,z, rx,ry,rz, Abstand. rz=180: Blick von hinten (+Y) nach vorn, rx: Neigung von oben.
$views = @{
  1 = @{ part = 'guide_1_pocket'; cam = '0,0,-3,70,0,160,130' }
  2 = @{ part = 'guide_2_cam';    cam = '0,0,-3,70,0,160,130' }
  3 = @{ part = 'guide_3_clip';   cam = '0,0,-3,70,0,160,130' }
  4 = @{ part = 'guide_4_stack';  cam = '0,0,-3,70,0,160,130' }
  5 = @{ part = 'guide_5_close';  cam = '0,25,0,62,0,125,300' }
}
$fails = 0
foreach ($n in 1..5) {
  if ($Only -ne 0 -and $Only -ne $n) { continue }
  $v = $views[$n]
  $png = Join-Path $pngDir ("einbau_kamera_{0}.png" -f $n)
  if (Test-Path $png) { Remove-Item $png }
  $argList = @('-D', ('part=\"{0}\"' -f $v.part), '-o', $png, '--camera', $v.cam, '--imgsize', '1200,1200', '--colorscheme', 'Tomorrow', $scad)
  $errFile = [IO.Path]::GetTempFileName(); $outFile = [IO.Path]::GetTempFileName()
  $p = Start-Process -FilePath $oscad -ArgumentList $argList -NoNewWindow -PassThru -RedirectStandardError $errFile -RedirectStandardOutput $outFile
  $null = $p.Handle
  if (-not $p.WaitForExit(600000)) { try { & taskkill.exe /PID $p.Id /T /F 2>$null | Out-Null } catch {}; Write-Host "[FAIL] Bild ${n}: Zeitueberschreitung" -ForegroundColor Red; $fails++; continue }
  $p.WaitForExit()
  $txt = (Get-Content $errFile -Raw) + (Get-Content $outFile -Raw)
  Remove-Item $errFile, $outFile -ErrorAction SilentlyContinue
  if ($p.ExitCode -ne 0)                                    { Write-Host "[FAIL] Bild ${n}: Exitcode $($p.ExitCode) $($txt.Trim())" -ForegroundColor Red; $fails++ }
  elseif ($txt -match 'WARNING')                            { Write-Host "[FAIL] Bild ${n}: WARNING: $($txt.Trim())" -ForegroundColor Red; $fails++ }
  elseif (-not (Test-Path $png) -or (Get-Item $png).Length -lt 5000) { Write-Host "[FAIL] Bild ${n}: Datei fehlt/leer" -ForegroundColor Red; $fails++ }
  else { Write-Host "[ OK ] Bild $n -> $png" -ForegroundColor Green }
}
Get-CimInstance Win32_Process -Filter "Name like 'openscad%'" | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
if ($fails -gt 0) { exit 1 }













