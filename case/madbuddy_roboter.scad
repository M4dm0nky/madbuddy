// MadBuddy Roboter (Stufe 3): druckbare Teile fuer die sitzende Roboterfigur. Dieses Paket: Kopf (Front, Blende, Rueckwand, Ohren, Zierschrauben).
// OpenSCAD 2021.01. Teil waehlen mit:  -D 'part="head_front"'
// Konstruktionslage des Kopfes wie Stufe 2: X nach rechts (Blick auf das Gesicht), Y nach oben, Z zum Betrachter.
// Gesichtsebene = Z 0, das Innere liegt bei negativem Z. Ursprung X/Y = Mitte der aktiven Display-Flaeche (disp_x = 0).
// Weltlage (place_head): Z oben, das Gesicht blickt nach -Y, der Kopf ist um head_tilt nach hinten geneigt.
// part = "head_front" | "bezel" | "head_back" | "ear" | "deco" | "assembly" | "exploded" | "section" | "collision" | "selftest" | "info"
// Druckausrichtung: die Teile-Zweige am Dateiende liefern jedes Teil fertig auf dem Bett, ohne Stuetzen.
//
// ERWEITERN (Task 3/4): neue Teile als Module in Konstruktionslage + Weltlage anlegen, dann
//   1. in world_shells() / world_dummies() einhaengen,
//   2. ihre Massen an die Liste 'masses' haengen (concat),
//   3. seat_poly (Standflaeche, Welt-XY) setzen -> die Standpruefung wird dann aktiv,
//   4. in check_roboter.ps1 die Liste $parts erweitern und im part-Zweig unten den Druckzweig ergaenzen.

part = "assembly";

// ---------------------------------------------------------------- Steuerung / Pruefhilfen
show_dummies = true;   // nur Ansicht: Pruefkoerper (Platine, Display ...) mitzeigen

// ---------------------------------------------------------------- Allgemein
fit        = 0.3;    // Passungsspiel in mm (Variable); cyd_common.scad braucht es nicht, der Roboter schon
fit_peg    = 0.15;   // Spiel fuer Steckzapfen/Zierschrauben, als Presspassung gedacht
wall       = 2.4;    // Wandstaerke (>= 2 mm)
front_t    = 2.0;    // Dicke der Frontplatte (Gesicht)
back_t     = 2.6;    // Dicke der Rueckwand (Platz fuer Schraubenkopf-Senkung)
leg_h      = 33.6;   // Auftraggeber (Fix-Runde): Hoehe der Oberschenkel 32 (Konzept) + 1,6, damit ueber den Zellen 2 mm Luft bleiben; der ganze Oberbau (Oberkoerper, Hals, Kopf, Arme, Haende) wandert um 1,6 mit
lift       = leg_h - 32;   // abgeleitet: Verschiebung des Oberbaus gegenueber dem Konzept (nur fuer die Massenliste)
ch         = 4.0;    // 45-Grad-Fase rundum am Gesicht und an der Rueckseite (statt Kugelrundung, ohne Stuetzen druckbar)
lip_h      = 3.0;    // Hoehe der Falz (Passlippe) zwischen Front und Rueckwand
head_tilt  = 8;      // QUELLE: Konzept (roboter_konzept.scad head_tilt): Neigung des Kopfes nach hinten in Grad
$fn        = 64;

// ---------------------------------------------------------------- Werte, die cyd_common.scad vom Gehaeuse erwartet
glass_gap  = 0.2;    // Luft zwischen Display-Oberkante und Frontplatte (wie Stufe 2)
disp_x     = 0;      // Roboter: Display sitzt mittig im Kopf

// CYD-Masse, Fenster und Pruefkoerper. Muss NACH front_t, glass_gap, fit, disp_x stehen: OpenSCAD wertet Zuweisungen der Reihe nach aus.
include <cyd_common.scad>

// ---------------------------------------------------------------- Kopfform
head_w      = 114;   // QUELLE: Konzept (h_w): Kopfbreite
head_h      = 84;    // QUELLE: Konzept (h_h): Kopfhoehe
head_r      = 14;    // QUELLE: Konzept (h_r): Eckradius in der Frontansicht
rear_air    = 17;    // Zusatztiefe hinten (Kabelweg, Halssockel): Rueckwand braucht Bohrung 24,3 + 2 x 2 mm Rand = 28,3 mm Tiefe
dome_d      = 6.4;   // Schraubdom aussen (wie Stufe 2)
dome_x      = 36.0;  // Domposition X (links/rechts), wie Stufe 2
pilot_d     = 2.6;   // Kernloch fuer M3 in PETG
pilot_len   = 7.5;
screw_d     = 3.4;   // Durchgang M3 in der Rueckwand
cbore_d     = 6.2;   // Senkung Schraubenkopf
cbore_t     = 1.8;

// ---------------------------------------------------------------- Blende (bezel) und Nut
// Die Blende liegt AUF dem Gesicht (1,2 dick) und rastet mit einem umlaufenden Steg in eine Nut. Eine flaechige Mulde waere bei
// "Gesicht nach unten" eine freitragende Flaeche von ca. 12 mm: nicht ohne Stuetzen druckbar. Die Nut ist nur 2 mm breit.
bz_hx       = 43;    // Blende Halbbreite (Blende 86 x 60, hell zwischen Fenster und Zierschrauben)
bz_hy       = 30;    // Blende Halbhoehe
bz_r        = 8;     // Blende Eckradius
bezel_t     = 1.2;   // QUELLE: Aufgabe: Rahmen 1,2 mm dick
nut_t       = 0.8;   // QUELLE: Aufgabe: Tiefe der Mulde/Nut 0,8
ridge_w     = 2.0;   // Stegbreite der Blende
ridge_in    = 1.5;   // Abstand des Stegs vom Blendenrand

// ---------------------------------------------------------------- Zierschrauben (als Einlage, Teil "deco")
deco_d      = 4;     // QUELLE: Aufgabe: Zierschraube Kopf Ø 4
deco_h      = 0.6;   // QUELLE: Aufgabe: 0,6 erhaben
deco_in     = 0.8;   // Einsatztiefe in der Frontplatte (Frontplatte behaelt 1,2)
deco_pts    = [for (sx = [-1, 1], sy = [-1, 1]) [sx * 46, sy * 32]];  // ausserhalb der Blende, in der Flaeche des Gesichts

// ---------------------------------------------------------------- Ohren
ear_d       = 30;    // QUELLE: Aufgabe: Scheibe Ø 30
ear_t       = 10;    // QUELLE: Aufgabe: 10 dick
peg_d       = 6;     // QUELLE: Aufgabe: Zapfen Ø 6
peg_l       = 6;     // QUELLE: Aufgabe: Zapfen 6 lang (Loch 6,3 tief)
peg_flat    = 0.5;   // Abflachung gegen Verdrehen (Tiefe)
peg_ch      = 0.5;   // QUELLE: Aufgabe: 0,5er Fase
ear_screw_d = 8;     // QUELLE: Aufgabe: Zierschraube Ø 8
ear_deco_in = 1.0;   // Einsatztiefe in der Ohrscheibe
ear_deco_h  = 0.8;   // erhaben (VORLÄUFIG, Geschmack)
ear_y       = 22;    // Ohr-Mitte Y: so weit oben, dass die Scheibe (Ø30) die USB-C-Oeffnung (Y bis +5) freilaesst
boss_r_extra = 2.0; // Steg ums Ohrloch: Radius = Tropfenspitze + 2 mm (Wand >= 2)

// ---------------------------------------------------------------- Hals (Schnittstelle zu Task 3)
neck_d       = 22;   // QUELLE: Spec: Rohr Ø 22
neck_flange_d = 24;  // VORLÄUFIG (mit Task 3 abstimmen): Ø des Halsflansches
neck_bolt_x  = 8;    // VORLÄUFIG (mit Task 3 abgestimmt): 2 M3-Kernloecher bei X = +-8; Flansch bekommt ein Langloch 8 x 12, Wand aussen/innen je 2,3
cable_d      = 12;   // Kabelloch (>= 10, JST-Stecker passt hindurch)
pad_h        = 11;   // Halssockel steht so weit unten aus dem Kopf (Kernloch 7,6 + Restboden im Material)
neck_pilot_len = 7.6; // Kernloch M3 im Material (>= 7,5)
bore_t       = 3;  // Tiefe der Flanschbohrung im Sockel
neck_pt      = [0, -8, leg_h + 50];  // QUELLE: Konzept: Anschlusspunkt des Kopfes in der Weltlage (Mitte Sockelflaeche) = leg_h - 4 + t_h (48) + neck_h (6); folgt leg_h, die Form des Oberbaus bleibt gleich

// ================================================================= abgeleitete Werte
a_lip     = (wall - fit) / 2;                 // Aussenhaelfte der Wand im Falzbereich
hx        = head_w / 2;
hy        = head_h / 2;
hx_in     = hx - wall;
z_split   = z_pcb_bot;
z_floor   = z_rear - rear_clear - rear_air;
z_back    = z_floor - back_t;
dome_y    = pcb_h / 2 + fit + dome_d / 2 + 0.1;
dome_pts  = [for (sx = [-1, 1], sy = [-1, 1]) [sx * dome_x, sy * dome_y]];
z_neck    = (z_back + z_split) / 2;           // Hals-Mitte in Z (Konstruktionslage)
pad_w     = neck_flange_d + fit + 4;       // 2 mm Rand links/rechts der Flanschbohrung
ear_z     = z_back / 2;                       // Ohr-Mitte in Z: Mitte der Kopftiefe
// Innenflaeche folgt der 45-Grad-Fase parallel (Wanddicke senkrecht = wall): Einzug d(z) = ch + z + wall*sqrt(2)
d_cf      = ch - front_t + wall * sqrt(2);    // Einzug des Hohlraums unter der Frontplatte
z_cf      = wall - ch - wall * sqrt(2);       // ab hier ist der Einzug wieder = wall
d_bf      = ch - back_t + wall * sqrt(2);     // Einzug des Hohlraums ueber der Rueckplatte
z_bf      = z_back + ch + wall * (sqrt(2) - 1); // ab hier ist der Einzug wieder = wall
hole_ear  = peg_d + fit_peg;
assert(hx_in >= pcb_x1 + sock_over + usb_clear, "Kopf zu schmal fuer die Platine");
bore_d    = neck_flange_d + fit;
bore_roof = bore_d / 2 * sqrt(2) - 6;         // Hoehe der gekappten Tropfenspitze ueber der Mitte (Dachbreite 12)
assert((z_split - z_back) / 2 - bore_d / 2 >= 2, "Halssockel: unter/neben der Bohrung in Z weniger als 2 mm Wand");
assert((z_split - z_back) / 2 - bore_roof >= 2, "Halssockel: Dach ueber der Bohrung unter 2 mm");
assert(pad_w / 2 - bore_d / 2 >= 2, "Halssockel: neben der Bohrung in X weniger als 2 mm Wand");
assert(pad_h - bore_t + wall - 0.5 >= neck_pilot_len + 1.5, "Halssockel: Kernloch zu lang fuer das Material (Restboden < 1,5 mm)");
assert(boss_r_extra >= 2, "Ohrsteg: Wand unter 2 mm");

// ================================================================= 2D-Formen
module outer2d() { rrect(hx, hy, head_r); }
module inset(d) { offset(r = -d) outer2d(); }
module ring2d() { difference() { rrect(bz_hx - ridge_in, bz_hy - ridge_in, bz_r - ridge_in); rrect(bz_hx - ridge_in - ridge_w, bz_hy - ridge_in - ridge_w, bz_r - ridge_in - ridge_w); } }
// Tropfenform: Loch mit 45-Grad-Dach (Spitze nach +v), druckbar ohne Stuetzen. clipw > 0: Spitze auf diese Dachbreite kappen.
module tear2d(d, clipw = 0) {
  r = d / 2;
  intersection() {
    union() { circle(d = d); rotate(45) square(r); }
    if (clipw > 0) translate([-d, -d]) square([2 * d, d + r * sqrt(2) - clipw / 2]);
  }
}

// Extrusion mit exakter 45-Grad-Fase (konvexe Form: Huelle zweier Platten)
module chamf_solid(z0, z1, c, at_top = true) {
  if (at_top) hull() {
    translate([0, 0, z0]) linear_extrude(z1 - z0 - c) outer2d();
    translate([0, 0, z1 - 0.001]) linear_extrude(0.001) inset(c);
  } else hull() {
    translate([0, 0, z0 + c]) linear_extrude(z1 - z0 - c) outer2d();
    translate([0, 0, z0]) linear_extrude(0.001) inset(c);
  }
}

// ================================================================= Teile (Konstruktionslage)
module usb_cuts() {
  for (u = usb_list) if (u[4])
    translate([pcb_x1 - 3, u[0] - u[2] / 2, usb_zc - usb_open_h / 2])
      cube([hx + 3 - pcb_x1 + 1, u[2], usb_open_h]);
}

module front_cavity() {
  hull() {
    translate([0, 0, z_split - 0.01]) linear_extrude(z_cf - z_split + 0.01) inset(wall);
    translate([0, 0, -front_t - 0.01]) linear_extrude(0.01) inset(d_cf);
  }
}

module head_front() {
  difference() {
    union() {
      difference() {
        chamf_solid(z_split, 0, ch, true);
        // Falz-Aussparung (Aussenhaelfte der Wand bleibt)
        translate([0, 0, z_split - 0.01]) linear_extrude(lip_h + 0.01) offset(r = -a_lip) outer2d();
        front_cavity();
      }
      // Schraubdome: von der Frontplatte bis zur Innenseite der Rueckwand
      for (p = dome_pts) translate([p[0], p[1], z_floor]) cylinder(d = dome_d, h = -front_t / 2 - z_floor);
    }
    window_cut();
    // Nut fuer den Steg der Blende (2 mm breit, Bruecke 2 mm)
    translate([0, 0, -nut_t]) linear_extrude(nut_t + 0.01) offset(delta = fit_peg) ring2d();
    // Einsaetze fuer die Zierschrauben
    for (p = deco_pts) translate([p[0], p[1], -deco_in]) cylinder(d = deco_d + fit_peg, h = deco_in + 0.01);
    usb_cuts();
    for (p = dome_pts) translate([p[0], p[1], z_floor - 0.01]) cylinder(d = pilot_d, h = pilot_len);
  }
}

// Fenster der Blende: setzt die Fase von window_cut() (win_bevel) durch die Blendendicke fort
module bezel_window() {
  s = tan(win_bevel);
  translate([disp_x + win_off_x, win_off_y, 0]) hull() {
    translate([0, 0, -nut_t - 0.01]) linear_extrude(0.01) rrect(win_w / 2 + (-nut_t + front_t) * s, win_h / 2 + (-nut_t + front_t) * s, win_r);
    translate([0, 0, bezel_t]) linear_extrude(0.01) rrect(win_w / 2 + (bezel_t + front_t + 0.01) * s, win_h / 2 + (bezel_t + front_t + 0.01) * s, win_r);
  }
}
module bezel() {
  difference() {
    union() {
      linear_extrude(bezel_t) rrect(bz_hx, bz_hy, bz_r);
      translate([0, 0, -nut_t + 0.1]) linear_extrude(nut_t - 0.1 + 0.01) ring2d();   // Steg, 0,1 kuerzer als die Nut tief ist
    }
    bezel_window();
  }
}

