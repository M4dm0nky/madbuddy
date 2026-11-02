// MadBuddy: Kamera-/XIAO-Geometrie fuer den ROBOTER (Kamera v2: XIAO + Kamera als EIN Block in Lieferlage). Ersetzt xiao_cam_common.scad im Roboter; der Kopf (madbuddy_case.scad) nutzt weiter xiao_cam_common.scad. NICHT beide einbinden (gleiche Namen).
// Wird von den Gehaeuse-Dateien mit include <xiao_cam_common.scad> eingebunden (nach cyd_common.scad). OpenSCAD 2021.01.
// Alle Module arbeiten in EINEM lokalen System: Innenseite der Frontwand = Z 0, Blickrichtung (nach aussen) = +Z,
// Mitte der Linse = Ursprung (X/Y), lange XIAO-Seite = X. Die einbindende Datei platziert/kippt das System.
// Die Aussenflaeche der Frontwand liegt bei Z = wall_t (jedes Modul nimmt wall_t, Vorgabe cam_wall_t).
// Der Block (XIAO ESP32S3 Sense mit aufgesteckter Kamera) bleibt in der Lieferlage: die Kamera sitzt ueber dem microSD-Steckplatz, der Klappstecker liegt
// seitlich (+X). Die Kamera klemmt zwischen der Tasche in der Front und dem SD-Steckplatz (Schaumpad dazwischen). Kein Riegel, keine Nuten, kein FPC-Bogen.
// Block-Koordinaten der Seeed-Zeichnung: u = ab USB-Kante nach +X, v = quer (Linsenmitte 0), h = ueber der XIAO-Unterseite.
// Umrechnung ins lokale System: X = u - 3.53, Y = v, Z = xiao_zbot + h (bei xiao_flip: X gespiegelt, USB zeigt nach +X).
// Optional darf die einbindende Datei VOR dem include cam_chamfer_ang (Fasenwinkel) setzen. Definiert hier (NICHT in der einbindenden Datei wiederholen, sonst WARNING):
//   cam_*, xiao_* (siehe unten).
// Quellen: Seeed-STEP des Sense-Moduls (umgerechnet), gemessene Kamera OV3660 (schmale Linse), Spec docs/superpowers/specs (Kamera-Halter v2)

// ---------------------------------------------------------------- Auswahl
cam_type   = "OV3660";   // nur OV3660 (schmale Linse, gemessen) ist belegt
cam_wall_t = 3.9;        // Vorgabe fuer die Wandstaerke (Linse 3,6 + 0,3 Rueckstand); die einbindende Datei uebergibt ihren Wert

// ---------------------------------------------------------------- Kameramodul
cam_body       = 8.0;    // GEMESSEN: Kamerakoerper 8,0 x 8,0
cam_body_t     = 2.0;    // GEMESSEN: Dicke des Koerpers
cam_lens_d1    = 7.0;    // GEMESSEN: Linse unten Ø 7,0
cam_lens_h1    = 0.8;    // QUELLE: STEP (h 10,2 ... 11,0)
cam_lens_d2    = 5.7;    // GEMESSEN: Linse oben Ø 5,7
cam_lens_h2    = 2.8;    // QUELLE: STEP (h 11,0 ... 13,8)
cam_lens_h     = cam_lens_h1 + cam_lens_h2;   // GEMESSEN: Linsenhoehe 3,6
cam_depth      = cam_body_t + cam_lens_h;     // GEMESSEN: Gesamttiefe 5,6
cam_fov_d      = 102;    // QUELLE: Hersteller (diagonal, Grad)
cam_fov_h      = 85;     // QUELLE: Hersteller OV3660 (horizontal)
cam_dbg_nocradle = false; // nur Pruefhilfe (Gegenprobe): true = XIAO-Wiege entfaellt

