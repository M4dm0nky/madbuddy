"""Offline-Test fuer tools/cam-snap.py:  python tools/cam_snap_test.py"""
import base64
import importlib.util
import os

spec = importlib.util.spec_from_file_location('cam_snap', os.path.join(os.path.dirname(__file__), 'cam-snap.py'))
cs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cs)


def seal(c):
    x = 0
    for b in c.encode():
        x ^= b
    return '%s*%02X' % (c, x)


def build(data, chunk=222, drop=None, corrupt=None):
    lines = [seal('J B %d' % len(data))]
    for seq, off in enumerate(range(0, len(data), chunk)):
        l = seal('J D %d %s' % (seq, base64.b64encode(data[off:off + chunk]).decode()))
        if seq == corrupt:
            l = l[:-2] + ('00' if l[-2:] != '00' else '01')   # absichtlich falsche Pruefsumme
        if seq != drop:
            lines.append(l)
    lines.append(seal('J E'))
    return lines


def run(lines):
    a = cs.Assembler()
    for l in ['# cam 0.1.1', 'cam: conn=1'] + lines:
        a.feed(l)
    return a.result()


data = bytes(range(256)) * 5
d, msg = run(build(data))
assert d == data, msg
assert 'OK 1280 Bytes, 6 Stuecke, 0 ' in msg, msg

d, msg = run(build(data, corrupt=2))   # falsche Pruefsumme -> Stueck fehlt
assert d is None and 'Luecke' in msg and '1 fehlerhafte' in msg, msg

d, msg = run(build(data, drop=1))
assert d is None and 'Luecke' in msg, msg

d, msg = run([seal('J X cam_off')])
assert d is None and 'cam_off' in msg, msg

d, msg = run(build(data)[:-1])   # J E fehlt
assert d is None and 'Zeitlimit' in msg, msg

assert cs.check_line('J E*00') is None and cs.check_line(seal('J E')) == 'J E'
print('cam_snap_test: OK')
