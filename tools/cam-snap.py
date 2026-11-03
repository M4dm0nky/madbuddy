#!/usr/bin/env python3
"""Kamerabild vom CYD holen (Diagnose): sendet "snap" über USB, setzt die J-Zeilen des XIAO zu einem JPEG zusammen.

  python tools/cam-snap.py COM6 --out bild.jpg [--seconds 12]

Zeilen: J B <len> / J D <seq> <base64> / J E / J X <grund>, jeweils mit *XX (XOR über den Inhalt davor)."""
import argparse
import base64
import sys
import time


def check_line(line):
    """'inhalt*XX' -> inhalt bei gültiger Prüfsumme, sonst None."""
    line = line.strip()
    if len(line) < 4 or line[-3] != '*':
        return None
    try:
        want = int(line[-2:], 16)
    except ValueError:
        return None
    content = line[:-3]
    x = 0
    for ch in content.encode('latin-1', 'replace'):
        x ^= ch
    return content if x == want else None


class Assembler:
    """Sammelt Zeilen; result() liefert (jpeg|None, Meldung)."""

    def __init__(self):
        self.total = None
        self.chunks = {}
        self.bad = 0
        self.done = False
        self.error = None

    def feed(self, raw):
        if not raw.startswith('J '):
            return
        c = check_line(raw)
        if c is None:
            self.bad += 1
            return
        p = c.split(' ')
        try:
            if p[1] == 'B' and len(p) == 3:
                self.total = int(p[2])
            elif p[1] == 'D' and len(p) == 4:
                self.chunks[int(p[2])] = base64.b64decode(p[3], validate=True)
            elif p[1] == 'E' and len(p) == 2:
                self.done = True
            elif p[1] == 'X':
                self.error = ' '.join(p[2:]) or 'unbekannt'
            else:
                self.bad += 1
        except ValueError:
            self.bad += 1

    def finished(self):
        return self.done or self.error is not None

    def result(self):
        if self.error is not None:
            return None, 'XIAO meldet Fehler: ' + self.error
        if not self.done:
            return None, 'Zeitlimit (kein J E), %d Stuecke, %d fehlerhafte Zeilen' % (len(self.chunks), self.bad)
        if self.total is None:
            return None, 'J B fehlt'
        n = (max(self.chunks) + 1) if self.chunks else 0
        missing = [i for i in range(n) if i not in self.chunks]
        if missing:
            return None, 'Luecke: Stuecke %s fehlen, %d fehlerhafte Zeilen' % (missing[:10], self.bad)
        data = b''.join(self.chunks[i] for i in range(n))
        if len(data) != self.total:
            return None, 'Laenge %d statt %d, %d fehlerhafte Zeilen' % (len(data), self.total, self.bad)
        return data, 'OK %d Bytes, %d Stuecke, %d fehlerhafte Zeilen' % (len(data), n, self.bad)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('port')
    ap.add_argument('--out', required=True)
    ap.add_argument('--seconds', type=float, default=12)
    a = ap.parse_args()
    import serial
    s = serial.Serial()
    s.port = a.port
    s.baudrate = 115200
    s.timeout = 0.5
    s.dtr = False   # kein Reset des Geraets beim Oeffnen
    s.rts = False
    s.open()
    asm = Assembler()
    try:
        time.sleep(0.5)
        s.reset_input_buffer()
        s.write(b'snap\n')
        end = time.time() + a.seconds
        while time.time() < end and not asm.finished():
            raw = s.readline().decode('latin-1', 'replace').strip()
            if raw:
                asm.feed(raw)
    finally:
        s.close()
    data, msg = asm.result()
    print(msg)
    if data is None:
        return 1
    with open(a.out, 'wb') as f:
        f.write(data)
    return 0


if __name__ == '__main__':
    sys.exit(main())
