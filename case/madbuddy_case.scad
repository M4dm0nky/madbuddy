// MadBuddy Gehaeuse "Kopf mit Stirn" -- ESP32-2432S028 (CYD 2,8", Micro-USB + USB-C) + XIAO ESP32S3 Sense
// OpenSCAD 2021.01. Teil waehlen mit:  -D 'part="front"'
// Koordinaten (Konstruktion): X nach rechts (Blick auf das Gesicht), Y nach oben, Z zum Betrachter.
// Gesichtsebene = Z 0, das Innere liegt bei negativem Z. Ursprung X/Y = Mitte der aktiven Display-Flaeche.
// part = "front" | "back" | "frame_test" | "assembly" | "section" | "collision" | "info"

part = "assembly";

// ---------------------------------------------------------------- Steuerung / Pruefhilfen
view_tilt     = 15;    // nur Ansicht (assembly/section): Neigung des Kopfes nach hinten in Grad
show_ground   = true;  // nur Ansicht: Tischplatte andeuten
show_dummies  = true;  // nur Ansicht: Pruefkoerper (Platine, Display, XIAO ...) mitzeigen

// ---------------------------------------------------------------- Allgemein
fit        = 0.3;   // Passungsspiel in mm (Variable)
wall       = 2.4;   // Wandstaerke (>= 2 mm)
front_t    = 2.0;   // Dicke der Frontplatte (Gesicht)
back_t     = 2.6;   // Dicke der Rueckwand (Platz fuer Schraubenkopf-Senkung)
ch         = 1.2;   // Fase 45 Grad an der Druckbett-Kante (Front vorn, Rueckwand hinten)
lip_h      = 3.0;   // Hoehe der Falz (Passlippe) zwischen Front und Rueckwand
tilt       = 15;    // Neigung des Fusses nach hinten in Grad
$fn        = 64;

// ---------------------------------------------------------------- Werte, die cyd_common.scad aus dem Gehaeuse erwartet bzw. die gehaeusespezifisch sind
glass_gap  = 0.2;   // Luft zwischen Display-Oberkante und Frontplatte
disp_x     = 2.0;   // war -1; +3 gleicht aa_left 10 -> 13 aus (Platine bleibt, Fenster folgt der echten Bildflaeche). Versatz des Displaymoduls (Platine+Glas+Fenster) im Gehaeuse in X, negativ = nach links
rear_extra  = 10.0; // Zusatztiefe hinten: verlegt die hintere Kippkante nach hinten (Standsicherheit bei 15 Grad), Platz fuer Akku/Kabel

// CYD-Masse, Fenster und Pruefkoerper. Muss NACH front_t, glass_gap, fit, disp_x stehen: OpenSCAD wertet Zuweisungen der Reihe nach aus.
include <cyd_common.scad>

// ---------------------------------------------------------------- XIAO ESP32S3 Sense + Kamera (Halter v2): Masse, Tasche, Endwaende, Pruefkoerper
// Lokales System der Kamera: Frontwand-Innenseite = Z 0, Blick nach +Z, Linsenmitte = Ursprung (siehe cam_place unten).
// Im Kopf nur die Endwaende des Schachts (Indizes 2..5 von xiao_tray()): zwischen Display-Kante und Stirn-Innenhaut sind nur ca. 19 mm frei, der volle Schacht braucht 24,4 mm.
// Die Tasche haelt den Block in X/Y, der Anschlag im Rueckteil in Z. Muss VOR dem include stehen.
// Nutzerentscheidung 2026-11-06 (Platzmangel in der Stirn): Endwaende = Indizes 2..5. Index 2 (Klappstecker-Ende, v -8,55 ... -7,0, U.FL-Seite) bleibt nach dem Zuschnitt
// auf Y >= 25,3 nur 1,65 mm lang (< 3 mm, Splitter) und entfaellt: [3, 4, 5].
cam_tray_idx = [3, 4, 5];
include <xiao_cam_block.scad>