module ear_boss_solid() {
  // Steg um das Ohrloch: Huelle aus Rohr und Fusspunkt auf der Rueckplatte (keine Ueberhaenge)
  bd = 2 * (hole_ear / 2 * sqrt(2) + boss_r_extra);
  x0 = hx - peg_l - 0.3 - 1.5;
  hull() {
    translate([x0, ear_y, ear_z]) rotate([0, 90, 0]) cylinder(d = bd, h = hx_in - x0 + 0.5);
    translate([x0, ear_y - bd / 2, z_floor - 0.01]) cube([hx_in - x0 + 0.5, bd, 0.01]);
  }
}
module ear_hole() {
  // Loch mit Abflachung unten (-Z) und 45-Grad-Dach oben (+Z)
  translate([hx - peg_l - 0.3, ear_y, ear_z]) rotate([90, 0, 90]) linear_extrude(peg_l + 0.3 + 1)
    intersection() {
      tear2d(hole_ear);
      translate([-hole_ear, -(hole_ear / 2 - peg_flat)]) square([2 * hole_ear, 2 * hole_ear]);
    }
}

module back_cavity() {
  hull() {
    translate([0, 0, z_floor]) linear_extrude(0.01) inset(d_bf);
    translate([0, 0, z_bf]) linear_extrude(z_split + 0.01 - z_bf) inset(wall);
  }
}

module head_back(pilots = true) {
  difference() {
    union() {
      difference() {
        chamf_solid(z_back, z_split, ch, false);
        back_cavity();
      }
      // Falz
      translate([0, 0, z_split - 0.01]) linear_extrude(lip_h - 0.2 + 0.01)
        difference() {
          offset(r = -(a_lip + fit)) outer2d();
          offset(r = -wall) outer2d();
          for (p = dome_pts) translate(p) circle(d = dome_d + 0.5);
        }
      // Platinen-Andruckfuesse mit Zentrierstift (wie Stufe 2)
      for (p = hole_pts) translate([p[0], p[1], z_floor - 0.01]) {
        cylinder(d = 6.4, h = z_pcb_bot - 0.1 - z_floor + 0.01);
        cylinder(d = hole_d - 0.4, h = z_pcb_bot - 0.1 - z_floor + 0.01 + (pcb_t - 0.2));
      }
      // Ohr-Stege innen
      for (m = [0, 1]) mirror([m, 0, 0]) ear_boss_solid();
      // Halssockel unten: Klotz von der Rueckplatte bis zur Trennebene (keine Ueberhaenge)
      translate([-pad_w / 2, -hy - pad_h, z_back]) cube([pad_w, pad_h + wall - 0.5, z_split - z_back]);
    }
    // Schrauben von hinten
    for (p = dome_pts) {
      translate([p[0], p[1], z_back - 1]) cylinder(d = screw_d, h = back_t + 2);
      translate([p[0], p[1], z_back - 0.01]) cylinder(d = cbore_d, h = cbore_t + 0.01);
    }
    usb_cuts();
    // BOOT/RESET
    translate([btn_x - btn_open_w / 2, -pcb_h / 2 + btn_y_from_bottom - btn_open_h / 2, z_back - 1])
      cube([btn_open_w, btn_open_h, back_t + 2]);
    // Ohrloecher
    for (m = [0, 1]) mirror([m, 0, 0]) ear_hole();
    // Halssockel: Flanschbohrung (Dach auf 12 mm gekappt), Kabelloch, 2 M3-Kernloecher
    translate([0, -hy - pad_h + bore_t, z_neck]) rotate([90, 0, 0]) linear_extrude(bore_t + 0.01)
      tear2d(neck_flange_d + fit, 12);
    translate([0, -hy + wall + 1, z_neck]) rotate([90, 0, 0]) linear_extrude(wall + pad_h + 2) tear2d(cable_d);
    if (pilots) neck_pilots();
  }
}
// 2 M3-Kernloecher: ab Boden der Flanschbohrung nach +Y ins Material (Richtung Kopfinneres)
module neck_pilots() {
  for (sx = [-1, 1]) translate([sx * neck_bolt_x, -hy - pad_h + bore_t - 0.01, z_neck]) rotate([-90, 0, 0])
    cylinder(d = pilot_d, h = neck_pilot_len + 0.01);
}

module ear() {
  // Konstruktionslage des Ohrs: aussen z = 0 (Zierschraube erhaben nach -z), Scheibe z 0..ear_t, Zapfen z ear_t..ear_t+peg_l,
  // Abflachung am Zapfen bei lokal -X
  difference() {
    union() {
      hull() {
        cylinder(d = ear_d - 2, h = 0.01);
        translate([0, 0, 1]) cylinder(d = ear_d, h = ear_t - 1);
      }
      intersection() {
        translate([0, 0, ear_t - 0.01]) union() {
          cylinder(d = peg_d, h = peg_l - peg_ch + 0.01);
          translate([0, 0, peg_l - peg_ch]) cylinder(d1 = peg_d, d2 = peg_d - 2 * peg_ch, h = peg_ch);
        }
        translate([-(peg_d / 2 - peg_flat), -peg_d, ear_t - 1]) cube([2 * peg_d, 2 * peg_d, peg_l + 2]);
      }
    }
    translate([0, 0, -0.01]) cylinder(d = ear_screw_d + fit_peg, h = ear_deco_in + 0.01);
  }
}
module ear_deco()  { cylinder(d = ear_screw_d, h = ear_deco_in + ear_deco_h - 0.2); translate([0, 0, ear_deco_in + ear_deco_h - 0.2]) cylinder(d1 = ear_screw_d, d2 = ear_screw_d - 0.4, h = 0.2); }
module head_deco() { cylinder(d = deco_d, h = deco_in + deco_h - 0.2); translate([0, 0, deco_in + deco_h - 0.2]) cylinder(d1 = deco_d, d2 = deco_d - 0.4, h = 0.2); }

// Einbaulage der beiden Ohren (Konstruktionslage des Kopfes). Rechts: Zapfen zeigt nach -X, links nach +X, Abflachung beide nach -Z.
module ear_xf_right() { translate([hx + ear_t, ear_y, ear_z]) rotate([0, -90, 0]) children(); }
module ear_xf_left()  { translate([-(hx + ear_t), ear_y, ear_z]) rotate([0, 90, 0]) rotate([0, 0, 180]) children(); }
module ear_right() { ear_xf_right() ear(); }
module ear_left()  { translate([-(hx + ear_t), ear_y, ear_z]) rotate([0, 90, 0]) rotate([0, 0, 180]) ear(); }
module ears() { ear_right(); ear_left(); }
// Zierschrauben in Einbaulage: Kopf (Gesichtsebene) und Ohren
module decos() {
  for (p = deco_pts) translate([p[0], p[1], -deco_in]) head_deco();
  ear_xf_right() ear_deco_in_ear();
  ear_xf_left() ear_deco_in_ear();
}
module ear_deco_in_ear() { translate([0, 0, ear_deco_in]) mirror([0, 0, 1]) ear_deco(); }

// ================================================================= Pruefkoerper (Dummies) in Konstruktionslage
module plugs_dummy() {
  // nur dort, wo eine Gehaeuseoeffnung ist
  for (u = usb_list) if (u[4])
    translate([pcb_x1 + sock_over, u[0] - u[3] / 2, usb_zc - plug_h / 2]) cube([hx + 25, u[3], plug_h]);
}
module head_dummies() {
  pcb_dummy(); glass_dummy(); rear_dummy(); sockets_dummy(); plugs_dummy(); buttons_dummy();
  active_cone(); sd_dummy();
}
module head_shells() { head_front(); bezel(); head_back(); ears(); decos(); }

// ================================================================= Weltlage
// Konstruktionslage -> Welt: Sockelmitte (0, -hy-pad_h, z_neck) auf neck_pt, Gesicht nach -Y, um head_tilt nach hinten geneigt
module place_head() {
  translate(neck_pt) rotate([-head_tilt, 0, 0]) rotate([90, 0, 0]) translate([0, hy + pad_h, -z_neck]) children();
}
function head_to_world(p) = let(q = [p[0], -(p[2] - z_neck), p[1] + hy + pad_h], c = cos(head_tilt), s = sin(head_tilt))
  neck_pt + [q[0], q[1] * c + q[2] * s, -q[1] * s + q[2] * c];

// HALSACHSE FUER TASK 3: Achse und Flanschebene des Halses sind um head_tilt gegen die Senkrechte geneigt (Kopf lehnt nach hinten),
// weil der Sockel fest am Kopf sitzt. neck_axis_w zeigt vom Sockel in den Kopf (nach oben und um head_tilt nach hinten, +Y).
// Die Flanschebene steht senkrecht auf dieser Achse und geht durch neck_pt. Der Hals unten (Oberkoerper) ist dagegen waagerecht:
// der Hals braucht entweder eine Schraegung/Knick oder der Kopf-Flansch wird gegen den Oberkoerper-Flansch um head_tilt gekippt.
neck_axis_w = [0, sin(head_tilt), cos(head_tilt)];
// Lokales Koordinatensystem am Halsflansch: Ursprung neck_pt, lokales +Z = neck_axis_w, lokales Y = Weltrichtung "hinten" in der Flanschebene.
module neck_frame() { translate(neck_pt) rotate([-head_tilt, 0, 0]) children(); }

// ================================================================= Oberkoerper, Hals, Arme, Haende (Task 3)
// Oberkoerper-Konstruktionslage = Weltlage um torso_origin verschoben (place_torso): X rechts, Y nach hinten, Z oben.
// Lokaler Ursprung = Mitte der Unterkante. Teilungsebene y = 0 (Mitte): vorn = Brust mit Kamera, hinten = Rueckwand.
// MONTAGEFOLGE (jede Schraube ist per Prueckoerper nachgewiesen erreichbar, check_roboter.ps1 Schritt 3g):
//   Phase 1: Oberkoerper vorn + hinten sind verschraubt, der Kopf fehlt. Den Hals von OBEN mit 2 M3 auf den Oberkoerper schrauben.
//            Beide Schrauben (neck_lo_bx, neck_lo_by) sitzen mit ihren Kernloechern vollstaendig in torso_front: das Abnehmen der Rueckwand loest den Hals nicht.
//   Phase 2: Rueckwand des Oberkoerpers ab, XIAO noch nicht eingesetzt, Oberkoerper noch nicht auf dem Sitz. Kopf auf den Hals setzen und die
//            2 Kopfschrauben (neck_bolt_x) von UNTEN entlang der Halsachse durch die Seitenkanaele von Hals und Oberkoerper (drv_channel) eindrehen.
//   Phase 3: XIAO von hinten in die Brust schieben, Rueckwand schliessen, Oberkoerper auf den Sitz stecken und von unten verschrauben.
t_w        = 64;     // QUELLE: Konzept (t_w): Breite
t_d        = 44;     // QUELLE: Konzept (t_d): Tiefe
t_h        = 48;     // QUELLE: Konzept (t_h): Hoehe
t_r        = 10;     // QUELLE: Konzept (t_r): Eckradius oben (Frontansicht)
t_y        = 14;     // QUELLE: Konzept (t_y): Rueckseite des Oberkoerpers
// leg_h steht oben bei "Allgemein" (der Kopf-Anschlusspunkt neck_pt haengt davon ab)
t_ch       = 3.5;    // VORLÄUFIG (Entwurf): 45-Grad-Fase an Brust und Rueckseite, ohne Stuetzen druckbar
torso_origin = [0, t_y - t_d / 2, leg_h - 4];   // QUELLE: Konzept: [0, t_y - t_d/2, leg_h - 4] = Mitte der Unterkante des Oberkoerpers in Weltlage (Task 4 baut darauf auf)
t_plate_z  = 4;      // VORLÄUFIG (Entwurf): Bodenplatte 4 mm ueber der Unterkante, darunter die Steckkante (Rand), in die der Sitz (Task 4) greift
t_cable_d  = 10;     // QUELLE: Aufgabe: Kabeldurchfuehrung unten >= 10
t_dome_pts = [[-16, 8], [16, 8], [-16, 41], [16, 41]];   // VORLÄUFIG (Entwurf): M3-Dome (Teilung vorn/hinten), Lage X/Z; untere Dome von Z 10 auf 8 gesenkt, damit die XIAO-Wiege (Z ab 12,3) frei bleibt
t_dome_d   = 7.4;    // VORLÄUFIG (Entwurf): Dom aussen; Wand um die Durchgangsbohrung (Ø3,4) 2,0
t_dome_pilot = 9;    // VORLÄUFIG (Entwurf): Kernloch im vorderen Dom (>= 7,5)
t_seat_pts = [for (sx = [-1, 1], sy = [-1, 1]) [sx * 24, sy * 12]];   // VORLÄUFIG (Entwurf): 4 M3-Kernloecher unten fuer den Sitz (X/Y), Schrauben von unten
t_seat_pilot = 8.0;  // VORLÄUFIG (Entwurf): Kernloch M3 (>= 7,5); 8,0 damit die Sitzschraube M3x10 (Spitze bei 7,5 ab Plattenunterseite) 0,5 mm Reserve hat; Restboden 1,5
t_seat_blk_z = 13.5; // VORLÄUFIG (Entwurf): Oberkante der Sitzpfosten ueber der Unterkante
t_deco_pts = [for (sx = [-1, 1], z = [12, 40.8]) [sx * 23, z]];   // QUELLE: Konzept: Brust-Schrauben (Einlagen) bei 0,25 und 0,85 t_h
// --- Kamera / XIAO: einfach gehalten und parametrisch, die Halterung wird nach Task 4 neu bestimmt
cam_tilt   = 15;     // QUELLE: Aufgabe: Kamera nach oben gekippt
cam_z      = 27;     // VORLÄUFIG (gerechnet): Linsenhoehe ueber der Unterkante (Konzept 0,6 * t_h = 28,8). Der gekippte Stapel (15 Grad, 21 mm Tiefe) belegt Z cam_z-14,8 ... cam_z+6,3: oben bleiben 1,7 mm zum Halsblock (Z 35), unten 0,5 mm ueber den unteren Domen (Oberkante Z 11,7)
// Linsenloch-Fase im Roboter: 45 Grad zur Achse (QUELLE: Spec 45 Grad; Entscheidung 2026-10-28: Stuetzen am Linsenloch beim Druck von torso_front akzeptiert, die Fase ist
// die einzige Ausnahme der Ueberhang-Pruefung, siehe check_roboter.ps1 Schritt 3j). cam_chamfer_ang bleibt ungesetzt (Vorgabe 45 in xiao_cam_block.scad).
cam_cone_l = 26;     // QUELLE: Entscheidung Auftraggeber 2026-10-28 (Spec: Roboter 26 mm statt 30 mm). Begruendung (gerechnet): Laenge des Sichtkegels (Spec: 30). Die Fuesse beginnen bei Y -62, 32 mm vor der Linse: der untere Bildrand (102 Grad diagonal) trifft sie ab etwa 27 mm Abstand; bis 26 mm ist die Sicht frei
// Kameratasche, XIAO-Wiege und Pruefkoerper (Block in Lieferlage, v2) kommen aus xiao_cam_block.scad (lokales System: Innenseite der Wand = Z 0, Blick +Z, Linse = Ursprung).
// Dort definiert (hier NICHT wiederholen): cam_*, xiao_* ... Die Common-Datei liest von der Roboter-Datei nur die oben gesetzten, optionalen Werte (z. B. cam_chamfer_ang); sie muessen VOR dem include stehen.
include <xiao_cam_block.scad>
cam_wt     = 3.9;    // QUELLE: Spec (Linse 3,6 + 0,3 Rueckstand): "Wandstaerke" fuer das lokale System = Abstand Innenebene (Z 0) bis Aussenebene (Z cam_wt) entlang der Linsenachse. Die Brustwand steht senkrecht, die Achse ist 15 Grad geneigt: die Aussenflaeche liegt bei Z = cam_wt + 0,268 * (Hoehe ueber der Achse). Mit 3,9 liegt die Kamera-Vorderseite (cam_zbf) auf Z 0 und die Linsenspitze in der Achse 0,3 mm hinter der Aussenflaeche; der Rest zwischen Z 0 und der Aussenflaeche wird von cam_fill() massiv gefuellt (Taschenboden)
cam_tip_ref = cam_lens_h;   // Pruefmass fuer lens_tip: Linsenspitze der Konstruktion (Kamera-Vorderseite auf Z 0, Linse 3,6 hoch); unabhaengig von cam_wt, damit eine zu duenne Wand auffaellt
// --- Hals
neck_lo_d  = 36;     // VORLÄUFIG (Entwurf): Ø unterer Flansch (Scheibe), dazu zwei Laschen fuer die Schrauben
neck_lug_d = 9;      // VORLÄUFIG (Entwurf): Ø der Laschen am unteren Flansch (Wand um das Loch 2,8)
neck_lo_bx = 16;     // VORLÄUFIG (Entwurf): untere Halsschrauben bei X = +-16 ...
neck_lo_by = -13;    // VORLÄUFIG (Entwurf): ... und Y = -13 (beide vor der Teilungsebene, also in torso_front)
neck_fl_t  = 2.8;    // VORLÄUFIG (Entwurf): Flanschdicke (Bohrung im Kopfsockel ist 3 tief, 0,2 Luft)
neck_bore  = 12;     // QUELLE: Aufgabe: Innen-Ø >= 12
neck_slot_w = 8;     // VORLÄUFIG (Entwurf): Langloch im oberen Flansch, 8 in X
neck_slot_l = 12;    // VORLÄUFIG (Entwurf): Langloch im oberen Flansch, 12 in Y
neck_lobe_d = 6.6;   // VORLÄUFIG (Entwurf): Werkzeugkanal fuer die Kopfschrauben (Schraubendreher Ø6 + 0,6)
drv_d      = 6;      // QUELLE: Aufgabe: Schraubendreher-Pruefkoerper Ø6
drv_l      = 60;     // QUELLE: Aufgabe: Schraubendreher-Pruefkoerper 60 lang
screw_head_h = 3;    // VORLÄUFIG (Datenblatt): M3-Linsenkopf, Hoehe
neck_rec_floor = 45; // VORLÄUFIG (Entwurf): Boden der Flanschmulde im Oberkoerper (Mulde 3 tief)
neck_lo_z  = neck_rec_floor + 0.05;   // VORLÄUFIG (Entwurf): 0,05 Luft zum Muldenboden
neck_blk_z = 35;     // VORLÄUFIG (Entwurf): Unterkante des Halsblocks im Oberkoerper
block_hw   = 24;     // VORLÄUFIG (Entwurf): Halbbreite des Halsblocks (Mulde + Laschen 20,65 + Wand >= 2)
neck_pilot_t = 7.6;  // VORLÄUFIG (Entwurf): Kernloch M3 im Halsblock (>= 7,5)
// --- Arm / Hand (Weltlage)
arm_x      = 40.5;   // VORLÄUFIG (Entwurf): Armebene |x|; Schulterkugel 1 mm neben der Seitenwand (Konzept 37, dort ragte die Kugel in den Koerper)
ball_r     = 8.5;    // QUELLE: Konzept: Schulterkugel R 8,5
ball_in    = 7.5;    // VORLÄUFIG (Entwurf): Abflachung innen (zum Koerper)
arm_flat   = 4.95;   // VORLÄUFIG (gerechnet): Abflachung aussen = Druckflaeche (45-Grad-Flanke, der Arm druckt auf der Aussenseite)
arm_r1     = 7;      // QUELLE: Konzept: Arm-Hull r 7 an der Schulter
arm_r2     = 6;      // QUELLE: Konzept: r 6 an der Hand
hand_r     = 8;      // QUELLE: Konzept: Hand scale([1,1.3,0.8]) sphere(r=8)
hand_sc    = [1, 1.3, 0.8];   // QUELLE: Konzept
hand_cut   = 3.3;    // VORLÄUFIG (gerechnet): Abflachung unten (Hand liegt auf dem Oberschenkel, Flanke >= 45 Grad)
hand_contact_z = leg_h;   // QUELLE: Konzept: leg_h = Oberflaeche der Oberschenkel unter den Haenden (Task 4)
hand_c_y   = -31;    // VORLÄUFIG (Entwurf): Hand-Mitte Y (Weltlage)
arm_S      = [arm_x, 0, hand_contact_z + hand_cut + 31];       // VORLÄUFIG (Entwurf): Schulter, Welt (rechter Arm); Arm 45 Grad nach vorn unten
arm_H      = [arm_x, hand_c_y, hand_contact_z + hand_cut];     // VORLÄUFIG (Entwurf): Hand-Mitte, Welt (rechter Arm)
arm_d      = [0, -sqrt(0.5), -sqrt(0.5)];                      // Achsrichtung Schulter -> Hand (45 Grad)
arm_peg_d  = 5;      // QUELLE: Aufgabe: Zapfen Ø 5 fuer die Hand
arm_s_end  = -9;     // VORLÄUFIG (gerechnet): Armende, Abstand zur Hand-Mitte entlang der Achse (Ellipsoid-Stuetzwert 8,63 + 0,37)
arm_peg_tip = -1.3;  // VORLÄUFIG (Entwurf): Zapfenspitze, Abstand zur Hand-Mitte (Zapfen 7,7 lang)
hand_hole_end = -1;  // VORLÄUFIG (gerechnet): Ende des Zapfenlochs (Wand zur Unterseite >= 2)
arm_sh_peg_l = 7;    // QUELLE: Aufgabe: Schulterzapfen Ø 6 (peg_d), >= 6 lang ab Innenflaeche
sh_bore_depth = 6.5; // VORLÄUFIG (Entwurf): Tiefe der Schulterbohrung ab Aussenseite
sh_y_l     = arm_S[1] - torso_origin[1];   // abgeleitet: Schulter lokal Y
sh_z_l     = arm_S[2] - torso_origin[2];   // abgeleitet: Schulter lokal Z
side       = 1;      // Druckzweig arm/hand: 1 = rechts, -1 = links (gespiegelt)