// ---------------------------------------------------------------- Tasche
cam_lens_rec   = 0.3;    // QUELLE: Spec -- Linse endet 0,3 mm hinter der Aussenflaeche
cam_pocket     = 8.4;    // QUELLE: Spec -- Tasche 8,4 x 8,4 (Koerper 8,0 + 0,2 Spiel je Seite)
cam_hole_d     = 7.4;    // QUELLE: Spec -- Linsenloch Ø 7,4, durchgehend
cam_chamfer    = 1.0;    // Fase aussen am Linsenloch: Aufweitung je Seite (Winkel siehe cam_cone_ang)
// Flankenwinkel der Fase zur Linsenachse: 45 Grad (Spec). Die einbindende Datei darf cam_chamfer_ang setzen; bisher setzt keine Datei es.
cam_cone_ang   = is_undef(cam_chamfer_ang) ? 45 : cam_chamfer_ang;
cam_cone_h     = cam_chamfer / tan(cam_cone_ang);   // Tiefe der Fase entlang der Achse
cam_frame_wall = 2.0;    // Wandstaerke des Rahmens um die Tasche (>= 2 mm)
cam_rear_over  = 0.5;    // QUELLE: Spec -- der Rahmen reicht nur bis cam_zbb + 0,5 nach hinten (darunter liegt der SD-Steckplatz)
cam_open_w     = 7.2;    // QUELLE: Spec -- Oeffnung des Rahmens zur Klappstecker-Seite (+X), 7,2 breit

// ---------------------------------------------------------------- XIAO ESP32S3 Sense als Block (Seeed-STEP, u/v/h siehe oben)
xiao_flip    = false;    // true: Block gespiegelt, USB zeigt nach +X (Standard: USB nach -X, Klappstecker nach +X)
xiao_u_lens  = 3.53;     // QUELLE: STEP -- Linsenmitte u (USB-Kante bei X = -3,53)
xiao_v_mid   = 0.64;     // QUELLE: STEP -- Mitte der XIAO-Platine in v (v > 0 ist die Seite OHNE U.FL)
xiao_h_cam   = 8.2;      // QUELLE: STEP -- Rueckseite des Kamerakoerpers ueber der XIAO-Unterseite
xiao_pos     = [(xiao_flip ? -1 : 1) * (20.96 / 2 - xiao_u_lens), xiao_v_mid];   // Mitte der XIAO-Platine im lokalen System (X, Y)
xiao_play    = 0.3;      // Spiel um den Block (= fit)
xiao_post_t  = 2.4;      // Dicke der Wiege-Pfosten (>= 2 mm)
xiao_post_u0 = 7.0;      // QUELLE: Spec -- Pfosten nur im Bereich u 7,0 ... 12,5 (frei: Loetstellen 5V/GND am USB-Ende, D6/D7 am anderen Ende)
xiao_post_u1 = 12.5;
xiao_lip     = 1.0;      // Lippe ueber der XIAO-Unterseite (45 Grad Einfuehrschraege); Schnappmass je Seite = Lippe - Spiel
xiao_lip_gap = 0.1;      // Luft zwischen XIAO-Unterseite und Lippe
xiao_stop_gap = 0.15;    // QUELLE: Spec -- Anschlag im Rueckteil hinter der XIAO-Unterseite, Luft

// ================================================================= abgeleitete Werte
cam_half    = cam_pocket / 2;                    // halbe Taschenweite
cam_outer_h = cam_half + cam_frame_wall;         // halbe Rahmenweite
cam_tan_h   = tan(cam_fov_h / 2);
cam_tan_v   = sqrt(tan(cam_fov_d / 2) * tan(cam_fov_d / 2) - cam_tan_h * cam_tan_h);   // aus diagonal + horizontal
xdir        = xiao_flip ? -1 : 1;                // Richtung zur Klappstecker-Seite (Kabeloeffnung)
// Z-Lagen (lokal, Frontwand-Innenseite = 0, Aussenflaeche = w)
function cam_zbf(w = cam_wall_t) = w - cam_lens_h - cam_lens_rec;       // Vorderseite Kamerakoerper = Taschenboden
function cam_ztip(w = cam_wall_t) = w - cam_lens_rec;                   // Linsenspitze
function cam_zbb(w = cam_wall_t) = cam_zbf(w) - cam_body_t;             // Rueckseite Kamerakoerper
function xiao_zbot(w = cam_wall_t) = cam_zbb(w) - xiao_h_cam;           // XIAO-Unterseite
function cam_zrear(w = cam_wall_t) = cam_zbb(w) + cam_rear_over;        // Rahmen-Rueckseite

// Quader in Block-Koordinaten (u0..u1, v0..v1, h0..h1) im lokalen System; g = seitliches Mass dazu, gz0/gz1 = unten/oben
module xbox(u0, u1, v0, v1, h0, h1, w = cam_wall_t, g = 0, gz0 = 0, gz1 = 0) {
  x0 = u0 - xiao_u_lens - g; x1 = u1 - xiao_u_lens + g;
  translate([xiao_flip ? -x1 : x0, v0 - g, xiao_zbot(w) + h0 - gz0]) cube([x1 - x0, v1 - v0 + 2 * g, h1 - h0 + gz0 + gz1]);
}