// ---------------------------------------------------------------- Kopfform
dome_d      = 6.4;  // Schraubdom aussen
dome_x      = 36.0; // Domposition X (links/rechts)
pilot_d     = 2.6;  // Kernloch fuer M3 in PETG
pilot_len   = 7.5;
screw_d     = 3.4;  // Durchgang M3 in der Rueckwand
cbore_d     = 6.2;  // Senkung Schraubenkopf
cbore_t     = 1.8;
head_r      = 15;   // Eckradius des Kopfes
fillet_r    = 8;    // Verrundung zwischen Kopf und Stirn
stirn_cy    = 36;   // Stirn-Wölbung Mitte Y
stirn_rx    = 36;   // Stirn-Wölbung Halbbreite
stirn_ry    = 11.5; // Stirn-Wölbung Halbhoehe
foot_w      = 66;   // Fussbreite
stand_min   = 1.5;  // geforderter Mindestabstand des Lots zum Rand der Standflaeche (mm)

// ================================================================= abgeleitete Werte
a_lip     = (wall - fit) / 2;                 // Aussenhaelfte der Wand im Falzbereich
z_split   = z_pcb_bot;
z_floor   = z_rear - rear_clear - rear_extra;
z_back    = z_floor - back_t;
// Gehaeusemass fest: bis 2026-11-03 aus der Platinenlage zur Bildmitte berechnet (= 47,4); mit aa_left 13 waere das Gehaeuse 6 mm geschrumpft
hx_in     = 47.4;
assert(hx_in >= pcb_x1 + sock_over + usb_clear && hx_in >= -pcb_x0 + fit, "Gehaeuse zu schmal fuer die Platine");
dome_y    = pcb_h / 2 + fit + dome_d / 2 + 0.1;
hy_in     = dome_y + dome_d / 2;
hx        = hx_in + wall;
hy        = hy_in + wall;
xiao_cy   = pcb_h / 2 + 0.4 - (xiao_vmin - xiao_play);   // Linsenmitte Y: Unterkante des Blocks samt Spiel (Linse + vmin - Spiel = Linse - 8,25 - 0,3) liegt 0,4 mm ueber der Display-Kante (pcb_h/2) -> ca. 33,95
xiao_bot  = -front_t + xiao_zbot(front_t);     // Konstruktions-Z der Stapelrueckseite
module cam_place() { translate([0, xiao_cy, -front_t]) children(); }   // lokales Kamerasystem -> Konstruktionskoordinaten
// Anschlag im Rueckteil (Muster cam_stop() im Roboter): Anschlagflaeche xiao_stop_gap hinter der XIAO-Unterseite
stop_u0 = 0.5; stop_u1 = 7.5;      // VORLÄUFIG (Entwurf, wie Roboter): Anschlag in u mittig am USB-Ende -- Loetstellen u 2,9/5,4 liegen am Platinenrand (v +-8), u 18,1 weit ausserhalb
stop_v0 = -4;  stop_v1 = 5;        // VORLÄUFIG (Entwurf, wie Roboter): Anschlag in v, die Platinenkanten mit den Loetstellen (-8,25 / 9,53) bleiben frei
xiao_stop_z  = xiao_bot - xiao_stop_gap;                                  // Anschlagflaeche (Konstruktions-Z)
xiao_stop_x0 = xiao_flip ? -(stop_u1 - xiao_u_lens) : stop_u0 - xiao_u_lens;   // Anschlag in X
xiao_stop_x1 = xiao_flip ? -(stop_u0 - xiao_u_lens) : stop_u1 - xiao_u_lens;
dome_pts  = [for (sx = [-1, 1], sy = [-1, 1]) [sx * dome_x, sy * dome_y]];
// Fusslage: Tischebene y*cos(tilt)+z*sin(tilt)=c, beruehrt die hintere untere Kante
plane_c   = -hy * cos(tilt) + z_back * sin(tilt);
function foot_y(z) = (plane_c - z * sin(tilt)) / cos(tilt);

// ================================================================= 2D-Formen

module outer2d() {
  offset(r = -fillet_r) offset(r = fillet_r)
    union() {
      rrect(hx, hy, head_r);
      translate([0, stirn_cy]) scale([stirn_rx, stirn_ry]) circle(r = 1, $fn = 120);
    }
}