d_cf_t = t_ch - wall + wall * sqrt(2);
y_cf_t = t_ch + wall * (sqrt(2) - 1);
lo_lug_hw = neck_lo_bx + neck_lug_d / 2 + fit / 2;      // Halbbreite der Flanschmulde (mit Laschen)
// ---- Wandbilanz (Ziel >= 2 mm)
assert(t_cable_d >= 10, "Kabeldurchfuehrung unten < 10");
assert(neck_bore >= 12, "Hals Innen-Ø < 12");
assert(neck_rec_floor - neck_pilot_t - neck_blk_z >= 1.5, "Halsblock: Restboden unter Kernloch < 1,5");
assert(neck_pilot_t >= 7.5, "Halsblock-Kernloch < 7,5");
assert(t_seat_blk_z - t_plate_z - t_seat_pilot >= 1.5, "Sitzpfosten: Kernloch zu lang");
assert((7 - pilot_d) / 2 >= 2, "Sitzpfosten: Wand um das Kernloch unter 2 mm");
assert(t_d / 2 - wall >= t_dome_pilot + 1.5, "Dom: Kernloch zu lang");
assert(t_dome_pilot >= 7.5, "Dom-Kernloch < 7,5");
assert((t_dome_d - screw_d) / 2 >= 2, "Dom: Wand um die Durchgangsbohrung unter 2 mm");
assert((t_dome_d - pilot_d) / 2 >= 2, "Dom: Wand um das Kernloch unter 2 mm");
assert(sh_y_l - hole_ear / 2 * sqrt(2) - 1.0 >= 2, "Schulterbohrung: Wand zur Trennebene unter 2 mm");
assert(neck_d / 2 - neck_bore / 2 >= 2, "Halsrohr: Wand unter 2 mm");
assert(arm_x - ball_in > t_w / 2, "Schulterkugel ragt in den Oberkoerper");
assert(arm_sh_peg_l >= 6 && arm_peg_tip - arm_s_end >= 6, "Zapfen kuerzer als 6 mm");
// oberer Halsflansch: Wand aussen und innen (Langloch) um das Schraubenloch
assert(neck_flange_d / 2 - (neck_bolt_x + screw_d / 2) >= 2, "oberer Flansch: Wand aussen unter 2 mm");
assert((neck_bolt_x - screw_d / 2) - neck_slot_w / 2 >= 2, "oberer Flansch: Wand zwischen Langloch und Schraubenloch unter 2 mm");
assert(neck_slot_l >= neck_bore, "Langloch kuerzer als die Bohrung");
// unterer Halsflansch: Wand um die Laschenloecher, Kopf-Freiheit zum Rohr
assert((neck_lug_d - screw_d) / 2 >= 2, "unterer Flansch: Wand um das Schraubenloch unter 2 mm");
assert(norm([neck_lo_bx, neck_lo_by]) - screw_d / 2 - neck_bore / 2 >= 2, "unterer Flansch: Wand zur Bohrung unter 2 mm");
assert(norm([neck_lo_bx, neck_lo_by]) - 3.1 - neck_d / 2 >= 0, "untere Halsschraube: Kopf stoesst ans Rohr");
assert(neck_lo_by + neck_lug_d / 2 < 0, "untere Halsschraube liegt nicht vor der Teilungsebene");
// Halsblock: Wand um Mulde und Laschen (Dach), keine Schlitze
assert(block_hw - lo_lug_hw >= 2, "Halsblock: Wand neben der Mulde unter 2 mm");
assert(t_d / 2 - (-neck_lo_by + neck_lug_d / 2 + fit / 2) >= 2, "Halsblock: Wand vor der Mulde unter 2 mm");
assert(18.5 - (-neck_lo_by + neck_lug_d / 2 + fit / 2) >= 0, "Mulde ragt in die Fase");
assert(22 - lo_lug_hw >= 0, "Mulde ragt in die Eckrundung");
assert(norm([neck_lo_bx - neck_bolt_x, neck_lo_by]) > neck_lug_d / 2 + neck_lobe_d / 2 + 2, "untere Halsschraube zu nah am Werkzeugkanal");

module place_torso() { translate(torso_origin) children(); }
module t_out_low() { hull() { translate([-t_w / 2, -10]) square([t_w, 11]); for (sx = [-1, 1]) translate([sx * (t_w / 2 - t_r), t_h - t_r]) circle(r = t_r); } }
module t_outline() { hull() { translate([-t_w / 2, 0]) square([t_w, 1]); for (sx = [-1, 1]) translate([sx * (t_w / 2 - t_r), t_h - t_r]) circle(r = t_r); } }
module t_inset(d) { offset(r = -d) t_outline(); }
module t_cav2d(d) { offset(r = -d) t_out_low(); }
// 2D-Form (X/Z) entlang Y extrudieren: belegt y0..y1
module t_ext(y0, y1) { translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) children(); }
module t_solid() {
  hull() {
    t_ext(-t_d / 2 + t_ch, t_d / 2 - t_ch) t_outline();
    t_ext(-t_d / 2, -t_d / 2 + 0.001) t_inset(t_ch);
    t_ext(t_d / 2 - 0.001, t_d / 2) t_inset(t_ch);
  }
}
module t_cavity() {
  hull() for (m = [0, 1]) mirror([0, m, 0]) {
    t_ext(-t_d / 2 + wall - 0.01, -t_d / 2 + wall) t_cav2d(d_cf_t);
    t_ext(-t_d / 2 + y_cf_t - 0.01, -t_d / 2 + y_cf_t) t_cav2d(wall);
  }
}
module t_half(front) { intersection() { t_solid(); translate([-40, front ? -t_d / 2 - 1 : 0, -10]) cube([80, t_d / 2 + 1, 70]); } }
// Innenkontur des Steckrandes (XY-Schnitt bei z < t_plate_z) OHNE Spiel (fit nicht abgezogen): Task 4 zieht fit/2 je Seite ab, damit der Sitz hineingreift
module torso_pocket2d() {
  hxp = t_w / 2 - wall; hyp = t_d / 2 - wall;
  polygon([[t_w / 2 - d_cf_t, -hyp], [hxp, -(t_d / 2 - y_cf_t)], [hxp, t_d / 2 - y_cf_t], [t_w / 2 - d_cf_t, hyp],
           [-(t_w / 2 - d_cf_t), hyp], [-hxp, t_d / 2 - y_cf_t], [-hxp, -(t_d / 2 - y_cf_t)], [-(t_w / 2 - d_cf_t), -hyp]]);
}
// ---- Kamera (lokales System aus xiao_cam_block.scad, um cam_tilt nach oben gekippt)
// Ursprung der Achse: Punkt der Aussenflaeche der Brust (Y = -t_d/2) auf Hoehe cam_z; das System liegt cam_wt hinter dieser Stelle (Innenebene Z 0).
module cam_frame() { translate([0, -t_d / 2, cam_z]) rotate([90 - cam_tilt, 0, 0]) translate([0, 0, -cam_wt]) children(); }
module xiao_dummy_l()  cam_frame() xiao_stack_dummy(cam_wt);
module camera_dummy_l() cam_frame() cam_dummy(cam_wt);
module lens_cone_l()   cam_frame() cam_fov_cone(cam_cone_l, cam_wt);
module xiao_dummy_r()  { place_torso() xiao_dummy_l(); }
module camera_dummy_r() { place_torso() camera_dummy_l(); }
module lens_cone_r()   { place_torso() lens_cone_l(); }
module guide_c() { translate(-(torso_origin + [0, -t_d / 2, cam_z])) place_torso() children(); }   // nur Anzeige (Einbauanleitung): Kamera-Tasche in den Ursprung, damit die feste --camera sie mittig zeigt
// Massiver Block zwischen der Tasche und der (senkrechten) Aussenflaeche: die Wand ist senkrecht, die Achse geneigt, die Innenebene schneidet
// die Wand schraeg. torso_front schneidet das mit der Aussenhuelle ab (t_half), die Anschlagplatte wird so an jeder Stelle der Tasche massiv.
module cam_body_probe(grow) {   // Kamerakoerper + Linse (ohne FPC), um grow groesser, fuer die Montagewege
  translate([-(cam_body + grow) / 2, -(cam_body + grow) / 2, cam_zbb(cam_wt)]) cube([cam_body + grow, cam_body + grow, cam_body_t]);
  translate([0, 0, cam_zbf(cam_wt) - 0.01]) cylinder(d = cam_lens_d1 + grow, h = cam_lens_h1 + 0.01);
  translate([0, 0, cam_zbf(cam_wt) + cam_lens_h1 - 0.01]) cylinder(d = cam_lens_d2 + grow, h = cam_lens_h2 + 0.01);
}
module block_probe(narrow) intersection() {   // ganzer Block (Kamera + XIAO) fuer die Montagewege, in Y um je narrow schmaler (Lippen-Ueberdeckung)
  cam_block_dummy(cam_wt);
  translate([-100, xiao_vmin + narrow, -100]) cube([200, xiao_vmax - xiao_vmin - 2 * narrow, 200]);
}
module cam_fill() translate([-cam_outer_h, -cam_outer_h, 0.4]) cube([2 * cam_outer_h, 2 * cam_outer_h, cam_wt + 4]);
// Wiege-Pfosten und Anschlag liegen in Bloecken der Seeed-Koordinaten (u/v): Pfosten an den Laengsseiten (u xiao_post_u0 ... u1), der Anschlag mittig unter der XIAO-Unterseite.
// Der Block liegt (15 Grad geneigt) bei Y ca. -22 ... -4: die Pfosten gehoeren ganz zu torso_front. Der Anschlag kommt von der Rueckwand und reicht ueber die Teilungsebene in die
// Vorderhaelfte hinein (Falz-Prinzip): er gehoert nur zu torso_back und wird nicht an t_half geschnitten. Er ist ein Hull (Anschlagflaeche -> Rueckwand), seine Seitenflaechen
// liegen in Ebenen X = const bzw. Z = const (im Druck von torso_back senkrecht), die Anschlagflaeche ist oben (Druckrichtung -Y), kein Ueberhang.
stop_u0 = 0.5; stop_u1 = 7.5;      // VORLÄUFIG (Entwurf): Anschlag in u: mittig am USB-Ende, X -3,0 ... 4,0 -- zwischen den Werkzeugkanaelen (Kanalkanten X +-4,7), frei von den Loetstellen am Rand
stop_v0 = -4; stop_v1 = 5;         // VORLÄUFIG (Entwurf): Anschlag in v (die Platinenkanten mit den Loetstellen -8,25 / 9,53 bleiben frei)
module cam_post_roots() cam_frame() for (sy = [-1, 1]) {
  yin = sy > 0 ? xiao_vmax + xiao_play : xiao_vmin - xiao_play;
  translate([xiao_post_x(sy)[0], sy > 0 ? yin : yin - xiao_post_t, 0.4]) cube([xiao_post_x(sy)[1], xiao_post_t, cam_wt + 4]);   // Pfostenwurzeln bis zur Aussenhuelle (sonst waere der Pfostenanfang im Druck eine freitragende Flaeche)
}
module cam_stop() {   // Anschlag im Rueckteil: Anschlagflaeche xiao_stop_gap hinter der XIAO-Unterseite, Hull zur Rueckwand
  zw = xiao_zbot(cam_wt) - xiao_stop_gap;
  x0 = stop_u0 - xiao_u_lens; x1 = stop_u1 - xiao_u_lens; sx0 = xiao_flip ? -x1 : x0;
  zlo = cam_z + stop_v0 * cos(cam_tilt) + (zw - cam_wt) * sin(cam_tilt);   // Z (Oberkoerper) der Anschlag-Enden
  zhi = cam_z + stop_v1 * cos(cam_tilt) + (zw - cam_wt) * sin(cam_tilt);
  hull() {
    cam_frame() translate([sx0, stop_v0, zw - 0.01]) cube([x1 - x0, stop_v1 - stop_v0, 0.01]);
    translate([sx0, t_d / 2 - wall - 0.3, zlo]) cube([x1 - x0, 0.8, zhi - zlo]);
  }
}
module cam_stop_back() if (!cam_off) intersection() { t_solid(); cam_stop(); }   // nur fuer torso_back
cam_off     = false;  // nur Pruefhilfe (Ueberhang-Vergleich): true = ganze Kamera-Geometrie (Tasche, Wiege, Anschlag) entfaellt
module cam_posts() difference() { cam_frame() xiao_cradle_add(cam_wt); cam_free_volumes(); }   // Wiege-Pfosten: der Werkzeugkanal der Kopfschraube (X +8) schneidet das Ende des oberen Pfostens an (dort bleibt nur der Teil links der Kanalkante, siehe Bericht)
module cam_mount() { if (!cam_off) { cam_frame() { cam_pocket_add(cam_wt); cam_fill(); } cam_posts(); cam_post_roots(); } }   // positiv (ungeschnitten), nur torso_front
module cam_nofill() { cam_frame() cam_pocket_add(cam_wt); cam_posts(); cam_stop(); }   // Rahmen + Wiege + Anschlag ohne Fuellblock (fuer die Lage-Pruefungen)
module cam_free_volumes() { drv_channel(); translate([0, 0, t_plate_z - 1]) cylinder(d = t_cable_d, h = wall + 2); }   // Werkzeugkanaele + Kabelloch Ø10
module cam_parts_tol() for (d = [[0, 0, 0.08], [0, 0, -0.08], [0, 0.08, 0], [0, -0.08, 0], [0.08, 0, 0], [-0.08, 0, 0]]) translate(d) place_torso() { torso_front(); torso_back(); }   // torso_front + torso_back, je 0,08 mm in X/Y/Z verschoben vereint: schluckt nur Rundungs-Splitter (< 0,08 mm) der Schnittmenge in cam_present
module t_solid_in(d) hull() {   // Brusthuelle um ca. d nach innen verkleinert (45-Grad-Fase mitgefuehrt)
  t_ext(-t_d / 2 + t_ch + d, t_d / 2 - t_ch - d) t_inset(d);
  t_ext(-t_d / 2 + d, -t_d / 2 + d + 0.001) t_inset(t_ch + d);
  t_ext(t_d / 2 - d - 0.001, t_d / 2 - d) t_inset(t_ch + d);
}
module cam_cut() if (!cam_off) cam_frame() {   // negativ: Linsenloch 6,7 mit 45-Grad-Fase, Tasche, Nuten, FPC-Schlitz, Stapelraum
  cam_pocket_cut(cam_wt); xiao_cradle_cut(cam_wt);
  // Die Aussenflaeche steht senkrecht (nicht in der Linsen-Normalebene): oben ragt sie bis ca. 1,2 mm ueber Z cam_wt. Die Fase wird als Kegel ueber cam_wt hinaus weitergefuehrt.
  translate([0, 0, cam_wt]) cylinder(d1 = cam_hole_d + 2 * cam_chamfer, d2 = cam_hole_d + 2 * cam_chamfer + 8 * tan(cam_cone_ang), h = 4);   // Fase-Kegel (gleicher Flankenwinkel) ueber cam_wt hinaus
}


