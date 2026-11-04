// MadBuddy: gemeinsame CYD-Masse (ESP32-2432S028, 2,8") -- Platine, Display, Rueckseite, Buchsen, Pruefkoerper
// Wird von den Gehaeuse-Dateien mit include <cyd_common.scad> eingebunden. OpenSCAD 2021.01.
// Gleiche Koordinaten wie madbuddy_case.scad: Gesichtsebene = Z 0, Inneres bei negativem Z, Ursprung X/Y = Mitte der aktiven Flaeche.
// Erwartet aus der einbindenden Datei (hier NICHT definieren, sonst doppelte Zuweisung = WARNING): front_t, glass_gap, fit, disp_x.
// Gehaeusespezifisches (hx/hy, rear_extra, Dome, XIAO, plugs_dummy, card_dummy) bleibt in der einbindenden Datei.

// ---------------------------------------------------------------- Pruefhilfe
window_shrink = 0;     // Pruefhilfe: Fenster um X mm schmaler (Gegenprobe der Kollisionspruefung), im Normalbetrieb 0

// ---------------------------------------------------------------- CYD-Platine
pcb_w      = 86.0;  // QUELLE: Originalzeichnung
pcb_h      = 50.0;  // QUELLE: Originalzeichnung
pcb_r      = 4.0;   // Eckradius der Platine -- QUELLE: Originalzeichnung (ca.)
pcb_t      = 1.6;   // VORLÄUFIG (Datenblatt) Platinendicke
hole_d     = 3.0;   // GEMESSEN am CYD (Nutzer 2026-11-03), Zeichnung sagte R 1,6 = 3,2. Zentrierstift = hole_d - 0,4
hole_off   = 4.0;   // QUELLE: Originalzeichnung: Lochmitte 4,0 mm von beiden Kanten (Lochabstand 78 x 42)

// ---------------------------------------------------------------- Displaymodul
glass_w    = 69.2;  // QUELLE: Originalzeichnung (Backlight-Aussenmass), sitzt an der Seite GEGENUEBER den USB-Buchsen
glass_h    = 50.0;  // QUELLE: Originalzeichnung
glass_t    = 2.3;   // QUELLE: Originalzeichnung (Modul ohne Kleber/Abstand)
disp_h     = 4.5;   // VORLÄUFIG (Datenblatt) Oberkante Display ueber Platine (Modul 2,3 + Klebeband/Abstand)
lcd_len    = 64.75; // QUELLE: Originalzeichnung: LCD-Glasflaeche in Laengsrichtung
aa_left    = 10.4;  // GEMESSEN am Testbild (Roboterkopf 2026-11-04): mit 13 rote Randlinie links verdeckt, rechts 3 mm schwarz -> 2,6 mm zurueck (Fenster je Seite 0,4 groesser als Bild). 2026-11-03 war 10 -> 13 (Schaetzung ohne Testbild). Linke Platinenkante (Blick von vorn) bis Beginn bespielte Flaeche
aa_meas    = 58.8;  // GEMESSEN: bespielte Breite (rechts bis Platinenkante rechnerisch ca. 14; frueher gemessen 16, passt nicht zum Druck)
act_w      = 59.0;  // GEMESSEN: bespielte Flaeche (58,8, aufgerundet)
act_h      = 44.3;  // VORLÄUFIG (aus Breite hochgerechnet, 320x240 Pixel) -- nachmessen
// Die Zeichnung (AA 57,6 mittig in 64,75 Glas) passt nicht zum Geraet: AA-Lage kommt aus der Messung.
// Quer ist der Versatz ca. 0,2 mm (vernachlaessigt).
win_off_x  = 0.0;   // Fenster-Versatz in X gegen die AA-Mitte (Variable zum Feinabgleich)
win_off_y  = 0.0;   // Fenster-Versatz in Y gegen die AA-Mitte (Variable zum Feinabgleich)
win_margin = 0.3;   // QUELLE: Hersteller empfiehlt Fenster = AA + 0,3 mm je Seite
win_r      = 0.5;   // Eckradius Fenster (klein, damit der Sichtkegel in die Ecken passt)
view_angle = 28;    // VORLÄUFIG (Datenblatt) Sichtwinkel (aus Fensterkante), Fenster ist nach vorn so angefast