// Extrusion mit 45-Grad-Fase (in kleinen Stufen, entspricht der Druckschichtung)
module chamf_extrude(z0, z1, c, at_top = true, steps = 4) {
  if (at_top) {
    translate([0, 0, z0]) linear_extrude(z1 - z0 - c + 0.001) children();
    for (i = [0 : steps - 1])
      translate([0, 0, z1 - c + i * c / steps])
        linear_extrude(c / steps + (i == steps - 1 ? 0 : 0.001))
          offset(r = -(i + 0.5) * c / steps) children();
  } else {
    translate([0, 0, z0 + z1]) mirror([0, 0, 1]) chamf_extrude(z0, z1, c, true, steps) children();
  }
}

// ================================================================= Teile (Konstruktionslage)
module usb_cuts() {
  for (u = usb_list) if (u[4])
    translate([pcb_x1 - 3, u[0] - u[2] / 2, usb_zc - usb_open_h / 2])
      cube([hx + 3 - pcb_x1 + 1, u[2], usb_open_h]);
}

module sd_cut() {
  translate([sd_x - sd_open_w / 2, -hy - 20, sd_zc - sd_open_h / 2])
    cube([sd_open_w, hy - pcb_h / 2 + 20 + 0.5, sd_open_h]);
}

// Endwaende des Schachts, auf den Innenraum des Kopfs zugeschnitten: innen an die Kopfwand angewachsen (0,01 mm Ueberlappung), nicht nach aussen, nicht ins Display (Y >= pcb_h/2 + fit)
// Oben (Z >= z_split + lip_h, wo die Innenhaelfte der Kopfwand wieder voll ist) wachsen sie mit 0,01 mm Ueberlappung an die Wand an (offset -wall + 0,01).
// Darunter liegen Falz und Wandring des Rueckteils (Innenflaeche bei offset -wall): dort halten die Endwaende 0,05 mm Abstand, sonst schneiden sie das Rueckteil (fb_overlap).
module cam_cradle_clipped() intersection() {
  cam_place() xiao_cradle_add(front_t);
  union() {
    translate([0, 0, z_split + lip_h]) linear_extrude(-(z_split + lip_h))
      intersection() { offset(r = -wall + 0.01) outer2d(); translate([-200, pcb_h / 2 + fit]) square([400, 200]); }
    translate([0, 0, z_floor]) linear_extrude(z_split + lip_h - z_floor + 0.001)
      intersection() { offset(r = -wall - 0.05) outer2d(); translate([-200, pcb_h / 2 + fit]) square([400, 200]); }
  }
}
module cam_mount_raw() cam_place() { cam_pocket_add(front_t); xiao_cradle_add(front_t); }   // ungeschnitten, nur fuer die Positiv-Kontrollen
module cam_mount_add() { cam_place() cam_pocket_add(front_t); cam_cradle_clipped(); }   // Tasche (Rahmen) + zugeschnittene Endwaende

module front_shell() {
  difference() {
    union() {
      difference() {
        chamf_extrude(z_split, 0, ch, true) outer2d();
        // Falz-Aussparung (Aussenhaelfte der Wand bleibt)
        translate([0, 0, z_split - 0.01]) linear_extrude(lip_h + 0.01) offset(r = -a_lip) outer2d();
        // Hauptraum
        translate([0, 0, z_split - 0.01]) linear_extrude(-front_t - z_split + 0.01) offset(r = -wall) outer2d();
      }
      // Schraubdome: von der Frontplatte bis zur Innenseite der Rueckwand
      for (p = dome_pts) translate([p[0], p[1], z_floor]) cylinder(d = dome_d, h = -front_t / 2 - z_floor);
      // Kameratasche (Rahmen) und Endwaende des XIAO-Schachts
      cam_mount_add();
    }
    window_cut();
    cam_place() { cam_pocket_cut(front_t); xiao_cradle_cut(front_t); }   // Linsenloch 7,4 mit Fase, Tasche, Oeffnung zum Klappstecker, Huellraum des Blocks
    usb_cuts();
    sd_cut();
    for (p = dome_pts) translate([p[0], p[1], z_floor - 0.01]) cylinder(d = pilot_d, h = pilot_len);
  }
}