// ---- Einbauten
module t_adds() {
  for (p = t_dome_pts) translate([p[0], t_d / 2 - 0.5, p[1]]) rotate([90, 0, 0]) cylinder(d = t_dome_d, h = t_d - 1);
  translate([-block_hw, -t_d / 2, neck_blk_z]) cube([2 * block_hw, t_d, t_h - neck_blk_z]);   // Halsblock
  for (sx = [-1, 1]) translate([sx > 0 ? t_w / 2 - 8.5 : -t_w / 2, 1.0, sh_z_l - 5.1]) cube([8.5, t_d / 2 - 1.0 - 0.5, 10.2]); // Schulterbosse bis zur Rueckwand
  for (p = t_seat_pts) translate([p[0] - 3.5, p[1] < 0 ? -t_d / 2 + 0.5 : p[1] - 3.5, t_plate_z])
    cube([7, p[1] < 0 ? t_d / 2 - 0.5 + p[1] + 3.5 : t_d / 2 - 0.5 - p[1] + 3.5, t_seat_blk_z - t_plate_z]);
  cam_mount();
}
module neck_pilots_t()  { for (sx = [-1, 1]) translate([sx * neck_lo_bx, neck_lo_by, neck_rec_floor - neck_pilot_t]) cylinder(d = pilot_d, h = neck_pilot_t + 0.01); }
// unterer Halsflansch: Scheibe + 2 Laschen fuer die Schrauben (beide vor der Teilungsebene)
module neck_lo2d() { hull() { circle(d = neck_lo_d); for (sx = [-1, 1]) translate([sx * neck_lo_bx, neck_lo_by]) circle(d = neck_lug_d); } }
// Werkzeugkanal fuer die Kopfschrauben: gerade entlang der (gekippten) Halsachse, Ø6,6, von der Flanschunterseite nach unten durch Hals, Halsblock und Bodenplatte
module drv_channel() for (sx = [-1, 1]) neck_up() translate([sx * neck_bolt_x, 0, -80]) cylinder(d = neck_lobe_d, h = 80.05);
module seat_pilots_t()  { for (p = t_seat_pts) translate([p[0], p[1], t_plate_z - 0.01]) cylinder(d = pilot_d, h = t_seat_pilot + 0.01); }
module dome_pilots_t()  { for (p = t_dome_pts) translate([p[0], 0.01, p[1]]) rotate([90, 0, 0]) cylinder(d = pilot_d, h = t_dome_pilot + 0.01); }
module sh_bore2d() { intersection() { rotate(90) tear2d(hole_ear); translate([-hole_ear, -hole_ear]) square([hole_ear + hole_ear / 2 - peg_flat, 2 * hole_ear]); } }
module sh_bores() for (m = [0, 1]) mirror([m, 0, 0]) translate([t_w / 2 - sh_bore_depth, sh_y_l, sh_z_l]) rotate([90, 0, 90]) linear_extrude(sh_bore_depth + 1) sh_bore2d();
module neck_cuts() {
  translate([0, 0, neck_rec_floor]) linear_extrude(5) offset(delta = fit / 2) neck_lo2d();   // Mulde fuer den unteren Flansch
  translate([0, 0, neck_blk_z - 1]) cylinder(d = neck_bore, h = neck_rec_floor - neck_blk_z + 1.01);
  drv_channel();
}
module t_cuts_common(pilots) {
  neck_cuts();
  translate([0, 0, t_plate_z - 1]) cylinder(d = t_cable_d, h = wall + 2);
  if (pilots) { neck_pilots_t(); seat_pilots_t(); }
}
module t_lip_ring() { intersection() { t_ext(-(lip_h - 0.2), 0.01) difference() { t_cav2d(a_lip + fit); t_cav2d(wall); } t_solid(); } }

module torso_front(pilots = true) {
  difference() {
    union() {
      difference() { t_half(true); t_cavity(); }
      intersection() { t_half(true); translate([-40, -25, t_plate_z]) cube([80, 50, wall]); }
      intersection() { t_half(true); t_adds(); }
    }
    t_ext(-lip_h, 0.01) difference() { t_cav2d(a_lip); t_cav2d(wall); }     // Falz: Innenhaelfte der Wand
    cam_cut();
    t_cuts_common(pilots);
    if (pilots) dome_pilots_t();
    for (p = t_deco_pts) translate([p[0], -t_d / 2 - 0.01, p[1]]) rotate([-90, 0, 0]) cylinder(d = deco_d + fit_peg, h = deco_in + 0.01);
  }
}
module vent_slots() for (x = [-8, -4, 0, 4, 8]) translate([x - 0.8, t_d / 2 - wall - 1, 17]) cube([1.6, wall + 2, 14]);   // Lueftungsschlitze (schneiden auch die Stuetzwand der Wiege an)
module torso_back(pilots = true) {
  difference() {
    union() {
      difference() { t_half(false); t_cavity(); }
      intersection() { t_half(false); translate([-40, -25, t_plate_z]) cube([80, 50, wall]); }
      intersection() { t_half(false); t_adds(); }
      t_lip_ring();
      cam_stop_back();   // Anschlag hinter der XIAO-Unterseite (reicht ueber die Teilungsebene nach vorn)
    }
    t_cuts_common(pilots);
    if (!cam_off) cam_frame() xiao_cradle_cut(cam_wt);   // Stapelraum auch in der Rueckhaelfte freihalten
    sh_bores();
    for (p = t_dome_pts) {
      translate([p[0], t_d / 2 + 1, p[1]]) rotate([90, 0, 0]) cylinder(d = screw_d, h = t_d / 2 + 2);
      translate([p[0], t_d / 2 + 0.01, p[1]]) rotate([90, 0, 0]) cylinder(d = cbore_d, h = cbore_t + 0.01);
    }
    vent_slots();
  }
}
module torso_decos() for (p = t_deco_pts) translate([p[0], -t_d / 2 + deco_in, p[1]]) rotate([90, 0, 0]) head_deco();

// ---- Hals (Lokalkoordinaten des Oberkoerpers; oberer Flansch um head_tilt gekippt, passt zum Kopfsockel)
module neck_up() { translate(neck_pt - torso_origin) rotate([-head_tilt, 0, 0]) children(); }
module neck() {
  difference() {
    union() {
      translate([0, 0, neck_lo_z]) linear_extrude(neck_fl_t) neck_lo2d();
      hull() { translate([0, 0, neck_lo_z + neck_fl_t - 0.01]) cylinder(d = neck_d, h = 0.01); neck_up() translate([0, 0, -0.01]) cylinder(d = neck_d, h = 0.01); }
      // oberer Flansch mit Abflachung zur Gesichtsseite (lokal -Y): die Bohrung im Kopfsockel hat dort ein auf 12 mm gekapptes Dach (bore_roof)
      neck_up() intersection() { cylinder(d = neck_flange_d, h = neck_fl_t); translate([-neck_flange_d, -(bore_roof - 0.3), -1]) cube([2 * neck_flange_d, 2 * neck_flange_d, neck_fl_t + 2]); }
    }
    // Bohrung Ø12 im Rohr bis zur Unterseite des oberen Flansches, dort Langloch 8 x 12 (Wand zum Schraubenloch 2,3)
    hull() { translate([0, 0, neck_lo_z - 1]) cylinder(d = neck_bore, h = 0.01); neck_up() translate([0, 0, 0.01]) cylinder(d = neck_bore, h = 0.01); }
    neck_up() translate([0, 0, -0.5]) linear_extrude(neck_fl_t + 1.5) hull() for (sy = [-1, 1]) translate([0, sy * (neck_slot_l - neck_slot_w) / 2]) circle(d = neck_slot_w);
    drv_channel();                                   // Seitenkanaele: verschmelzen mit der Bohrung, kein Steg unter 2 mm
    for (sx = [-1, 1]) neck_up() translate([sx * neck_bolt_x, 0, -1]) cylinder(d = screw_d, h = neck_fl_t + 2);
    for (sx = [-1, 1]) translate([sx * neck_lo_bx, neck_lo_by, neck_lo_z - 1]) cylinder(d = screw_d, h = neck_fl_t + 2);
  }
}

// ---- Arm und Hand (Weltlage)
module arm_dir() rotate([135, 0, 0]) children();     // lokale z-Achse -> arm_d
module peg2d() { rotate(-90) tear2d(arm_peg_d); rotate(90) tear2d(arm_peg_d); }   // Raute: Spitzen +-X, druckt liegend ohne Stuetzen
module sh_peg() {
  intersection() {
    union() { cylinder(d = peg_d, h = arm_sh_peg_l - peg_ch + 0.01); translate([0, 0, arm_sh_peg_l - peg_ch]) cylinder(d1 = peg_d, d2 = peg_d - 2 * peg_ch, h = peg_ch); }
    translate([-peg_d, -peg_d, -1]) cube([2 * peg_d, peg_d + peg_d / 2 - peg_flat, arm_sh_peg_l + 2]);
  }
}
module arm_r() {
  e0 = arm_H + arm_s_end * arm_d;
  intersection() {
    union() {
      translate(arm_S) sphere(r = ball_r);
      hull() { translate(arm_S) sphere(r = arm_r1); translate(e0) arm_dir() cylinder(r = arm_r2, h = 0.01); }
    }
    translate([arm_x - ball_in, -45, 20]) cube([ball_in + arm_flat, 65, 60]);
  }
  translate(e0 - 0.01 * arm_d) arm_dir() linear_extrude(arm_peg_tip - arm_s_end + 0.01) peg2d();
  translate([arm_x - ball_in + 0.01, arm_S[1], arm_S[2]]) rotate([0, -90, 0]) sh_peg();
}
module arm(s = 1) { if (s > 0) arm_r(); else mirror([1, 0, 0]) arm_r(); }
module hand_l() {   // Mitte im Ursprung, Flaeche unten bei z = -hand_cut
  difference() {
    intersection() { scale(hand_sc) sphere(r = hand_r, $fn = 48); translate([-20, -20, -hand_cut]) cube([40, 40, 40]); }
    translate(-10 * arm_d) arm_dir() linear_extrude(10 + hand_hole_end) offset(delta = fit_peg) peg2d();
  }
}
module hand(s = 1) { translate([s * arm_x, hand_c_y, arm_H[2]]) hand_l(); }
module arm_print() translate([-(arm_S[2] - arm_S[1]) * cos(45), 0, arm_x + arm_flat]) rotate([0, 90, 0]) rotate([-45, 0, 0]) arm_r();

module body_shells() { place_torso() { torso_front(); torso_back(); neck(); torso_decos(); } for (s = [1, -1]) { arm(s); hand(s); } }
module shells()   { place_head() head_shells(); body_shells(); seat_shells(); }
module dummies()  { place_head() head_dummies(); xiao_dummy_r(); camera_dummy_r(); lens_cone_r(); seat_dummies(); }