// ---------------------------------------------------------------- Rueckseite der Platine
rear_h      = 8.0;  // VORLÄUFIG (Datenblatt) Bauteile auf der Rueckseite bis 8 mm
rear_margin = 7.5;  // VORLÄUFIG (Datenblatt) bauteilfreier Rand der Platinenrueckseite (Fuesse an den Loechern)
rear_clear  = 1.0;  // Luft zwischen Bauteilen und Rueckwand (Kabelweg)
// USB-Buchsen an der rechten Schmalseite (USB-Seite), auf der Platinenrueckseite
usb_edge_is_sd_side = true;  // GEMESSEN: Messkante A ist die Laengskante MIT dem SD-Slot (true = unten, SD-Seite)
micro_start = 9.67;  // GEMESSEN: Micro-USB beginnt 9,67 mm von Kante A
micro_w     = 7.6;   // VORLÄUFIG (Datenblatt) Micro-USB Breite (noch nicht gemessen)
usbc_start  = 17.88; // GEMESSEN: USB-C beginnt 17,88 mm von Kante A
usbc_w      = 9.44;  // GEMESSEN: USB-C Breite (Ende 27,32 mm; Gegenprobe 22,68 mm von der anderen Kante)
open_micro_usb = false; // Micro-USB bekommt keine Gehaeuseoeffnung (true = Oeffnung wird geschnitten)
sock_h      = 3.5;  // VORLÄUFIG (Datenblatt) Buchse Hoehe (Z)
sock_len    = 6.0;  // VORLÄUFIG (Datenblatt) Buchse Tiefe (X) auf der Platine
sock_over   = 0.5;  // VORLÄUFIG (Datenblatt) Ueberstand ueber die Platinenkante
usb_clear   = 0.3;  // Innenfreiraum zwischen Buchse und Wand
usbc_open_w = 15.0; // Wandoeffnung USB-C Breite (Stecker mit Huelle, +1 mm Zugabe rundum, VORLÄUFIG)
micro_open_w = 13.0; // Wandoeffnung Micro-USB Breite (nur bei open_micro_usb, VORLÄUFIG)
usb_open_h  = 9.5;  // Wandoeffnung Hoehe (VORLÄUFIG, +1 mm Zugabe rundum)
plug_w      = 12.0; // Steckerhuelle USB-C Breite (VORLÄUFIG, Pruefkoerper)
micro_plug_w = 10.0; // Steckerhuelle Micro-USB Breite (VORLÄUFIG, Pruefkoerper)
plug_h      = 6.5;  // Steckerhuelle Hoehe (VORLÄUFIG, Pruefkoerper)
btn_x_from_left = 68.4; // VORLÄUFIG (Datenblatt) BOOT/RESET Mitte X ab linker Platinenkante (Tastergruppe)
btn_y_from_bottom = 11.0; // VORLÄUFIG (Datenblatt) BOOT/RESET Mitte Y vom unteren Rand
btn_open_w  = 20.0; // VORLÄUFIG (Datenblatt) Aussparung Breite
btn_open_h  = 8.0;  // VORLÄUFIG (Datenblatt) Aussparung Hoehe
// SD-Kartenslot an der unteren Laengskante der Platine (Rueckseite), ragt ueber die Kante
sd_len      = 12.0; // VORLÄUFIG (aus Zeichnung abgeschaetzt) Laenge des Slots (X)
sd_over     = 1.5;  // VORLÄUFIG (aus Zeichnung abgeschaetzt) Ueberstand ueber die Platinenkante
sd_x_from_display = 48.0; // VORLÄUFIG (aus Zeichnung abgeschaetzt) Mitte, gemessen von der Display-Schmalseite
sd_h        = 2.0;  // VORLÄUFIG (Datenblatt) Slot-Hoehe (Z)
sd_depth    = 14.0; // VORLÄUFIG (Datenblatt) Slot-Tiefe auf der Platine (Y)
sd_open_w   = 16.0; // Wandoeffnung Breite fuer Karte (VORLÄUFIG)
sd_open_h   = 6.0;  // Wandoeffnung Hoehe (VORLÄUFIG)
card_w      = 11.0; // Karte Breite (microSD)
card_t      = 1.0;  // Karte Dicke