module back_part() {
  difference() {
    union() {
      chamf_extrude(z_back, z_floor, ch, false) outer2d();
      // Wandring
      translate([0, 0, z_floor - 0.01]) linear_extrude(z_split - z_floor + 0.01)
        difference() { outer2d(); offset(r = -wall) outer2d(); }
      // Falz
      translate([0, 0, z_split - 0.01]) linear_extrude(lip_h - 0.2 + 0.01)
        difference() {
          offset(r = -(a_lip + fit)) outer2d();
          offset(r = -wall) outer2d();
          for (p = dome_pts) translate(p) circle(d = dome_d + 0.5);
        }
      // Platinen-Andruckfuesse mit Zentrierstift
      for (p = hole_pts) translate([p[0], p[1], z_floor - 0.01]) {
        cylinder(d = 6.4, h = z_pcb_bot - 0.1 - z_floor + 0.01);
        cylinder(d = hole_d - 0.4, h = z_pcb_bot - 0.1 - z_floor + 0.01 + (pcb_t - 0.2));
      }
      // Anschlag unter dem XIAO-Block (Muster cam_stop() im Roboter): ein Pfosten mittig am USB-Ende, Anschlagflaeche xiao_stop_gap hinter der XIAO-Unterseite.
      // Er haelt den Block in Z (die Tasche haelt ihn in X/Y). Frei bleiben die Loetstellen am USB-Ende (u 2,9 / 5,4) und u 18,1 am Rand des Blocks.
      translate([xiao_stop_x0, xiao_cy + stop_v0, z_floor - 0.01]) cube([xiao_stop_x1 - xiao_stop_x0, stop_v1 - stop_v0, xiao_stop_z - z_floor + 0.01]);
      // Kabelkanal (Schienen), mit der Blocklage (xiao_pos) mitgefuehrt; enden wie der Anschlag
      for (sx = [-1, 1])
        translate([xiao_pos[0] + sx * 3.8 - 0.6, pcb_h / 2 + 0.6, z_floor - 0.01]) cube([1.2, 17, xiao_stop_z - z_floor + 0.01]);
      // Fuss mit Neigung
      yt = -hy + 1.5;
      translate([-foot_w / 2, 0, 0]) rotate([90, 0, 90]) linear_extrude(foot_w)
        polygon([[yt, z_back], [-hy, z_back], [foot_y(z_split), z_split], [yt, z_split]]);
    }
    // Schrauben von hinten
    for (p = dome_pts) {
      translate([p[0], p[1], z_back - 1]) cylinder(d = screw_d, h = back_t + 2);
      translate([p[0], p[1], z_back - 0.01]) cylinder(d = cbore_d, h = cbore_t + 0.01);
    }
    usb_cuts();
    sd_cut();
    // BOOT/RESET
    translate([btn_x - btn_open_w / 2, -pcb_h / 2 + btn_y_from_bottom - btn_open_h / 2, z_back - 1])
      cube([btn_open_w, btn_open_h, back_t + 2]);
    // Lueftungsschlitze hinter dem XIAO
    for (sx = [-9.5, -6.5, 6.5, 9.5])
      translate([xiao_pos[0] + sx - 0.8, xiao_cy + xiao_pos[1] - 3, z_back - 1]) cube([1.6, 12, back_t + 2]);
  }
}

module frame_test() {
  // flacher Streifen: Frontplatte mit Fenster + Auflage-Rahmen fuer das Display, mit Markierungen der Platinen-Loecher
  x0 = pcb_x0 - fit - wall; x1 = glass_x1 + fit + wall; yy = glass_h / 2 + fit + wall;
  difference() {
    union() {
      translate([0, 0, -front_t]) linear_extrude(front_t) translate([x0, -yy]) square([x1 - x0, 2 * yy]);
      translate([0, 0, z_pcb_top])
        linear_extrude(-front_t - z_pcb_top + 0.01)
          difference() {
            translate([x0, -yy]) square([x1 - x0, 2 * yy]);
            translate([pcb_x0 - fit, -glass_h / 2 - fit]) square([glass_w + 2 * fit, glass_h + 2 * fit]);
          }
    }
    window_cut();
    for (p = hole_pts) if (p[0] < glass_x1) translate([p[0], p[1], -front_t - 0.01]) cylinder(d = hole_d, h = 0.6);
  }
}