// ---- Pruefkoerper fuer die Montage (Schraubendreher Ø6, 60 lang) und Kollisionspaare
module hits() { for (i = [1 : $children - 1]) intersection() { children(0); children(i); } }   // child 0 gegen jedes weitere Kind einzeln
module arms_hands() { for (s = [1, -1]) { arm(s); hand(s); } }
module drv_p1(dx = 0) for (sx = [-1, 1]) place_torso() translate([sx * neck_lo_bx + dx, neck_lo_by, neck_lo_z + neck_fl_t + screw_head_h]) cylinder(d = drv_d, h = drv_l);
module drv_p2(dx = 0) for (sx = [-1, 1]) place_torso() neck_up() translate([sx * neck_bolt_x + dx, 0, -screw_head_h]) rotate([180, 0, 0]) cylinder(d = drv_d, h = drv_l);
// Phase 1: Oberkoerper komplett, Hals aufgesetzt, kein Kopf. Phase 2: Rueckwand ab, kein XIAO, Kopf drauf.
module phase1_parts() { place_torso() { torso_front(); torso_back(); neck(); } arms_hands(); }
module phase2_parts() { place_torso() { torso_front(); neck(); } place_head() head_shells(); arms_hands(); }

// ================================================================= Po, Beine, Akkufach, Boden, Fuesse (Task 4)
// Weltlage: X rechts, Y nach hinten, Z oben. Standflaeche = Unterseite von bottom bei z = 0.
// MONTAGEFOLGE (komplett, jede Schraube per Pruefkoerper Ø6 x 60 nachgewiesen, check_roboter.ps1 Schritt 3g/3h):
//   1. Hals von oben mit 2 M3x10 auf den Oberkoerper (Phase 1, torso_front + torso_back sind verschraubt, kein Kopf).
//   2. Rueckwand des Oberkoerpers ab, Kopf auf den Hals, 2 M3x10 von UNTEN durch die Halsschlitze (Phase 2, Oberkoerper noch nicht auf dem Sitz).
//   3. XIAO von hinten einsetzen, Rueckwand schliessen (4 M3x28).
//   4. Einheit Oberkoerper+Hals+Kopf auf seat stecken (Steckrand greift um den Sitz) und von UNTEN mit 4 M3x10 (Flachkopf, seat_screw_l) verschrauben.
//      Der Schraubendreher kommt durch den offenen Boden (bottom ab, Akku noch nicht eingelegt).
//   5. Akku einlegen, 5V/GND verdrahten (Schalter, Kabel nach oben), bottom mit 4 M3x10 (Flachkopf) schliessen.
// ---- Akkumodul DFR0969 (DFRobot "2-Way 18650 Battery Holder"), Quelle: https://www.dfrobot.com/product-2578.html, Size Diagram (3_0x0.png)
bat_l      = 103.5;  // QUELLE (Datenblatt): Size Diagram Platine 103,50 mm
bat_w      = 48.0;   // QUELLE (Datenblatt): Size Diagram Platine 48,00 mm
bat_hole_dx = 95.5;  // QUELLE (Datenblatt): Montagebohrungen 95,50 mm in X (nur Information, nicht verwendet)
bat_hole_dy = 33.0;  // QUELLE (Datenblatt): Montagebohrungen 33,00 mm in Y (nur Information, nicht verwendet)
bat_r      = 6;      // VORLÄUFIG (Schätzung): Eckradius der Platine aus der Zeichnung
bat_pcb_t  = 1.6;    // VORLÄUFIG (Schätzung): Platinendicke, Standard
bat_h      = 25.0;   // VORLÄUFIG (Spec 2026-10-26: ca. 25 mm mit Zellen): Platinenunterkante bis Oberkante Zellen, im Datenblatt steht keine Hoehe
cell_d     = 18.6;   // QUELLE: Spec: 18650 Ø 18,6
cell_l     = 69;     // QUELLE: Spec: 65 mm, mit Polkappen bis 69 mm
cell_pitch = 19.5;   // QUELLE (Datenblatt): Zeichnung, Abstand der beiden Zellreihen ca. 19,5 (aus dem Bild abgelesen)
holder_end_w = 6;    // VORLÄUFIG (Schätzung): Kontaktbloecke an beiden Zellenenden, 6 mm tief in X
holder_end_h = 14;   // VORLÄUFIG (Schätzung): Hoehe der Kontaktbloecke ueber der Platine
// Lage der Bauteile in Modulkoordinaten (Mitte der Platine, Platinenunterkante z = 0). Die Zeichnung zeigt die Platine in Draufsicht mit 5V-Reihe oben und
// Anschluessen rechts; im Gehaeuse liegt das Modul um 180 Grad gedreht (5V-Reihe vorn, Anschluesse links). Lage der Buchsen aus der Zeichnung nur grob lesbar.
bat_usbc_y = 3;      // VORLÄUFIG (Schätzung): USB-C Mitte Y in Modulkoordinaten (Zeichnung: Anschluesse am Rand, Mitte der Kante)
bat_usbc_w = 8.94;   // QUELLE: USB-C Norm-Buchse, Breite (VORLÄUFIG, Teil nicht vermessen)
bat_usbc_d = 8.65;   // VORLÄUFIG (Schätzung): Buchse reicht 8,65 mm in die Platine (buendig mit der Kante)
bat_usbc_h = 1.6;    // VORLÄUFIG (Schätzung): Buchse sitzt auf der Unterseite und ragt 1,6 unter die Platine
bat_dip    = [-38.8, -19.5];  // VORLÄUFIG (Schätzung aus Zeichnung): HOLD/NORMAL-Schalter, Mitte (X, Y), nach 180-Grad-Drehung
bat_dip_w  = 11.0;   // VORLÄUFIG (Schätzung): DIP-Schalter 11 x 4
bat_dip_d  = 4.0;    // VORLÄUFIG (Schätzung)
bat_dip_h  = 1.8;    // VORLÄUFIG (Schätzung): ragt 1,8 unter die Platine
bat_led    = [-25, -20];     // VORLÄUFIG (Schätzung): Lade-LED, Mitte (X, Y). Lage im Datenblatt NICHT belegt, frei gewaehlt am vorderen Rand zwischen DIP-Schalter und Schalterbucht; das Loch im Boden ist deshalb gross (Ø5,6)
bat_pad    = [-5.8, -22.3];  // VORLÄUFIG (Schätzung aus Zeichnung): 5V/GND-Lötpads, drittes Paar der 5V-Reihe am vorderen Rand
batt_clear = 1.0;    // Vorgabe Auftraggeber: >= 1 mm Zusatzspiel seitlich rund um das Akkumodul (Maße nicht belegt)
bat_yc     = -6;     // VORLÄUFIG (Entwurf): Modulmitte Y (Welt); der Akku liegt unter dem Oberkoerper (torso Y -30...14)
// ---- Sitz (seat) und Boden (bottom)
bot_t      = 2.4;    // QUELLE: Aufgabe: Boden >= 2,4
bat_z0     = bot_t;  // Modul liegt direkt auf dem Boden (Unterseitenbauteile stehen in den Bodenoeffnungen)
s_top      = leg_h;  // QUELLE: Konzept (leg_h, hier 33,6, siehe oben): Oberflaeche der Oberschenkel, die Haende liegen darauf
s_wall     = 5.4;    // VORLÄUFIG (Entwurf): Wand des Sitzes. Die Dome Ø7,4 und die Senkungen Ø6,2 der Bodenschrauben sitzen genau in den Ecken des Fachs (Modul-Ecke R6 laesst dort Platz); unter der Senkung bleibt am Rand der Bodenplatte 2,15 mm
s_lip      = 1.8;    // VORLÄUFIG (Entwurf): Falz zwischen Sitz und Boden (aeusserer Rand des Sitzes reicht bis z = 0)
ceil_air   = 2.0;    // Vorgabe Auftraggeber: >= 2 mm Luft ueber Zellen und Modul (Hoehe nicht belegt)
zc         = bat_z0 + bat_h + ceil_air;   // abgeleitet: Unterseite der Deckenplatte des Sitzes (Oberkante des Fachs)
s_ch       = 4;      // QUELLE: wie t_ch/Kopf: 45-Grad-Fase an der Oberkante (der Sitz wird auf dem Kopf druckend gedruckt)
s_chb      = 0;      // VORLÄUFIG (Entwurf): keine Fase an der Unterkante (Standflaeche bis zum Rand, Senkung der Eckschrauben braucht den Rand)
chx        = bat_l / 2 + batt_clear;   // abgeleitet: Halbbreite des Fachs
chy        = bat_w / 2 + batt_clear;   // abgeleitet: Halbtiefe des Fachs
cav_y0     = bat_yc - chy;
cav_y1     = bat_yc + chy;
dome_s_d   = 7.4;    // VORLÄUFIG (Entwurf): Dom aussen, Wand um das Kernloch (Ø2,6) 2,4, um die Bohrung Ø3,4 genau 2,0 (wie t_dome_d)
boss_h     = 2.0;    // VORLÄUFIG (Entwurf): Auflage am Boden unter den Domen (Restdicke unter der Senkung >= 2)
dome_z0    = bat_z0 + boss_h;   // abgeleitet: Unterkante der Dome
pilot_s_len = 7.6;   // VORLÄUFIG (Entwurf): Kernloch M3 in den Domen (>= 7,5)
bay_x0     = -26;    // VORLÄUFIG (Entwurf): Schalterbucht vorn links, X von ...
bay_x1     = -12;    // ... bis
bay_y0     = -45;    // VORLÄUFIG (Entwurf): Y von ...
bay_y1     = cav_y0 + 1;   // ... bis (1 mm Ueberlappung mit dem Akkufach)
rib_hx     = 34;     // VORLÄUFIG (Entwurf): Halbbreite der Stuetzrippen unter dem Steckrand (vorn/hinten ueber den Rand-Streifen des Moduls)
rib_z0     = torso_origin[2] - fit / 2 - 2.05;   // abgeleitet: Unterkante der Rippen, 2,05 mm unter dem Boden der Steckrand-Aussparung (Rand-Unterkante - fit/2)
rib_f_y1   = -25.4;  // VORLÄUFIG (Entwurf): Rippe vorn bis Y (die Zellen beginnen bei Y -25,05)
rib_b_y0   = 10;     // VORLÄUFIG (Entwurf): Rippe hinten ab Y
notch_x0   = -10;    // VORLÄUFIG (Entwurf): Kabelkerbe in der vorderen Rippe, X von ...
notch_x1   = 8;      // ... bis
cbore_hd   = 1.65;   // VORLÄUFIG (Datenblatt): M3 Flachkopf ISO 7380, Kopfhoehe 1,65
s_cbore_t  = 1.7;    // VORLÄUFIG (Entwurf): Senkung der Sitzschrauben im Fach (>= Kopfhoehe); Schraube M3x10 endet damit 0,1 mm vor dem Kernlochende im Oberkoerper
seat_screw_l = 10;   // Vorgabe Auftraggeber: Sitzschrauben M3x10
dome_off   = 2.4;    // VORLÄUFIG (Entwurf): Dom- und Schraubenmitte um dieses Mass je Achse nach aussen aus der Fach-Ecke geschoben: haelt >= 1 mm Abstand zur Platinenecke auch ohne Eckradius
ear_r      = 6.0;    // VORLÄUFIG (Entwurf): Aussenradius der Eck-Ohren um die Domachse: Wand um die Bodenauflagen-Tasche (Ø7,7) 2,15, um die Senkung Ø6,2 im Boden 2,75
op_sure    = 1.5;    // Vorgabe Auftraggeber: Zugabe >= 1,5 mm rundum an allen Bodenoeffnungen
op_unsure  = 3.0;    // VORLÄUFIG (Entwurf): grosse Zugabe dort, wo die Lage am Modul nicht belegt ist (USB-C, DIP, LED)
led_hole_d = 5.6;    // Vorgabe Auftraggeber: LED-Loch >= Ø5,6
thigh_join = 15;     // VORLÄUFIG (Entwurf): Tiefe des Blockstreifens (ab Vorderkante), den der Oberschenkel-Hull mitnimmt, damit die Beine glatt in den Po uebergehen
sw_rib_w   = 2.0;    // VORLÄUFIG (Entwurf): Breite der Schalter-Auflagerippen (an den Schlitzenden)
edge_h     = 12;     // VORLÄUFIG (Schätzung): Hoehe von Bauteilen im Randstreifen des Moduls (nur fuer den Rippen-Abstand)
// Schiebeschalter SS12D00 (VORLÄUFIG, nicht vermessen)
sw_l       = 12.0;   // VORLÄUFIG (Schätzung): Gehaeuse 12 x 6 x 6
sw_w       = 6.0;
sw_h       = 6.0;
sw_lev_h   = 4.0;    // VORLÄUFIG (Schätzung): Hebel 4 mm lang (zeigt nach unten durch den Boden)
sw_lev_w   = 2.0;    // VORLÄUFIG (Schätzung): Hebelbreite
sw_travel  = 3.5;    // VORLÄUFIG (Schätzung): Schaltweg in X
sw_x       = -19;    // VORLÄUFIG (Entwurf): Schaltermitte X (in der Bucht)
sw_y       = -38;    // VORLÄUFIG (Entwurf): Schaltermitte Y
sw_slot_w  = sw_lev_w + 2 * op_sure;    // abgeleitet: Hebelbreite + 1,5 mm Zugabe je Seite
sw_slot_l  = sw_travel + 3 + 2 * op_sure;   // abgeleitet: Schaltweg + Hebel 3 + 1,5 mm Zugabe je Seite
sw_z0      = bot_t + 2.2;     // abgeleitet: Gehaeuseunterkante; Hebelspitze liegt bei z = 0,6 (>= 0, steht nicht unter der Standflaeche)
// Fuss (foot) und Zapfen
leg_x      = 28;     // QUELLE: Konzept (leg_x): Beinmitte
leg_len    = 62;     // QUELLE: Konzept (leg_len): vorderes Ende der Beine bei Y = -62
foot_w     = 44;     // QUELLE: Konzept (foot_w)
foot_t     = 18;     // QUELLE: Konzept (foot_l): Fussdicke in Y
foot_h     = 44;     // QUELLE: Konzept (foot_h); mit dem angehobenen Oberbau (leg_h 33,6) streift der Sichtkegel der Kamera die Fuesse nicht mehr (Pruefung: Kollision)
foot_r     = 7;      // QUELLE: Konzept: Eckradius 7
foot_chb   = 3;      // VORLÄUFIG (Entwurf): 45-Grad-Fase an der Rueckseite (Druckflaeche des Fusses, ohne Ueberhang)
foot_chf   = 5;      // VORLÄUFIG (Entwurf): Fase vorn
fpeg_d     = 8;      // QUELLE: Aufgabe: Zapfen Ø 8
fpeg_l     = 8;      // VORLÄUFIG (Entwurf): Zapfenlaenge (>= 6)
fpeg_z     = 14;     // VORLÄUFIG (Entwurf): Zapfenhoehe ueber dem Boden
fpeg_ch    = 0.5;    // QUELLE: Aufgabe: 0,5er Fase
leg_stub_r = 12;     // QUELLE: Konzept: Eckradius der vorderen Beine
leg_stub_w = 46;     // QUELLE: Konzept: Breite der vorderen Beine
leg_stub_d = 28;     // QUELLE: Konzept: Tiefe der vorderen Beine
seat_screw_pts = [for (p = t_seat_pts) [p[0], torso_origin[1] + p[1]]];   // abgeleitet: Welt-XY der 4 Sitzschrauben
dome_pts_s = [for (sx = [-1, 1], sy = [-1, 1]) [sx * (chx + dome_off), (sy < 0 ? cav_y0 : cav_y1) + sy * dome_off]];   // abgeleitet: Dome/Bodenschrauben ausserhalb der Fach-Ecken
// ---- Wandbilanz Sitz/Boden (Ziel >= 2 mm)
assert(s_wall >= 2, "Sitz: Wand unter 2 mm");
assert(s_top - zc - s_cbore_t >= 2, "Sitz: Rest ueber der Senkung der Sitzschrauben unter 2 mm");
assert(s_cbore_t >= cbore_hd, "Senkung zu flach fuer den M3-Flachkopf");
// ---- Luft ueber Zellen und Modul (Auftraggeber: >= 2 mm) und Abstand der Platinenecke (>= 1 mm, Eckradius 0 als ungünstigster Fall)
function cell_top(y) = bat_z0 + bat_h - cell_d / 2 + max([for (sy = [-1, 1]) let(d = abs(y - (bat_yc + sy * cell_pitch / 2))) d < cell_d / 2 ? sqrt(sq(cell_d / 2) - sq(d)) : -cell_d]);
function sq(v) = v * v;
assert(zc - (bat_z0 + bat_h) >= 2, "Luft zwischen Zellenoberkante und Deckenplatte unter 2 mm");
assert(torso_origin[2] - (bat_z0 + bat_h) >= 2, "Luft zwischen Zellenoberkante und Unterkante des Oberkoerper-Randes unter 2 mm");
assert(rib_z0 - max(cell_top(rib_f_y1), cell_top(rib_b_y0), bat_z0 + bat_pcb_t + edge_h) >= 2, "Rippe: weniger als 2 mm ueber Zellen/Randbauteilen des Moduls");
assert(torso_origin[2] - fit / 2 - rib_z0 >= 2, "Rippe: weniger als 2 mm unter der Steckrand-Aussparung");
assert(norm([chx + dome_off - bat_l / 2, dome_off + batt_clear + 0]) - dome_s_d / 2 >= 1, "Dom: Abstand zur Platinenecke (Radius 0) unter 1 mm");
assert(batt_clear >= 1, "Fach: seitliche Zugabe unter 1 mm");
assert(torso_origin[2] + t_plate_z + t_seat_pilot - (zc + s_cbore_t + seat_screw_l) >= 0.4 && zc + s_cbore_t + seat_screw_l - (torso_origin[2] + t_plate_z) >= 6, "Sitzschraube: greift weniger als 6 mm in den Oberkoerper oder ragt ueber das Kernloch");
assert(ear_r - fit / 2 - cbore_d / 2 >= 2, "Boden: Rand um die Senkung der Eckschrauben unter 2 mm");
assert(ear_r - (dome_s_d + fit) / 2 >= 2, "Sitz-Eck-Ohr: Wand ausserhalb der Bodenauflagen-Tasche unter 2 mm");
assert((dome_s_d - pilot_d) / 2 >= 2, "Sitz-Dom: Wand um das Kernloch unter 2 mm");
assert((dome_s_d - screw_d) / 2 >= 2, "Sitz-Dom/Bodenauflage: Wand um die Bohrung unter 2 mm");
assert(bot_t - cbore_t + boss_h >= 2, "Boden: Rest unter der Senkung unter 2 mm");
assert(bot_t >= 2.4, "Boden duenner als 2,4");
assert(s_wall - fit / 2 - cbore_d / 2 >= 2, "Boden: Rand um die Senkung der Eckschrauben unter 2 mm");
assert(pilot_s_len >= 7.5, "Sitz-Dom-Kernloch unter 7,5");
assert(zc - dome_z0 >= pilot_s_len + 1.5, "Sitz-Dom: Kernloch zu lang");
assert(fpeg_d >= 6 && fpeg_l >= 6, "Fusszapfen kleiner als 6 mm");
assert(foot_t - (fpeg_l + 0.6) >= 2, "Fuss: Wand hinter dem Zapfenloch unter 2 mm");
assert(fpeg_z - (fpeg_d / 2 * sqrt(2) + fit_peg) >= 2, "Fuss: Wand unter dem Zapfenloch unter 2 mm");
assert(sw_z0 - sw_lev_h >= 0, "Schalterhebel steht unter der Standflaeche");
assert(abs(bay_x1 - bay_x0) >= sw_l + 2 && abs(bay_y1 - bay_y0) >= sw_w + 6, "Schalterbucht zu klein");

