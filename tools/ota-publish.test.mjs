import test from 'node:test';
import assert from 'node:assert/strict';
import { inspectImage, buildManifest, CAM_PROJECT } from './ota-publish.mjs';

function img(chip, proj, ver = '') {
  const b = Buffer.alloc(300);
  b[0] = 0xe9; b.writeUInt16LE(chip, 12); b.writeUInt32LE(0xabcd5432, 32);
  b.write(ver, 32 + 16, 'latin1');
  b.write(proj, 32 + 48, 'latin1');
  for (let i = 0; i < 32; i++) b[32 + 144 + i] = i;
  return b;
}
test('ESP32-Image wird erkannt', () => {
  const r = inspectImage(img(0, 'cyd'), 'cyd');
  assert.equal(r.error, undefined);
  assert.equal(r.elf, '000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f');
});
test('S3-Image wird abgelehnt', () => assert.match(inspectImage(img(9, 'cyd'), 'cyd').error, /falscher Chip/));
test('falsches Projekt', () => assert.match(inspectImage(img(0, 'x'), 'cyd').error, /falsches Projekt/));

test('cam-Image (S3, Chip 9) wird erkannt, Version aus dem Deskriptor', () => {
  const r = inspectImage(img(9, CAM_PROJECT, '0.1.0'), CAM_PROJECT, 9);
  assert.equal(r.error, undefined);
  assert.equal(r.version, '0.1.0');
});
test('ESP32-Image wird als cam-Image abgelehnt', () => assert.match(inspectImage(img(0, CAM_PROJECT), CAM_PROJECT, 9).error, /falscher Chip/));

test('Manifest mit cam: version, url, sha256', () => {
  const sha = 'AB'.repeat(32);
  const m = buildManifest({ project: 'p', sha256: 'x' }, { version: '0.1.0', sha256: sha }, 'v20261101-1200');
  assert.equal(m.project, 'p');
  assert.equal(m.cam.version, '0.1.0');
  assert.equal(m.cam.url, 'https://github.com/M4dm0nky/madbuddy/releases/download/v20261101-1200/cam.bin');
  assert.match(m.cam.sha256, /^[0-9a-f]{64}$/);
});
test('Manifest ohne cam hat kein cam-Feld', () => assert.equal('cam' in buildManifest({ project: 'p' }, null, 'v1'), false));
test('Manifest: leere oder zu lange cam-Version wird abgelehnt', () => {
  assert.throws(() => buildManifest({}, { version: '', sha256: 'a'.repeat(64) }, 'v1'), /Version/);
  assert.throws(() => buildManifest({}, { version: '1'.repeat(16), sha256: 'a'.repeat(64) }, 'v1'), /Version/);
});