// ================================================================= Pruefkoerper (Dummies)
module plugs_dummy() {
  // nur dort, wo eine Gehaeuseoeffnung ist
  for (u = usb_list) if (u[4])
    translate([pcb_x1 + sock_over, u[0] - u[3] / 2, usb_zc - plug_h / 2]) cube([hx + 25, u[3], plug_h]);
}
tray_side   = 0;      // nur Pruefhilfe (cradle_fit): Seite des Schachts, die gemessen wird (0 USB-Ende, 1 Klappstecker-Ende)
tray_extra  = 0.05;   // nur Pruefhilfe (cradle_fit): Aufweitung ueber das Spiel hinaus (Gegenprobe -0,05: dann leer)
cam_tip_ref = 2.0 - cam_lens_rec;   // Pruefmass fuer lens_tip: Linsenspitze der Konstruktion (lokal, Soll-Frontplatte 2,0 minus 0,3 Rueckstand = 1,7); unabhaengig von front_t, damit eine zu duenne Frontplatte auffaellt
module block_probe() cam_block_dummy(front_t);   // ganzer Block (Kamera + XIAO) fuer den Montageweg
module cam_body_probe(grow) {   // Kamerakoerper + Linse (ohne FPC), um grow groesser, fuer den Montageweg
  translate([-(cam_body + grow) / 2, -(cam_body + grow) / 2, cam_zbb(front_t)]) cube([cam_body + grow, cam_body + grow, cam_body_t]);
  translate([0, 0, cam_zbf(front_t) - 0.01]) cylinder(d = cam_lens_d1 + grow, h = cam_lens_h1 + 0.01);
  translate([0, 0, cam_zbf(front_t) + cam_lens_h1 - 0.01]) cylinder(d = cam_lens_d2 + grow, h = cam_lens_h2 + 0.01);
}
module cam_body_dummy() cam_place() cam_dummy(front_t);           // Kamerakoerper, Tubus, FPC
module xiao_dummy()     cam_place() xiao_stack_dummy(front_t);    // XIAO-Stapel
module lens_cone()      cam_place() cam_fov_cone(30, front_t);    // Sichtpyramide 102/85 Grad, 30 mm

// Kabelweg XIAO -> CYD (Pruefkoerper, Ader/Kabel 1,6 mm): 4 Adern vom unteren Rand des Stapels (Loetstellen an der Laengsseite)
// zu CN1 (untere Laengskante) und P1 (+X-Schmalseite), im Raum hinter den Rueckseitenbauteilen (Z -20).
cab_w   = 1.6;                                  // VORLÄUFIG (Schaetzung) Dicke eines Kabelbuendels
cab_z   = -20;                                  // VORLÄUFIG (Entwurf) Hoehe des Kabelwegs hinter den Rueckseitenbauteilen (z_rear = -16,3)
cn1_x   = pcb_x1 - 55;                          // VORLÄUFIG (Foto, +-2 mm) CN1-Mitte, gemessen von der USB-Kante
p1_y    = pcb_h / 2 - 16;                       // VORLÄUFIG (Foto, +-2 mm) P1-Mitte in Y (16 mm von der oberen Laengskante)
module cab_seg(p, q) hull() for (r = [p, q]) translate(r - [cab_w / 2, cab_w / 2, cab_w / 2]) cube(cab_w);
module cable_dummy(dz = 0) {
  ya = xiao_cy + xiao_vmin - xiao_play - 0.9;      // knapp unter der unteren Blockkante (XIAO-Platine samt Spiel)
  bx = xiao_pos[0];
  zc = cab_z + dz;
  // zu CN1: nach unten durch den Freiraum, am Plattenrand (Y < -25) hoch zum Stecker
  cab_seg([bx - 5, ya, zc], [bx - 5, 8, zc]);
  cab_seg([bx - 5, 8, zc], [cn1_x, -pcb_h / 2 - 3.5, zc]);
  cab_seg([cn1_x, -pcb_h / 2 - 3.5, zc], [cn1_x, -pcb_h / 2 - 3.5, z_pcb_bot - 1.7]);
  // zu P1: schraeg ueber die Platinenmitte, an der +X-Kante (neben den Buchsen, Y > 2,3) hoch zum Stecker
  cab_seg([bx + 1, ya, zc], [30, 14, zc]);
  cab_seg([30, 14, zc], [pcb_x1 + 0.9, p1_y, zc]);
  cab_seg([pcb_x1 + 0.9, p1_y, zc], [pcb_x1 + 0.9, p1_y, z_pcb_bot - 1.7]);
}
module card_dummy() {
  // Karte (Pruefkoerper) steckt in der Wandoeffnung und ragt nach aussen (nur Kollisionspruefung, nicht in der Ansicht)
  translate([sd_x - card_w / 2, -hy - 12, z_pcb_bot - sd_h / 2 - card_t / 2]) cube([card_w, hy + 12 - pcb_h / 2 + 6, card_t]);
}
dummy_sel = "all"; // Debug: nur einen Pruefkoerper pruefen
module dummies() {
  if (dummy_sel != "all") {
    if (dummy_sel == "pcb") pcb_dummy(); if (dummy_sel == "glass") glass_dummy(); if (dummy_sel == "rear") rear_dummy();
    if (dummy_sel == "sockets") sockets_dummy(); if (dummy_sel == "plugs") plugs_dummy(); if (dummy_sel == "buttons") buttons_dummy();
    if (dummy_sel == "xiao") { xiao_dummy(); cam_body_dummy(); } if (dummy_sel == "lens") lens_cone(); if (dummy_sel == "active") active_cone(); if (dummy_sel == "sd") { sd_dummy(); card_dummy(); }
  } else {
  pcb_dummy(); glass_dummy(); rear_dummy(); sockets_dummy(); plugs_dummy(); buttons_dummy();
  xiao_dummy(); cam_body_dummy(); lens_cone(); active_cone(); sd_dummy(); card_dummy();
  }
}