// ---- 2D-Formen
module cav_main2d() translate([-chx, cav_y0]) square([2 * chx, 2 * chy]);
module bay2d()      translate([bay_x0, bay_y0]) square([bay_x1 - bay_x0, bay_y1 - bay_y0]);
module cav2d()      { cav_main2d(); bay2d(); }
// Zone des Bodens (z < bot_t): Falz-Streifen s_lip am Rand, in den vier Ecken (Dome, Senkungen) reicht der Boden bis an den Rand. d = -fit/2 fuer den Boden, 0 fuer die Aussparung im Sitz
module plate_zone(d, e = 0) { offset(r = s_wall - s_lip + d) cav2d(); for (p = dome_pts_s) translate(p) circle(r = ear_r + d + e); }   // e: Uebermass, damit die Aussparung die Aussenflaeche des Sitzes sauber durchschneidet
module main_out2d() offset(r = s_wall) cav_main2d();
module bay_out2d()  offset(r = s_wall) bay2d();
module thigh_out2d(sx) hull() {
  for (dx = [-1, 1], cy = [-leg_len + leg_stub_r, -leg_len + leg_stub_d - leg_stub_r])
    translate([sx * leg_x + dx * (leg_stub_w / 2 - leg_stub_r), cy]) circle(r = leg_stub_r);
  intersection() { main_out2d(); translate([sx > 0 ? 0 : -100, cav_y0 - s_wall - 0.01]) square([100, thigh_join]); }
}
module chamf3(h, ct, cb) {   // konvexe 2D-Form (children) extrudieren, oben ct, unten cb als 45-Grad-Fase
  hull() {
    translate([0, 0, cb]) linear_extrude(h - ct - cb) children();
    linear_extrude(0.01) offset(delta = -cb) children();
    translate([0, 0, h - 0.01]) linear_extrude(0.01) offset(delta = -ct) children();
  }
}
module seat_outer() {
  chamf3(s_top, s_ch, s_chb) main_out2d();
  chamf3(s_top, s_ch, s_chb) bay_out2d();
  for (sx = [-1, 1]) chamf3(s_top, s_ch, s_chb) thigh_out2d(sx);
  for (p = dome_pts_s) translate(p) chamf3(s_top, s_ch, s_chb) circle(r = ear_r);   // Eck-Ohren um die Dome
}
// Raender der Rippen und des Faches
module ribs() {
  translate([-rib_hx, cav_y0 - 0.5, rib_z0]) cube([2 * rib_hx, rib_f_y1 - cav_y0 + 0.5, zc - rib_z0 + 0.01]);
  translate([-rib_hx, rib_b_y0, rib_z0]) cube([2 * rib_hx, cav_y1 + 0.5 - rib_b_y0, zc - rib_z0 + 0.01]);
}
module dome_cols() for (p = dome_pts_s) translate([p[0], p[1], dome_z0]) cylinder(d = dome_s_d, h = zc - dome_z0 + 0.01);
module cavity_cut() {
  difference() {
    union() {
      translate([0, 0, bat_z0]) linear_extrude(zc - bat_z0) cav2d();
      for (p = dome_pts_s) translate([p[0], p[1], bat_z0 - 0.01]) cylinder(d = dome_s_d + fit, h = boss_h + 0.01);   // Tasche fuer die Bodenauflage (ragt in die Wand)
    }
    ribs();
    dome_cols();
  }
}
// Steckrand-Aussparung: Huellkoerper des Oberkoerper-Randes (z 0..t_plate_z, plus fit/2) ohne den Zapfen (Innenkontur minus fit/2)
module rim_env() place_torso() minkowski() {
  intersection() { t_solid(); translate([-40, -30, 0]) cube([80, 60, t_plate_z]); }
  cube(fit, center = true);
}
module plug_env() place_torso() translate([0, 0, -1]) linear_extrude(t_plate_z + 1.01) offset(delta = -fit / 2) torso_pocket2d();
module recess_cut() difference() { rim_env(); plug_env(); }
module dome_pilots_s() for (p = dome_pts_s) translate([p[0], p[1], dome_z0 - 0.01]) cylinder(d = pilot_d, h = pilot_s_len + 0.01);

module fpeg_prof() {   // Zapfenprofil in (X, Z): Tropfen mit Spitze nach unten (-Z), oben abgeflacht
  intersection() {
    rotate(180) tear2d(fpeg_d);
    translate([-fpeg_d, -fpeg_d]) square([2 * fpeg_d, fpeg_d + fpeg_d / 2 - peg_flat]);
  }
}
module fpeg() {   // Zapfen am Ursprung, zeigt nach -Y (Wurzel bei Y = 0)
  rotate([90, 0, 0]) hull() {
    translate([0, 0, -0.1]) linear_extrude(fpeg_l - fpeg_ch + 0.1) fpeg_prof();
    translate([0, 0, fpeg_l - 0.01]) linear_extrude(0.01) offset(delta = -fpeg_ch) fpeg_prof();
  }
}
module seat(pilots = true) {
  difference() {
    union() {
      difference() {
        seat_outer();
        translate([0, 0, -1]) linear_extrude(bat_z0 + 1) plate_zone(0, 0.5);   // Platz fuer den Boden (z < bot_t)
        cavity_cut();
      }
      for (sx = [-1, 1]) translate([sx * leg_x, -leg_len + 0.1, fpeg_z]) fpeg();
    }
    recess_cut();
    // Kabelkerbe in der vorderen Rippe (die Kabel laufen in der 2 mm hohen Luft ueber den Zellen), Kabelloch fluchtend mit dem Loch im Oberkoerper
    translate([notch_x0, cav_y0 - 0.6, rib_z0 - 0.01]) cube([notch_x1 - notch_x0, rib_f_y1 - cav_y0 + 0.6, zc - rib_z0 + 0.02]);
    translate([0, torso_origin[1], zc - 1]) cylinder(d = t_cable_d, h = s_top - zc + 2);
    // 4 Sitzschrauben von unten (Senkung im Fach, Flachkopf)
    for (p = seat_screw_pts) {
      translate([p[0], p[1], zc - 1]) cylinder(d = screw_d, h = s_top - zc + 2);
      translate([p[0], p[1], zc - 0.01]) cylinder(d = cbore_d, h = s_cbore_t + 0.01);
    }
    if (pilots) dome_pilots_s();
  }
}
module bottom() {
  difference() {
    union() {
      linear_extrude(bot_t) plate_zone(-fit / 2);
      for (p = dome_pts_s) translate([p[0], p[1], bot_t - 0.01]) cylinder(d = dome_s_d, h = boss_h + 0.01);
      // Auflage fuer den Schalter: zwei Rippen an den Schlitzenden (der Schlitz ist 1,5 mm groesser als der Hebel, das Gehaeuse liegt auf den Rippen)
      for (m = [-1, 1]) translate([sw_x + m * (sw_slot_l / 2 + sw_rib_w / 2) - sw_rib_w / 2, sw_y - sw_w / 2, bot_t - 0.01]) cube([sw_rib_w, sw_w, sw_z0 - bot_t - 0.01]);
    }
    for (p = dome_pts_s) {   // 4 x M3 von unten mit Senkung
      translate([p[0], p[1], -1]) cylinder(d = screw_d, h = bot_t + boss_h + 2);
      translate([p[0], p[1], -0.01]) cylinder(d = cbore_d, h = cbore_t + 0.01);
    }
    // USB-C und HOLD/NORMAL (Lage unbelegt: op_unsure rundum, an der Kante op_sure), LED Ø5,6, Schalterschlitz (op_sure rundum)
    translate([-bat_l / 2 - op_sure, bat_yc + bat_usbc_y - bat_usbc_w / 2 - op_unsure, -1]) cube([op_sure + bat_usbc_d + op_unsure, bat_usbc_w + 2 * op_unsure, bot_t + 2]);
    translate([bat_led[0], bat_yc + bat_led[1], -1]) cylinder(d = led_hole_d, h = bot_t + 2);
    translate([bat_dip[0] - bat_dip_w / 2 - op_unsure, bat_yc + bat_dip[1] - bat_dip_d / 2 - op_unsure, -1]) cube([bat_dip_w + 2 * op_unsure, bat_dip_d + 2 * op_unsure, bot_t + 2]);
    translate([sw_x - sw_slot_l / 2, sw_y - sw_slot_w / 2, -1]) cube([sw_slot_l, sw_slot_w, bot_t + 2]);
  }
}
module foot_sec() translate([0, foot_h / 2]) rrect(foot_w / 2, foot_h / 2, foot_r);
module foot_w(sx = 1) {   // Weltlage: Rueckseite bei Y = -leg_len, Fuss waechst nach -Y
  translate([sx * leg_x, -leg_len, 0]) difference() {
    rotate([90, 0, 0]) hull() {
      linear_extrude(0.01) offset(delta = -foot_chb) foot_sec();
      translate([0, 0, foot_chb]) linear_extrude(foot_t - foot_chb - foot_chf) foot_sec();
      translate([0, 0, foot_t - 0.01]) linear_extrude(0.01) offset(delta = -foot_chf) foot_sec();
    }
    // Zapfenloch: Profil mit fit_peg Spiel, in der Druckausrichtung senkrecht (Rueckseite auf dem Bett)
    translate([0, 0.01, fpeg_z]) rotate([90, 0, 0]) linear_extrude(fpeg_l + 0.6 + 0.01) offset(delta = fit_peg) fpeg_prof();
  }
}
module foot() foot_w(1);

// ---- Pruefkoerper
module wire(pts, d = 1.3) for (i = [0 : len(pts) - 2]) hull() { translate(pts[i]) sphere(d = d, $fn = 8); translate(pts[i + 1]) sphere(d = d, $fn = 8); }
module battery_dummy() translate([0, bat_yc, bat_z0]) {
  linear_extrude(bat_pcb_t) rrect(bat_l / 2, bat_w / 2, bat_r);
  for (sy = [-1, 1]) translate([-cell_l / 2, sy * cell_pitch / 2, bat_h - cell_d / 2]) rotate([0, 90, 0]) cylinder(d = cell_d, h = cell_l);
  for (sx = [-1, 1]) translate([sx * (cell_l / 2 + holder_end_w / 2) - holder_end_w / 2, -cell_pitch / 2 - cell_d / 2 + 0.5, bat_pcb_t - 0.01]) cube([holder_end_w, cell_pitch + cell_d - 1, holder_end_h]);
  translate([-bat_l / 2, bat_usbc_y - bat_usbc_w / 2, -bat_usbc_h]) cube([bat_usbc_d, bat_usbc_w, bat_usbc_h + 0.01]);
  translate([bat_dip[0] - bat_dip_w / 2, bat_dip[1] - bat_dip_d / 2, -bat_dip_h]) cube([bat_dip_w, bat_dip_d, bat_dip_h + 0.01]);
  translate([bat_led[0] - 1, bat_led[1] - 0.8, -0.8]) cube([2, 1.6, 0.81]);
}
module switch_dummy() {
  translate([sw_x - sw_l / 2, sw_y - sw_w / 2, sw_z0]) cube([sw_l, sw_w, sw_h]);
  translate([sw_x - (sw_travel + 3) / 2, sw_y - sw_lev_w / 2, sw_z0 - sw_lev_h]) cube([sw_travel + 3, sw_lev_w, sw_lev_h + 0.01]);   // Hebel ueber den ganzen Schaltweg
}
// Kabel 5V/GND: Pads (vorn am Modul) -> Schalter in der Bucht -> hoch durch die Rippenkerbe -> Rinne -> Kabelloch Ø10 -> in den Oberkoerper
module wire_dummy() {
  yh = torso_origin[1];
  zp = bat_z0 + bat_pcb_t;   // Platinenoberkante (Pads); Kabelebenen darueber: +5 (zum Schalter), +10 (Rueckweg), +12 (GND)
  wire([[bat_pad[0], bat_yc + bat_pad[1], zp], [bat_pad[0], bat_yc + bat_pad[1], zp + 5], [sw_x + 5, bat_yc + bat_pad[1], zp + 5], [sw_x + 5, sw_y + 2, zp + 5]]);
  zw = zc - 0.9;   // Kabelhoehe in der Luft ueber den Zellen (Mitte; Ø1,3, also 0,25 unter der Deckenplatte)
  wire([[sw_x - 5, sw_y + 2, zp + 5], [sw_x - 5, sw_y + 2, zp + 10], [sw_x - 5, -26.5, zp + 10], [-3, -26.5, zp + 10], [-3, -25.8, zp + 10], [-3, -25.8, zw], [0.7, yh, zw], [0.7, yh, s_top + 1]]);
  wire([[bat_pad[0] + 1.4, bat_yc + bat_pad[1], zp], [bat_pad[0] + 1.4, bat_yc + bat_pad[1], zp + 12], [bat_pad[0] + 1.4, -25.8, zp + 12], [bat_pad[0] + 1.4, -25.8, zw], [-0.7, yh, zw], [-0.7, yh, s_top + 1]]);
}
// Auflagenachweis: Grundriss der Handflaeche (Schnitt der Hand mit einer Platte z 32..32,3) als Prisma z 31,9..32 muss ganz im Material von seat liegen
module hand_support(dx = 0) difference() {
  translate([0, 0, leg_h - 0.1]) linear_extrude(0.09) projection() intersection() { translate([dx, 0, 0]) hand(1); translate([-100, -100, leg_h]) cube([200, 200, 0.3]); }
  seat();
}
module seat_shells() { seat(); bottom(); for (sx = [1, -1]) foot_w(sx); }
module seat_dummies() { battery_dummy(); switch_dummy(); wire_dummy(); }