// ================================================================= Tasche (positiv + negativ)
module cam_pocket_add(wall_t = cam_wall_t) {
  // Rahmen um die Tasche: von der Rahmen-Rueckseite bis 0,5 mm in die Wand hinein
  zr = cam_zrear(wall_t);
  translate([-cam_outer_h, -cam_outer_h, zr]) cube([2 * cam_outer_h, 2 * cam_outer_h, 0.5 - zr]);
  // Wandverstaerkung um das Loch: liegt der Taschenboden vor der Innenebene, fuellt die Innenseite bis zum Taschenboden
  if (cam_zbf(wall_t) < -0.001) translate([-cam_outer_h, -cam_outer_h, 0]) cube([2 * cam_outer_h, 2 * cam_outer_h, wall_t]);
}

module cam_pocket_cut(wall_t = cam_wall_t) {
  zbf = cam_zbf(wall_t); zbb = cam_zbb(wall_t); zr = cam_zrear(wall_t);
  // Linsenloch durch Boden und Wand, aussen 45 Grad gefast
  translate([0, 0, zbf - 0.01]) cylinder(d = cam_hole_d, h = wall_t - cam_cone_h - zbf + 0.02);
  translate([0, 0, wall_t - cam_cone_h]) cylinder(d1 = cam_hole_d, d2 = cam_hole_d + 2 * cam_chamfer, h = cam_cone_h + 0.01);
  translate([0, 0, wall_t]) cylinder(d = cam_hole_d + 2 * cam_chamfer, h = 1);
  // Koerpertasche: vom Taschenboden nach hinten offen (der Block wird von hinten eingesetzt)
  translate([-cam_half, -cam_half, zr - 1]) cube([2 * cam_half, 2 * cam_half, zbf - zr + 1]);
  // Oeffnung zur Klappstecker-Seite (+X): 7,2 breit, von cam_zbb - 0,5 bis zum Taschenboden
  translate([xdir > 0 ? 0 : -(cam_outer_h + 1), -cam_open_w / 2, zbb - 0.5]) cube([cam_outer_h + 1, cam_open_w, zbf - zbb + 0.5]);
}

// ================================================================= XIAO-Wiege (positiv + negativ)
// Je ein Pfosten an beiden Laengsseiten (Y aussen, xiao_play Spiel zur XIAO-Kante), nur im Bereich u xiao_post_u0 ... xiao_post_u1.
// Von der Wand (0,5 mm hinein) bis knapp unter die XIAO-Unterseite; dort greift eine Lippe mit 45-Grad-Einfuehrschraege (xiao_lip) ueber die
// Unterseite. Der Block wird von hinten eingeschoben: die Pfosten federn nach aussen, die Lippen rasten ueber der XIAO-Unterseite ein
// und halten ihn nach hinten (Schnappmass je Seite = Lippe - Spiel). Im Roboter schliesst zusaetzlich der Anschlag im Rueckteil.
xiao_vmin = -8.25; xiao_vmax = 9.53;    // QUELLE: STEP -- XIAO-Platine in v
// Oberer Pfosten (+Y): im Bereich u 7,0 ... 12,5 schneidet ihn im Roboter der Werkzeugkanal der Kopfschraube (X 4,7 ... 11,3) ab (Pfostenteile hingen frei).
// Darum liegt er hinter dem Kanal bei u 15,0 ... 17,2 (zwischen Pin 6 ohne Draht und Pin 7 mit D6/D7 bei u ca. 18,1). Entscheidung 2026-11-02 nach Testdruck.
xiao_post_u0_pos = 15.0; xiao_post_u1_pos = 17.2;
function xiao_post_x(sy) = let(u0 = sy > 0 ? xiao_post_u0_pos : xiao_post_u0, u1 = sy > 0 ? xiao_post_u1_pos : xiao_post_u1,
                               x0 = u0 - xiao_u_lens, x1 = u1 - xiao_u_lens) [xiao_flip ? -x1 : x0, x1 - x0];   // [Anfang X, Breite]