// ================================================================= Schwerpunkt / Standsicherheit (Schaetzung)
// [Masse g, y, z] im Kopf-Koordinatensystem
masses = [
  [28 + 0.2 * rear_extra, 2.5, -3.5 + 0.1 * z_floor],  // Frontschale + Dome
  [20, 0, z_back + back_t / 2],                         // Rueckplatte
  [14 + 0.45 * rear_extra, 1.0, (z_floor + z_split) / 2], // Rueckwaende
  [10, -37, (z_back + z_split) / 2],                    // Fuss
  [25, 0, -11],       // Platine mit Bauteilen
  [10, 0, -4.5],      // Display
  [5, xiao_cy, -9]    // XIAO + Kamera
];
mtot = sum_col(0);
function sum_col(i, k = 0) = k >= len(masses) ? 0 : (i == 0 ? masses[k][0] : masses[k][0] * masses[k][i]) + sum_col(i, k + 1);
com_y = sum_col(1) / mtot;
com_z = sum_col(2) / mtot;
// horizontale Weltkoordinate (Tiefe) eines Kopfpunktes: z*cos(t) - y*sin(t)
function wz(y, z) = z * cos(tilt) - y * sin(tilt);
rear_edge  = wz(-hy, z_back);
front_edge = wz(foot_y(z_split), z_split);
com_w      = wz(com_y, com_z);
margin_rear  = com_w - rear_edge;
margin_front = front_edge - com_w;

module info() {
  echo(str("HEAD_W=", 2 * hx, " HEAD_H=", hy + stirn_cy + stirn_ry, " DEPTH=", -z_back));
  echo(str("STAND_REAR_MARGIN=", margin_rear, " STAND_FRONT_MARGIN=", margin_front, " STAND_MIN=", stand_min));
  echo(str("PLANE tilt=", tilt, " c=", plane_c, " zback=", z_back));
  echo(str("WINDOW=", win_w, "x", win_h));
  cube(1);
}

// ================================================================= Anordnung
// Ausschnitt um die Kamera, nur X <= 0 (Schnittebene X = 0), fuer part = "cam_section"
module cam_cutbox() translate([-25, xiao_cy - 14, z_floor - 3]) cube([25, 28, -z_floor + 6]);
module world() {
  // Anzeige-Welt: Z nach oben, Gesicht blickt nach -Y, Kopf um view_tilt nach hinten geneigt, steht auf Z=0
  c = -hy * cos(view_tilt) + z_back * sin(view_tilt);
  rotate([90, 0, 0]) translate([0, -c, 0]) rotate([-view_tilt, 0, 0]) children();
  if (show_ground) color([0.85, 0.85, 0.88, 0.5]) translate([-90, -60, -2]) cube([180, 120, 2]);
}
module assembly_parts() {
  color("#f2b84b") front_shell();
  color("#4fa8c9") back_part();
  if (show_dummies) {
  color("#2e8b4a", 0.6) pcb_dummy();
  color("#222222", 0.6) glass_dummy();
  color("#888888", 0.5) rear_dummy();
  color("#c0c0c0") { sockets_dummy(); buttons_dummy(); sd_dummy(); }
  color("#8a4fc9", 0.8) xiao_dummy();
  color("#d03030") cam_body_dummy();
  }
}