// Werkzeug: Schritt 4 (Sitzschrauben von unten, Boden ab, Akku fehlt) und Schritt 5 (Bodenschrauben von unten)
head_bot_z4 = zc + s_cbore_t - cbore_hd;      // abgeleitet: Unterkante des Flachkopfes in der Senkung des Sitzes
head_bot_z5 = cbore_t - cbore_hd;            // abgeleitet: Unterkante des Flachkopfes in der Senkung des Bodens
module seat_screws() for (p = seat_screw_pts) translate([p[0], p[1], zc + s_cbore_t]) cylinder(d = 3, h = seat_screw_l);   // Gewinde-Ø3 ab der Auflage des Kopfes nach oben
module drv_p4(dy = 0) for (p = seat_screw_pts) translate([p[0], p[1] + dy, head_bot_z4 - drv_l]) cylinder(d = drv_d, h = drv_l);
module drv_p5(dx = 0, dz = 0) for (p = dome_pts_s) translate([p[0] - sign(p[0]) * dx, p[1], head_bot_z5 + dz - drv_l]) cylinder(d = drv_d, h = drv_l);
module phase4_parts() { phase1_parts(); seat(); for (sx = [1, -1]) foot_w(sx); }
module phase5_parts() { phase1_parts(); seat(); bottom(); battery_dummy(); switch_dummy(); for (sx = [1, -1]) foot_w(sx); }

// ================================================================= Schwerpunkt / Standsicherheit (Schaetzung)
// Massen in Konstruktionslage des Kopfes [g, x, y, z], VORLÄUFIG (Schaetzung)
head_masses_c = [
  [45, 0, 0, z_split / 2],                    // Frontschale + Dome
  [6, 0, 0, 0.3],                             // Blende
  [60, 0, -2, (z_back + z_split) / 2],        // Rueckwand + Sockel
  [12, hx + 4, ear_y, ear_z],                 // Ohr rechts
  [12, -hx - 4, ear_y, ear_z],                // Ohr links
  [25, 0, 0, (z_pcb_bot + z_rear) / 2],       // Platine mit Bauteilen
  [10, 0, 0, z_glass_bot + disp_h / 2]        // Display
];
function to_world(m) = let(w = head_to_world([m[1], m[2], m[3]])) [m[0], w[0], w[1], w[2]];
// [g, x, y, z] in Weltkoordinaten. Task 3/4 haengen ihre Listen mit concat(...) an.
arm_masses = [for (s = [-1, 1]) each [[8, s * arm_x, (arm_S[1] + arm_H[1]) / 2, (arm_S[2] + arm_H[2]) / 2], [6, s * arm_x, hand_c_y, arm_H[2]]]];
seat_masses = [
  [90, 0, -12, 16],                 // seat (Wand 3,8, Deckenplatte, massive Beine, ca. 40 % Fuellung), VORLÄUFIG (Schaetzung)
  [22, 0, bat_yc, 1.2],             // bottom
  [100, 0, bat_yc, bat_z0 + 11],    // Akkumodul mit 2 Zellen, ganz unten (QUELLE: Spec ca. 100 g)
  [22, leg_x, -leg_len - foot_t / 2, foot_h / 2],     // Fuss rechts
  [22, -leg_x, -leg_len - foot_t / 2, foot_h / 2],    // Fuss links
  [20, 0, -8, 40]                   // Schrauben (20 x M3), Kabel, Schalter
];
masses = concat([for (m = head_masses_c) to_world(m)],
  [[38, 0, -18, 52 + lift], [45, 0, 2, 52 + lift], [8, 0, -8, 78 + lift], [8, 0, -18, 52 + lift]],   // Oberkoerper vorn/hinten, Hals, XIAO (mit dem Oberbau um lift angehoben)
  arm_masses, seat_masses);
function mass_sum(ms, i, k = 0) = k >= len(ms) ? 0 : (i == 0 ? ms[k][0] : ms[k][0] * ms[k][i]) + mass_sum(ms, i, k + 1);
function msum(i) = mass_sum(masses, i);
mtot  = msum(0);
com_x = msum(1) / mtot;
com_y = msum(2) / mtot;
com_z = msum(3) / mtot;

stand_min = 5;      // QUELLE: Spec: Lot des Schwerpunkts >= 5 mm innerhalb der Sitzflaeche
// Aufstandsflaeche: konvexe Huelle der Aufstandspunkte von bottom (Unterkante mit 1 mm Fase) und beiden Fuessen (Standflaeche ohne Fasen), Welt-XY, gegen den Uhrzeigersinn
sp_x  = chx + s_wall - s_chb;                 // abgeleitet: Halbbreite der Unterkante von bottom
sp_y0 = cav_y0 - s_wall + s_chb;              // abgeleitet: Vorderkante von bottom (ohne Schalterbucht)
sp_y1 = cav_y1 + s_wall - s_chb;              // abgeleitet: Hinterkante
sp_fx = leg_x + foot_w / 2 - foot_r;          // abgeleitet: Fuss-Standflaeche ohne Eckrundung (x)
sp_fy = -leg_len - foot_t + foot_chf;         // abgeleitet: Vorderkante der Fuss-Standflaeche (y)
seat_poly = [[-sp_fx, sp_fy], [sp_fx, sp_fy], [sp_x, sp_y0], [sp_x, sp_y1], [-sp_x, sp_y1], [-sp_x, sp_y0]];
has_seat  = len(seat_poly) > 2;
function edge_d(a, b, p) = ((b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0])) / norm(b - a);
function stand_margin(p) = min([for (i = [0 : len(seat_poly) - 1]) edge_d(seat_poly[i], seat_poly[(i + 1) % len(seat_poly)], p)]);
// Positivfall: eine zusaetzliche Masse weit hinten, der Schwerpunkt muss die Aufstandsflaeche verlassen (Margin < stand_min)
ms_test = concat(masses, [[400, 0, 150, 60]]);   // 400 g bei Y = +150 (weit hinter dem Po)
com_t   = [mass_sum(ms_test, 1) / mass_sum(ms_test, 0), mass_sum(ms_test, 2) / mass_sum(ms_test, 0)];

module info() {
  echo(str("HEAD_W=", head_w, " HEAD_H=", head_h, " DEPTH=", -z_back, " SPLIT=", z_split, " NECK_Z=", z_neck));
  echo(str("MASS=", mtot, " COM=[", com_x, ",", com_y, ",", com_z, "]"));
  echo(str("WINDOW=", win_w, "x", win_h));
  echo(str("NECK_AXIS_W=", neck_axis_w, " NECK_PT=", neck_pt, " TILT=", head_tilt));
  if (has_seat) echo(str("STAND_MIN_MARGIN=", stand_margin([com_x, com_y]), " STAND_MIN=", stand_min));
  else echo("STAND_SKIPPED=kein seat vorhanden");
  echo(str("SEAT_PLAN_X=", sp_x, " Y=", sp_y0, "..", sp_y1, " FOOT_Y=", sp_fy));
  cube(1);
}
module info_selftest() {
  echo(str("STAND_SELFTEST_MARGIN=", stand_margin(com_t), " STAND_MIN=", stand_min));
  cube(1);
}

// ================================================================= Ansichten
module colored_head() {
  color("#3a7bd5") { head_front(); head_back(); }
  color("#161616") bezel();
  color("#ffb000") ears();
  color("#c0c0c0") decos();
}
module colored_dummies() {
  color("#2e8b4a", 0.6) pcb_dummy();
  color("#0b1a2a", 0.9) glass_dummy();
  color("#888888", 0.5) rear_dummy();
  color("#c0c0c0") { sockets_dummy(); buttons_dummy(); sd_dummy(); }
}
module assembly_parts() {
  place_head() { colored_head(); if (show_dummies) colored_dummies(); }
  colored_body();
}
module colored_body() {
  place_torso() {
    color("#ff7a29") { torso_front(); torso_back(); }
    color("#263238") neck();
    color("#c0c0c0") torso_decos();
    if (show_dummies) { color("#8a4fc9", 0.8) xiao_dummy_l(); color("#d03030") camera_dummy_l(); }
  }
  for (s = [1, -1]) { color("#ff7a29") arm(s); color("#263238") hand(s); }
  color("#3a7bd5") { seat(); bottom(); }
  for (s = [1, -1]) color("#161616") foot_w(s);
  if (show_dummies) { color("#2e8b4a", 0.7) battery_dummy(); color("#c0c0c0") switch_dummy(); color("#d33", 0.9) wire_dummy(); }
}
module exploded_upper() {
  place_head() {
    translate([0, 0, 28]) color("#161616") bezel();
    translate([0, 0, 14]) color("#c0c0c0") decos_head_only();
    color("#3a7bd5") head_front();
    translate([0, 0, -34]) color("#3a7bd5") head_back();
    translate([30, 0, 0]) color("#ffb000") ear_right();
    translate([-30, 0, 0]) color("#ffb000") ear_left();
    if (show_dummies) translate([0, 0, -16]) colored_dummies();
  }
  place_torso() {
    translate([0, -30, 0]) color("#ff7a29") torso_front();
    translate([0, 30, 0]) color("#ff7a29") torso_back();
    translate([0, 0, 22]) color("#263238") neck();
  }
  for (s = [1, -1]) translate([s * 25, 0, 0]) { color("#ff7a29") arm(s); color("#263238") hand(s); }
}
module exploded_parts() {
  translate([0, 0, 22]) exploded_upper();
  color("#3a7bd5") seat();
  translate([0, 0, -24]) color("#3a7bd5") bottom();
  for (s = [1, -1]) translate([s * 0, -16, 0]) color("#161616") foot_w(s);
  if (show_dummies) { translate([0, 0, -12]) { color("#2e8b4a") battery_dummy(); color("#c0c0c0") switch_dummy(); } }
}
module decos_head_only() { for (p = deco_pts) translate([p[0], p[1], -deco_in]) head_deco(); }

// ================================================================= Ausgabe
// Druckausrichtung: Gesicht bzw. Aussenseite nach unten, kein Teil braucht Stuetzen.
if (part == "head_front") rotate([180, 0, 0]) head_front();                // Gesicht (Z 0) auf dem Bett
else if (part == "head_front_ams") rotate([180, 0, 0]) head_front();           // 3MF/AMS: wie head_front (Lage identisch zu bezel_inlay und screws_inlay)
else if (part == "bezel_inlay") rotate([180, 0, 0]) bezel();                   // 3MF/AMS: Blende am Einbauort im Kopf (gleiche Lage wie head_front)
else if (part == "screws_inlay") rotate([180, 0, 0]) for (p = deco_pts) translate([p[0], p[1], -deco_in]) head_deco();   // 3MF/AMS: 4 Kopf-Zierschrauben am Einbauort
else if (part == "bezel") translate([0, 0, bezel_t]) rotate([180, 0, 0]) bezel(); // Blendenoberseite auf dem Bett, Steg nach oben
else if (part == "head_back") translate([0, 0, -z_back]) head_back();        // Rueckseite (Z back) auf dem Bett
else if (part == "ear") ear();                                                 // Aussenseite auf dem Bett, Zapfen nach oben
else if (part == "deco") {                                                     // Einsaetze flach: 4 Kopf-, 2 Ohr-Zierschrauben
  for (i = [0 : 3]) translate([i * 8, 0, 0]) head_deco();
  for (i = [0 : 1]) translate([i * 12, 12, 0]) ear_deco();
  for (i = [0 : 3]) translate([i * 8, -12, 0]) head_deco();   // Task 3: 4 weitere Einlagen fuer die Brust
}
else if (part == "torso_front") translate([0, 0, t_d / 2]) rotate([90, 0, 0]) torso_front();     // Brust (y = -22) auf dem Bett
else if (part == "torso_back")  translate([0, 0, t_d / 2]) rotate([-90, 0, 0]) torso_back();     // Rueckseite (y = +22) auf dem Bett
else if (part == "cam_test")   // Testausschnitt (ca. 15 min Druck): Brustwand X +-18 / Y +-14 mm um die Linse (X 18, damit die Rippen am Rand nicht frei haengen) mit Tasche, Rahmen, Pfosten mit Lippen und Wurzeln; Aussenseite unten wie torso_front
  difference() {   // ohne die zwei Randstuecke bei |X| > 12,3 (gehoeren am ganzen Teil zur Schale ausserhalb des Ausschnitts, hingen im Test frei)
  translate([0, 0, t_d / 2]) rotate([90, 0, 0]) intersection() { torso_front(false); cam_frame() translate([-18, -14, cam_zrear(cam_wt) - 13]) cube([36, 28, 13 + cam_wt + 6]); }
    for (sx = [-1, 1]) translate([sx > 0 ? 12.3 : -18.1, -11.85, -1]) cube([5.8, 10, 30]);
  }
