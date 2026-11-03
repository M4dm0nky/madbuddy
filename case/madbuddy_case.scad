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

// ---------------------------------------------------------------- XIAO ESP32S3 Sense + Kamera: Masse, Tasche, Riegel, Wiege, Pruefkoerper
// Lokales System der Kamera: Frontwand-Innenseite = Z 0, Blick nach +Z, Linsenmitte = Ursprung (siehe cam_place unten).
include <xiao_cam_common.scad>

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
xiao_cy   = pcb_h / 2 + 0.4 + xiao_l / 2;      // Linse mittig in der Stirn (wie bisher), Stapel dahinter
xiao_bot  = -front_t + xiao_zbot(front_t);     // Konstruktions-Z der Stapelrueckseite
module cam_place() { translate([0, xiao_cy, -front_t]) children(); }   // lokales Kamerasystem -> Konstruktionskoordinaten
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
      // Kameratasche (Rahmen mit Riegelnuten) und XIAO-Wiege (Stirnpfosten)
      cam_place() { cam_pocket_add(front_t); xiao_cradle_add(front_t); }
    }
    window_cut();
    cam_place() { cam_pocket_cut(front_t); xiao_cradle_cut(front_t); }   // Linsenloch 6,7 mit Fase, Tasche, Nuten, FPC-Schlitz
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
      // Anschlagpfosten unter dem XIAO-Stapel und Kabelkanal (Schienen), mit der Stapellage (xiao_offset) mitgewandert;
      // die Pfosten enden 0,1 mm unter der Stapelrueckseite
      for (sx = [-1, 1]) {
        translate([xiao_offset[0] + sx * 7.75 - 1.75, xiao_cy + xiao_offset[1] + 5, z_floor - 0.01]) cube([3.5, 3.0, xiao_bot - 0.1 - z_floor + 0.01]);
        translate([xiao_offset[0] + sx * 3.8 - 0.6, pcb_h / 2 + 0.6, z_floor - 0.01]) cube([1.2, 17, xiao_bot - 0.1 - z_floor + 0.01]);
      }
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
      translate([xiao_offset[0] + sx - 0.8, xiao_cy + xiao_offset[1] - 3, z_back - 1]) cube([1.6, 12, back_t + 2]);
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
  ya = xiao_cy + xiao_offset[1] - xiao_l / 2 - 0.9;      // knapp unter der unteren Stapelkante
  bx = xiao_offset[0];
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
  echo(str("FPC_AIR=", cam_fpc_air(front_t), " FPC_AIR_MAX=", cam_fpc_air_max));
  echo(str("FPC_R=", cam_fpc_rmin(front_t), " FPC_R_MIN=", cam_bend_r));
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
  color("#e0e0e0") cam_place() cam_clip_place(front_t);
}

if (part == "front") rotate([180, 0, 0]) front_shell();
else if (part == "cam_clip") cam_clip();
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
else if (part == "clip_test")
  // Riegel in Einbaulage gegen die Frontschale: MUSS durch das Uebermass (0,1 mm) Schnittmenge haben
  intersection() { front_shell(); cam_place() cam_clip_place(front_t); }
else if (part == "clip_band")
  // Obergrenze fuer den Riegel: die Schnittmenge darf NUR im Uebermassband (unter dem Nutboden zg1, hintere Lippe) liegen.
  // Ohne Nut (cam_dbg_nogroove=true) liegt der Riegel im vollen Material -> Rest nicht leer -> FAIL.
  difference() {
    intersection() { front_shell(); cam_place() cam_clip_place(front_t); }
    translate([-50, -50, -front_t + cam_zbb(front_t) - cam_clip_t - 1])
      cube([100, 100, cam_clip_t + 1 + cam_zg1(front_t) - cam_zbb(front_t) + 0.0005]);
  }
else if (part == "cradle_fit")
  // Positive-Kontrolle Wiege: Pfosten/Steg muessen den Stapel (um 0,6 mm aufgeweitet) beruehren, sonst fehlt die Wiege oder sie sitzt falsch
  intersection() {
    cam_place() xiao_cradle_add(front_t);
    cam_place() translate([xiao_offset[0] - xiao_w / 2 - 0.6, xiao_offset[1] - xiao_l / 2 - 0.6, xiao_zbot(front_t) - 0.6])
      cube([xiao_w + 1.2, xiao_l + 1.2, xiao_stack + 1.2]);
  }
else if (part == "inside_ctrl")
  // Positive-Kontrolle zu cam_inside: um 15 mm nach oben verschoben MUSS die Wiege aus der Kopfhaut ragen
  difference() {
    translate([0, 15, 0]) cam_place() { cam_pocket_add(front_t); xiao_cradle_add(front_t); }
    translate([0, 0, z_floor]) linear_extrude(-z_floor) offset(delta = -0.8) outer2d();
  }
else if (part == "fb_ctrl")
  // Positive-Kontrolle zu fb_overlap: um 15 mm nach oben verschoben MUSS die Wiege das Rueckteil (Ring) schneiden
  intersection() { translate([0, 15, 0]) cam_place() { cam_pocket_add(front_t); xiao_cradle_add(front_t); } back_part(); }
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
  intersection() { cam_place() { cam_pocket_add(front_t); xiao_cradle_add(front_t); } back_part(); }
else if (part == "cam_inside")
  // Tasche und Wiege muessen mindestens 0,8 mm innerhalb der Aussenkontur des Kopfes bleiben
  difference() {
    cam_place() { cam_pocket_add(front_t); xiao_cradle_add(front_t); }
    translate([0, 0, z_floor]) linear_extrude(-z_floor) offset(delta = -0.8) outer2d();
  }
else if (part == "cam_section")
  // Schnitt durch die Kameratasche bei X = 0 (Blick von +X), mit Riegel, Kamera, XIAO-Stapel; Hinterteil zeigt die Anschlagpfosten.
  // Gedreht (Blick von +X auf die Schnittflaeche: Frontwand links, Y nach oben), damit die Kamera-Einstellung des Prueflaufs einfach bleibt.
  // Jedes Teil einzeln geschnitten, damit die Schnittflaechen ihre eigene Farbe behalten.
  rotate([0, -90, 0]) union() {
    intersection() { color("#f2b84b") front_shell(); cam_cutbox(); }
    intersection() { color("#4fa8c9") back_part(); cam_cutbox(); }
    intersection() { color("#e8e8e8") cam_place() cam_clip_place(front_t); cam_cutbox(); }
    intersection() { color("#d03030") cam_body_dummy(); cam_cutbox(); }
    intersection() { color("#8a4fc9") xiao_dummy(); cam_cutbox(); }
  }