if (part == "front") rotate([180, 0, 0]) front_shell();
else if (part == "cam_test")   // Testausschnitt (flach druckbar, Aussenseite unten wie front): Stirn um Tasche und Endwaende, X -14 ... 24,5 / Y +-14 mm um die Linse
  rotate([180, 0, 0]) intersection() {
    front_shell();
    translate([-14, xiao_cy - 14, z_floor]) cube([38.5, 28, -z_floor + 1]);
  }
else if (part == "back") translate([0, 0, -z_back]) back_part();
else if (part == "frame_test") rotate([180, 0, 0]) frame_test();
else if (part == "assembly") world() assembly_parts();
else if (part == "section")
  difference() {
    world() assembly_parts();
    translate([0, -500, -500]) cube(1000);
  }
else if (part == "collision")
  intersection() {
    union() { front_shell(); back_part(); }
    dummies();
  }
else if (part == "info") info();
else if (part == "selftest")
  // Positive-Kontrolle fuer die Kollisionspruefung: absichtlich zu grosse Platine, MUSS Treffer liefern
  intersection() {
    union() { front_shell(); back_part(); }
    scale([1.1, 1.1, 1]) pcb_dummy();
  }
else if (part == "cam_selftest")
  // Positive-Kontrolle Kamera: Kamerakoerper +0,6 mm (Tasche hat nur +0,5) MUSS die Tasche treffen
  intersection() {
    front_shell();
    cam_place() translate([-(cam_body + 0.6) / 2, -(cam_body + 0.6) / 2, cam_zbb(front_t)])
      cube([cam_body + 0.6, cam_body + 0.6, cam_zbf(front_t) - cam_zbb(front_t)]);
  }
else if (part == "cradle_fit")
  // Positive-Kontrolle Endwaende: das aussenliegende Bauteil der Seite tray_side, um Spiel + 0,05 aufgeweitet, MUSS die (zugeschnittene) Endwand beruehren.
  // 0 USB-Ende: microSD-Steckplatz (u -3,11); 1 Klappstecker-Ende: Sense-Platine (u 21,25). Gegenprobe tray_extra=-0.05: leer (Spiel ist wirklich >= 0,25); cam_dbg_nocradle=true: leer.
  let(e = xiao_play + tray_extra) intersection() {
    cam_cradle_clipped();
    cam_place() if (tray_side == 0) xbox(-3.11 - e, -1, -5.77, 5.41, 6.85, xiao_h_cam, front_t);
    else xbox(19, 21.25 + e, -7.75, 9.03, 4.18, 5.43, front_t);
  }
else if (part == "lens_tip")   // Linsenspitze (Achsen-Probe Ø1, bis cam_tip_ref) darf die Aussenhuelle (Gesichtsebene Z 0) nicht durchstossen: leer. Bei zu duenner Frontplatte (front_t < 2,0) ragt sie heraus.
  difference() { cam_place() translate([0, 0, cam_tip_ref - 1]) cylinder(d = 1, h = 1); translate([0, 0, z_split]) linear_extrude(-z_split) outer2d(); }
else if (part == "lens_tip_ctrl")   // Kontrolle: Linse 0,5 mm laenger ragt ueber die Aussenhuelle, MUSS treffen
  difference() { cam_place() translate([0, 0, cam_tip_ref - 1]) cylinder(d = 1, h = 1.5); translate([0, 0, z_split]) linear_extrude(-z_split) outer2d(); }
else if (part == "asm_block")   // Block (Kamera + XIAO) wird bei abgenommenem Rueckteil von hinten (lokal -Z) in Tasche und Endwaende geschoben (Zwischenstellungen alle 1,5 mm bis 30 mm): darf nirgends anstossen
  for (s = [0 : 1.5 : 30]) intersection() { front_shell(); cam_place() translate([0, 0, -s]) block_probe(); }