else if (part == "neck")        translate([0, 0, -neck_lo_z]) neck();                              // stehend auf dem unteren Flansch
else if (part == "arm") { if (side > 0) arm_print(); else mirror([0, 1, 0]) arm_print(); }       // liegend auf der Aussenseite
else if (part == "hand")        translate([0, 0, hand_cut]) hand_l();                              // abgeflachte Unterseite auf dem Bett
else if (part == "seat")        translate([0, 0, s_top]) rotate([180, 0, 0]) seat();               // auf dem Kopf: Oberseite (z = 32) auf dem Bett, Fach nach oben offen, keine Stuetzen
else if (part == "bottom")      bottom();                                                           // Unterseite (Standflaeche) auf dem Bett
else if (part == "foot")        translate([0, 0, -leg_len]) rotate([-90, 0, 0]) translate([-leg_x, 0, 0]) foot_w(1);   // Rueckseite (Zapfenloch) auf dem Bett
else if (part == "screw_seat")  intersection() { seat_screws(); place_torso() { torso_front(); torso_back(); } }   // Sitzschrauben M3x10 gegen den Oberkoerper (mit Kernloechern): Z-Ausdehnung = Einschraubtiefe
else if (part == "pilot_seat_dome")intersection() { seat(false); dome_pilots_s(); }
else if (part == "hand_support") hand_support();
else if (part == "hand_support_selftest") hand_support(14);   // Hand 14 mm nach aussen versetzt: ragt ueber die Sitzkante, MUSS Material-Luecke melden
else if (part == "drv_p4") hits() { drv_p4(); phase4_parts(); }
else if (part == "drv_p5") hits() { drv_p5(); phase5_parts(); }
else if (part == "drv_selftest_p4") hits() { drv_p4(-6); phase4_parts(); }
else if (part == "drv_selftest_p5") hits() { drv_p5(5, 1.2); phase5_parts(); }
else if (part == "info_selftest") info_selftest();
else if (part == "collision_seat") union() {
  // Beruehrflaechen (Oberkante z = 32, Fuss-Rueckseite) liegen koplanar; fuer den Test den Sitz 0,02 nach unten bzw. die Fuesse 0,02 nach vorn versetzt, echte Durchdringungen bleiben sichtbar
  hits() { translate([0, 0, -0.02]) seat(); place_torso() { torso_front(); torso_back(); neck(); } place_head() head_shells(); arms_hands(); for (sx = [1, -1]) translate([0, -0.02, 0]) foot_w(sx); }
  hits() { translate([0, 0, 0.02]) seat(); bottom(); }
}
else if (part == "collision_seat_selftest") hits() { seat(); translate([0, 0, -3]) place_torso() torso_front(); }   // Oberkoerper 3 mm in den Sitz: MUSS Treffer liefern
else if (part == "pilot_neck")  intersection() { union() { torso_front(false); torso_back(false); } neck_pilots_t(); }
else if (part == "pilot_seat")  intersection() { union() { torso_front(false); torso_back(false); } seat_pilots_t(); }
else if (part == "pilot_dome")  intersection() { torso_front(false); dome_pilots_t(); }
else if (part == "collision_body") union() {
  hits() { place_torso() { torso_front(); torso_back(); } arm(1); arm(-1); }
  hits() { union() { arm(1); arm(-1); } hand(1); hand(-1); }
  hits() { place_torso() neck(); place_torso() { torso_front(); torso_back(); } place_head() head_shells(); }
  hits() { place_torso() { torso_front(); torso_back(); } place_head() head_shells(); }
  hits() { place_head() head_shells(); arm(1); arm(-1); hand(1); hand(-1); }     // Arme und Haende gegen Kopf und Ohren
}
else if (part == "collision_body_selftest")   // Arm 3 mm in den Oberkoerper geschoben: MUSS Treffer liefern
  hits() { place_torso() { torso_front(); torso_back(); } translate([-3, 0, 0]) arm(1); }
else if (part == "drv_p1") hits() { drv_p1(); phase1_parts(); }
else if (part == "drv_p2") hits() { drv_p2(); phase2_parts(); }
else if (part == "drv_selftest_p1") hits() { drv_p1(-12); phase1_parts(); }   // Werkzeug 12 mm zur Seite: muss Hals/Block treffen
else if (part == "drv_selftest_p2") hits() { drv_p2(6); phase2_parts(); }
else if (part == "pilot_neck_back") intersection() { torso_back(false); neck_pilots_t(); }   // Kernloecher der unteren Halsschrauben duerfen NICHT in torso_back liegen
else if (part == "warn_ctrl") { echo(k = undefined_k2_probe); intersection() { cube(1); translate([5, 5, 5]) cube(1); } }   // Kontrolle fuer Chk-Empty: leer, aber mit WARNING (unbekannte Variable)
else if (part == "empty_ctrl") intersection() { cube(1); translate([5, 5, 5]) cube(1); }   // Kontrolle fuer Chk-Empty: sauber leer, keine WARNING
else if (part == "ovh_ctrl_steep") translate([0, 0, 5]) rotate([35, 0, 0]) cube([10, 10, 3]);   // Kontrolle fuer die Ueberhang-Pruefung: Flanke mit 55 Grad Ueberhang (nz -0,82), MUSS als "steil" erkannt werden
else if (part == "ovh_ctrl_steep_low") translate([0, 0, 0.3]) rotate([35, 0, 0]) cube([10, 3, 3]);   // Kontrolle zur Ausnahme: steile Flanke ganz unten (innerhalb der Hoehengrenze der Ausnahme), aber weit von der Linsenachse: MUSS trotz Ausnahme erkannt werden
else if (part == "ovh_ctrl_flat") translate([0, 0, 5]) cube([20, 20, 2]);                       // Kontrolle: 20 mm freitragende Decke, MUSS als Bruecke > 15 mm erkannt werden
// ---- Montagewege (Kamera v2): der ganze Block (Kamera + XIAO) wird bei abgenommener Rueckwand von hinten (lokal -Z) in die Tasche und zwischen die Pfosten geschoben, dann schliesst torso_back. Jeder Weg ist eine Reihe von Zwischenstellungen.
else if (part == "asm_block")   // Block wird von hinten eingeschoben (Zwischenstellungen alle 1,5 mm bis 30 mm), torso_back ist abgenommen.
  // Die Lippen der Pfosten (xiao_lip 1,0, Spiel 0,3) werden vom Block beim Einrasten um 0,7 mm je Seite weggedrueckt (Schnapper, siehe asm_block_lip): der Sondierkoerper ist deshalb um 2 x 0,7 schmaler.
  place_torso() for (s = [0 : 1.5 : 30]) intersection() { torso_front(); cam_frame() translate([0, 0, -s]) block_probe(xiao_lip - xiao_play + 0.02); }
else if (part == "asm_block_ctrl")   // Kontrolle: Kamerakoerper +0,6 mm MUSS die Tasche treffen
  place_torso() for (s = [0 : 1.5 : 30]) intersection() { torso_front(); cam_frame() translate([0, 0, -s]) cam_body_probe(0.6); }
else if (part == "asm_block_lip")   // Kontrolle und Nachweis: voller Block trifft die Lippen (Schnapp-Ueberdeckung 0,7 mm je Seite), MUSS treffen
  place_torso() for (s = [0 : 1.5 : 30]) intersection() { torso_front(); cam_frame() translate([0, 0, -s]) block_probe(0); }
else if (part == "asm_close")  // torso_back wird in -Y auf torso_front geschoben (Zwischenstellungen bis 30 mm), Block sitzt schon: Anschlag und Lippen duerfen ihn nicht treffen
  for (s = [0.5, 1, 2, 3, 5, 8, 12, 20, 30]) intersection() { translate([0, s, 0]) place_torso() torso_back(); union() { xiao_dummy_r(); camera_dummy_r(); } }
else if (part == "asm_close_ctrl")   // Kontrolle: Block 1 mm zum Anschlag hin verschoben (Luft 0,15) MUSS den Anschlag treffen
  for (s = [0, 0.5, 1, 2, 3]) intersection() { translate([0, s, 0]) place_torso() torso_back(); place_torso() cam_frame() translate([0, 0, -1]) xiao_stack_dummy(cam_wt); }
else if (part == "torso_selftest")   // zu grosser Block (1,6-fach in X/Y um die Mitte der XIAO-Platine) MUSS die Schale (Wiege, Rahmen, Wand) treffen
  intersection() { shells(); place_torso() cam_frame() translate([xiao_pos[0], xiao_pos[1], 0]) scale([1.6, 1.6, 1]) translate([-xiao_pos[0], -xiao_pos[1], 0]) xiao_stack_dummy(cam_wt); }
// ---- Kamera-Pruefungen, alle in Oberkoerper-Koordinaten; jede "muss leer sein"-Pruefung hat eine Positiv-Kontrolle in check_roboter.ps1
else if (part == "cam_selftest")     // Kamerakoerper +0,6 mm (Tasche hat +0,4) MUSS die Tasche treffen
  intersection() { place_torso() torso_front(); place_torso() cam_frame() translate([-(cam_body + 0.6) / 2, -(cam_body + 0.6) / 2, cam_zbb(cam_wt)]) cube([cam_body + 0.6, cam_body + 0.6, cam_zbf(cam_wt) - cam_zbb(cam_wt)]); }
else if (part == "cam_collision_ctrl")   // Kontrolle zur Kollision: Kamera-Pruefkoerper 0,6 mm nach vorn (in den Taschenboden) MUSS treffen
  intersection() { place_torso() torso_front(); place_torso() cam_frame() translate([0, 0, 0.6]) cam_dummy(cam_wt); }
else if (part == "cone_ctrl")            // Kontrolle zum Sichtkegel: 8 mm nach hinten verschoben steckt die Spitze in Wand und Tasche, MUSS treffen
  intersection() { place_torso() torso_front(); place_torso() cam_frame() translate([0, 0, -8]) cam_fov_cone(cam_cone_l, cam_wt); }
else if (part == "lens_tip")         // Linsenspitze (Achsen-Probe Ø1, bis cam_tip_ref) darf die Aussenhuelle nicht durchstossen: leer. Bei zu duenner Wand (cam_wt < 3,6 + 0,3) ragt sie heraus.
  difference() { place_torso() cam_frame() translate([0, 0, cam_tip_ref - 1]) cylinder(d = 1, h = 1); place_torso() t_solid(); }
else if (part == "lens_tip_ctrl")    // Kontrolle: Linse 0,5 mm laenger ragt ueber die Aussenhuelle, MUSS treffen
  difference() { place_torso() cam_frame() translate([0, 0, cam_tip_ref - 1]) cylinder(d = 1, h = 1.5); place_torso() t_solid(); }
else if (part == "cradle_fit")       // Pfosten/Lippen muessen den um 0,6 mm aufgeweiteten Block beruehren (ohne Wiege: leer)
  place_torso() intersection() {
    cam_posts();
    cam_frame() xbox(0, 20.96, xiao_vmin, xiao_vmax, 0, 1.25, cam_wt, 0.6, 0.6, 0.6);
  }
else if (part == "cam_inside")       // Rahmen, Wiege und Anschlag (ohne Fuellblock) muessen mindestens 0,2 mm innerhalb der Brusthaut liegen; sie duerfen die Aussenhaut nicht durchstossen
  difference() { place_torso() cam_nofill(); place_torso() t_solid_in(0.2); }
else if (part == "inside_ctrl_z")    // Kontrolle: 30 mm nach oben verschoben ragt die Wiege aus der Brusthaut, MUSS treffen
  difference() { place_torso() translate([0, 0, 30]) cam_nofill(); place_torso() t_solid_in(0.2); }
else if (part == "inside_ctrl_x")    // Kontrolle: 30 mm zur Seite verschoben ragt die Wiege aus der Brusthaut, MUSS treffen
  difference() { place_torso() translate([30, 0, 0]) cam_nofill(); place_torso() t_solid_in(0.2); }
else if (part == "stack_inside")     // der XIAO-Block muss ganz im Innenraum der Brust liegen (nicht durch die Aussenhaut)
  difference() { xiao_dummy_r(); place_torso() t_solid_in(0.2); }
else if (part == "stack_inside_ctrl")   // Kontrolle: 40 mm nach vorn verschoben steckt der Block ausserhalb, MUSS treffen
  difference() { translate([0, -40, 0]) xiao_dummy_r(); place_torso() t_solid_in(0.2); }
else if (part == "cam_present")      // Rahmen, Wiege und Anschlag (ohne Tasche) muessen in torso_front + torso_back vollstaendig vorhanden sein (nichts durch Dome, Kanaele, Bohrungen weggeschnitten)
  difference() { difference() { place_torso() cam_nofill(); place_torso() { cam_frame() { cam_pocket_cut(cam_wt); xiao_cradle_cut(cam_wt); } cam_free_volumes(); vent_slots(); } } cam_parts_tol(); }
else if (part == "cam_present_ctrl") // Kontrolle: Wiege um 60 mm zur Seite (steht frei im Raum), MUSS als fehlend gemeldet werden
  difference() { difference() { place_torso() translate([60, 0, 0]) cam_nofill(); place_torso() { cam_frame() { cam_pocket_cut(cam_wt); xiao_cradle_cut(cam_wt); } cam_free_volumes(); vent_slots(); } } cam_parts_tol(); }
else if (part == "cam_channels")     // Rahmen, Wiege und Anschlag (ohne Fuellblock) duerfen weder die Werkzeugkanaele der Kopfschrauben noch das Kabelloch Ø10 schneiden
  intersection() { place_torso() cam_nofill(); place_torso() cam_free_volumes(); }
else if (part == "cam_channels_ctrl")   // Kontrolle: Kanaele und Kabelloch 8 mm zur Mitte hin (in X) verschoben MUSS Wiege/Anschlag treffen
  intersection() { place_torso() cam_nofill(); place_torso() translate([-8, 0, 0]) cam_free_volumes(); }
else if (part == "cam_section")    // Schnitt durch die Kamera-Tasche bei X = 0 (Blick von +X): jedes Teil einzeln geschnitten, damit die Schnittflaechen ihre Farbe behalten
  rotate([0, 0, 90]) rotate([0, -90, 0]) place_torso() {
    cb = [-25, -26, -2];   // Ausschnitt in Oberkoerper-Koordinaten, nur X <= 0
    intersection() { color("#ff7a29") torso_front(); translate(cb) cube([25, 40, 52]); }
    intersection() { color("#4fa8c9") torso_back(); translate(cb) cube([25, 40, 52]); }
    intersection() { color("#d03030") camera_dummy_l(); translate(cb) cube([25, 40, 52]); }
    intersection() { color("#8a4fc9") xiao_dummy_l(); translate(cb) cube([25, 40, 52]); }
    intersection() { color("#263238") neck(); translate(cb) cube([25, 40, 52]); }
  }
// ---- Anzeige-Teile fuer die Einbauanleitung (nur Ansicht, keine Druckteile; _l = Oberkoerper-Koordinaten, unter place_torso())
else if (part == "guide_1_pocket")   // Schritt 1: leere Tasche in torso_front (Rahmen, Pfosten mit Lippen)
  guide_c() color("#ff7a29") torso_front();
else if (part == "guide_2_block")    // Schritt 2: Block (Kamera + XIAO) 15 mm hinter der Sitzlage (lokal -Z), wird von hinten in die Tasche geschoben
  guide_c() {
    color("#ff7a29") torso_front();
    color("#d03030") cam_frame() translate([0, 0, -15]) cam_dummy(cam_wt);
    color("#3fbf5f") cam_frame() translate([0, 0, -15]) xiao_stack_dummy(cam_wt);
  }
else if (part == "guide_3_seated")   // Schritt 3: Block sitzt, Kamera in der Tasche, XIAO zwischen den Pfosten
  guide_c() {
    color("#ff7a29") torso_front();
    color("#d03030") camera_dummy_l();
    color("#3fbf5f") xiao_dummy_l();
  }
else if (part == "guide_4_close")    // Schritt 4: alles sitzt, torso_back 45 mm in +Y abgesetzt (halbtransparent)
  guide_c() {
    color("#ff7a29") torso_front();
    color("#d03030") camera_dummy_l();
    color("#3fbf5f") xiao_dummy_l();
    color([0.31, 0.66, 0.79, 0.2]) translate([0, 45, 0]) torso_back();
  }
else if (part == "assembly") assembly_parts();
else if (part == "exploded") exploded_parts();
else if (part == "section")
  difference() {
    assembly_parts();
    translate([0, -500, -500]) cube(1000);
  }
else if (part == "collision")
  intersection() { shells(); dummies(); }
else if (part == "selftest")
  // Positive-Kontrolle fuer die Kollisionspruefung: absichtlich zu grosse Platine, MUSS Treffer liefern
  intersection() { shells(); place_head() scale([1.1, 1.1, 1]) pcb_dummy(); }
else if (part == "pilot_test")
  // Nachweis Kernloecher: Bohrvolumen gegen head_back OHNE Kernloecher, muss Material treffen (Y-Ausdehnung >= neck_pilot_len)
  intersection() { head_back(false); neck_pilots(); }
else if (part == "info") info();