// ================================================================= abgeleitete Werte
pcb_x0    = -(aa_left + aa_meas / 2) + disp_x; // Platine linke Kante (Glas buendig aussen), Ursprung = Mitte der gemessenen AA (vor disp_x)
btn_x     = pcb_x0 + btn_x_from_left;
pcb_x1    = pcb_x0 + pcb_w;                   // Platine rechte Kante (USB-Seite)
glass_x1  = pcb_x0 + glass_w;
z_glass_top = -(front_t + glass_gap);
z_glass_bot = z_glass_top - disp_h;
z_pcb_top = z_glass_bot;
z_pcb_bot = z_pcb_top - pcb_t;
z_rear    = z_pcb_bot - rear_h;
usb_zc    = z_pcb_bot - sock_h / 2;
function edge_y(d) = usb_edge_is_sd_side ? -pcb_h / 2 + d : pcb_h / 2 - d;   // Position auf der USB-Seite, d gemessen von Kante A
micro_yc  = edge_y(micro_start + micro_w / 2);
usbc_yc   = edge_y(usbc_start + usbc_w / 2);
// [Mitte Y, Buchsenbreite, Oeffnungsbreite, Steckerbreite, Oeffnung schneiden?]
usb_list  = [[micro_yc, micro_w, micro_open_w, micro_plug_w, open_micro_usb], [usbc_yc, usbc_w, usbc_open_w, plug_w, true]];
win_bevel = 35;     // Fenster-Fasenwinkel nach vorn (muss > view_angle sein, Ueberhang <= 45 Grad)
sd_x      = pcb_x0 + sd_x_from_display;
sd_zc     = z_pcb_bot - sd_h / 2;
win_w     = act_w + 2 * win_margin - window_shrink;
win_h     = act_h + 2 * win_margin - window_shrink;
hole_pts  = [for (sx = [pcb_x0 + hole_off, pcb_x1 - hole_off], sy = [-1, 1]) [sx, sy * (pcb_h / 2 - hole_off)]];

// ================================================================= Formen und Pruefkoerper
module rrect(hx, hy, r) { offset(r = r) square([2 * (hx - r), 2 * (hy - r)], center = true); }
module window_cut() {
  // Fenster: hinten (Anlage am Glas) AA+0,3/Seite, nach vorn mit view_angle aufgeweitet
  e = (front_t + 0.01) * tan(win_bevel);
  translate([disp_x + win_off_x, win_off_y, 0])
    hull() {
      translate([0, 0, -front_t - 0.01]) linear_extrude(0.01) rrect(win_w / 2, win_h / 2, win_r);
      translate([0, 0, 0.01]) linear_extrude(0.01) rrect(win_w / 2 + e, win_h / 2 + e, win_r);
    }
}

module pcb_dummy() {
  difference() {
    translate([0, 0, z_pcb_bot]) linear_extrude(pcb_t)
      translate([(pcb_x0 + pcb_x1) / 2, 0]) rrect(pcb_w / 2, pcb_h / 2, pcb_r);
    for (p = hole_pts) translate([p[0], p[1], z_pcb_bot - 1]) cylinder(d = hole_d, h = pcb_t + 2);
  }
}
module glass_dummy() { translate([pcb_x0, -glass_h / 2, z_glass_bot]) cube([glass_w, glass_h, disp_h]); }
module rear_dummy() {
  translate([pcb_x0 + rear_margin, -pcb_h / 2 + rear_margin, z_rear])
    cube([pcb_w - 2 * rear_margin, pcb_h - 2 * rear_margin, rear_h]);
}
module sockets_dummy() {
  for (u = usb_list)
    translate([pcb_x1 + sock_over - sock_len, u[0] - u[1] / 2, z_pcb_bot - sock_h]) cube([sock_len, u[1], sock_h]);
}
module buttons_dummy() {
  for (dx = [-4, 4]) translate([btn_x + dx - 3, -pcb_h / 2 + btn_y_from_bottom - 3, z_pcb_bot - 3.5]) cube([6, 6, 3.5]);
}
module active_cone() {
  // aktive Flaeche + Sichtkegel nach vorn (view_angle)
  L = 25;
  translate([disp_x + win_off_x, win_off_y, 0]) hull() {
    translate([-act_w / 2, -act_h / 2, z_glass_top - 0.01]) cube([act_w, act_h, 0.01]);
    e = (L - z_glass_top) * tan(view_angle);
    translate([-act_w / 2 - e, -act_h / 2 - e, L]) cube([act_w + 2 * e, act_h + 2 * e, 0.01]);
  }
}
module sd_dummy() {
  translate([sd_x - sd_len / 2, -pcb_h / 2 - sd_over, z_pcb_bot - sd_h]) cube([sd_len, sd_depth + sd_over, sd_h]);
}