else if (part == "asm_block_ctrl")   // Kontrolle: Kamerakoerper +0,6 mm MUSS die Tasche treffen
  for (s = [0 : 1.5 : 30]) intersection() { front_shell(); cam_place() translate([0, 0, -s]) cam_body_probe(0.6); }
else if (part == "inside_ctrl")
  // Positive-Kontrolle zu cam_inside: um 15 mm nach oben verschoben MUSS die Wiege aus der Kopfhaut ragen
  difference() {
    translate([0, 15, 0]) cam_mount_raw();   // ungeschnittene Endwaende (der Zuschnitt wuerde den Treffer wegnehmen)
    translate([0, 0, z_floor]) linear_extrude(-z_floor) offset(delta = -0.8) outer2d();
  }
else if (part == "fb_ctrl")
  // Positive-Kontrolle zu fb_overlap: um 6 mm nach oben verschoben MUSS die Wiege das Rueckteil (Wandring unterhalb der Teilungsebene) schneiden. (Bei 15 mm laege alles schon ueber dem Kopf: die Endwaende enden bei Y ca. 47 und der Ring bei 47,5.)
  intersection() { translate([0, 6, 0]) cam_mount_raw(); back_part(); }   // ungeschnittene Endwaende (der Zuschnitt wuerde den Treffer wegnehmen)
else if (part == "cable_check")
  // Kabelweg zu P1 und CN1 muss frei sein: Kabel gegen Gehaeuse, Platine, Rueckseitenbauteile, Buchsen, Stapel, Kamera
  intersection() {
    cable_dummy();
    union() { front_shell(); back_part(); pcb_dummy(); glass_dummy(); rear_dummy(); sockets_dummy(); buttons_dummy(); sd_dummy(); xiao_dummy(); cam_body_dummy(); }
  }
else if (part == "cable_ctrl")
  // Positive-Kontrolle zum Kabelweg: 9 mm hoeher (Z -11) liegt das Kabel in den Rueckseitenbauteilen -> MUSS treffen
  intersection() {
    cable_dummy(9);
    union() { front_shell(); back_part(); pcb_dummy(); glass_dummy(); rear_dummy(); sockets_dummy(); buttons_dummy(); sd_dummy(); xiao_dummy(); cam_body_dummy(); }
  }
else if (part == "warn_ctrl") { echo(k = undefined_k2_probe); intersection() { cube(1); translate([5, 5, 5]) cube(1); } }   // Kontrolle fuer Chk-Empty: leer, aber mit WARNING (unbekannte Variable)
else if (part == "empty_ctrl") intersection() { cube(1); translate([5, 5, 5]) cube(1); }   // Kontrolle fuer Chk-Empty: sauber leer, keine WARNING
else if (part == "fb_overlap")
  // Rahmen und Wiege (Teile der Frontschale) duerfen das Rueckteil nicht durchdringen (Ring, Falz, Pfosten, Schienen).
  // Nur die neue Geometrie wird geprueft: Front gegen Rueck insgesamt ueberlappt schon im Bestand an den Schraubdomen.
  intersection() { cam_mount_add(); back_part(); }
else if (part == "cam_inside")
  // Tasche und Endwaende muessen mindestens 0,8 mm innerhalb der Aussenkontur des Kopfes bleiben
  difference() {
    cam_mount_add();
    translate([0, 0, z_floor]) linear_extrude(-z_floor) offset(delta = -0.8) outer2d();
  }
else if (part == "cam_section")
  // Schnitt durch die Kameratasche bei X = 0 (Blick von +X), mit Kamera, XIAO-Block; Hinterteil zeigt den Anschlag.
  // Gedreht (Blick von +X auf die Schnittflaeche: Frontwand links, Y nach oben), damit die Kamera-Einstellung des Prueflaufs einfach bleibt.
  // Jedes Teil einzeln geschnitten, damit die Schnittflaechen ihre eigene Farbe behalten.
  rotate([0, -90, 0]) union() {
    intersection() { color("#f2b84b") front_shell(); cam_cutbox(); }
    intersection() { color("#4fa8c9") back_part(); cam_cutbox(); }
    intersection() { color("#d03030") cam_body_dummy(); cam_cutbox(); }
    intersection() { color("#8a4fc9") xiao_dummy(); cam_cutbox(); }
  }