module xiao_cradle_add(wall_t = cam_wall_t) {
  zb = xiao_zbot(wall_t);
  if (!cam_dbg_nocradle) for (sy = [-1, 1]) {   // cam_dbg_nocradle nur fuer die Gegenprobe der Wiege-Pruefung
    px0 = xiao_post_x(sy)[0]; pw = xiao_post_x(sy)[1];
    yin = sy > 0 ? xiao_vmax + xiao_play : xiao_vmin - xiao_play;       // Innenflaeche des Pfostens
    yp = sy > 0 ? yin : yin - xiao_post_t;
    translate([px0, yp, zb - xiao_lip_gap]) cube([pw, xiao_post_t, 0.5 - zb + xiao_lip_gap]);
    // Lippe: Dreieck (Y/Z) mit ebener Oberseite 0,1 unter der XIAO-Unterseite, Schraege nach hinten zur Wand, ueber die Pfostenbreite extrudiert
    translate([px0, 0, 0]) rotate([90, 0, 90]) linear_extrude(pw)
      polygon(sy > 0 ? [[yin + 0.01, zb - xiao_lip_gap], [yin - xiao_lip, zb - xiao_lip_gap], [yin + 0.01, zb - xiao_lip_gap - xiao_lip]]
                     : [[yin - 0.01, zb - xiao_lip_gap], [yin + xiao_lip, zb - xiao_lip_gap], [yin - 0.01, zb - xiao_lip_gap - xiao_lip]]);
  }
}
module xiao_cradle_cut(wall_t = cam_wall_t) {               // Huellraum des Blocks (ohne Kamera) + Spiel seitlich; oben bis zur Rahmen-Rueckseite
  xbox(-3.11, 21.25, xiao_vmin, xiao_vmax, 0, cam_zrear(wall_t) - xiao_zbot(wall_t) - 0.02, wall_t, xiao_play);   // 0,02 unter der Rahmen-Rueckseite (keine deckungsgleiche Flaeche)
}

// ================================================================= Pruefkoerper
module cam_dummy(wall_t = cam_wall_t) {                     // Kamerakoerper, zweistufige Linse, FPC-Huellraum
  zbf = cam_zbf(wall_t); zbb = cam_zbb(wall_t);
  translate([-cam_body / 2, -cam_body / 2, zbb]) cube([cam_body, cam_body, zbf - zbb]);
  translate([0, 0, zbf - 0.01]) cylinder(d = cam_lens_d1, h = cam_lens_h1 + 0.01);
  translate([0, 0, zbf + cam_lens_h1 - 0.01]) cylinder(d = cam_lens_d2, h = cam_lens_h2 + 0.01);
  xbox(7.49, 15.12, -3.25, 3.25, 5.85, 8.75, wall_t);       // FPC (Huellraum)
}
module xiao_stack_dummy(wall_t = cam_wall_t) {              // XIAO, USB-C, Shield, Sense, microSD (bis h 8,2), Klappstecker
  xbox(0, 20.96, xiao_vmin, xiao_vmax, 0, 1.25, wall_t);                 // XIAO-Platine
  xbox(-1.53, 6.37, -3.86, 5.14, 0.26, 4.46, wall_t);                    // USB-C
  xbox(6.71, 17.31, -5.66, 6.94, 0.55, 3.25, wall_t);                    // Bauteile zwischen den Platinen
  xbox(6.58, 21.25, -7.75, 9.03, 4.18, 5.43, wall_t);                    // Sense-Platine
  xbox(-3.11, 11.95, -5.77, 5.41, 6.85, xiao_h_cam, wall_t);             // microSD-Steckplatz (im STEP bis 8,5; fuer Kollisionen auf 8,2 gekuerzt)
  xbox(15.12, 21.12, -7.43, 8.77, 5.42, 7.43, wall_t);                   // Klappstecker (weiss)
}
module cam_block_dummy(wall_t = cam_wall_t) { cam_dummy(wall_t); xiao_stack_dummy(wall_t); }
module cam_fov_cone(L = 30, wall_t = cam_wall_t) {          // Sichtpyramide ab der Linsenspitze (102 Grad diagonal / 85 Grad horizontal)
  zt = cam_ztip(wall_t);
  hull() {
    translate([0, 0, zt - 0.01]) cylinder(d = cam_lens_d2, h = 0.01);
    translate([-(cam_lens_d2 / 2 + L * cam_tan_h), -(cam_lens_d2 / 2 + L * cam_tan_v), zt + L])
      cube([2 * (cam_lens_d2 / 2 + L * cam_tan_h), 2 * (cam_lens_d2 / 2 + L * cam_tan_v), 0.01]);
  }
}
