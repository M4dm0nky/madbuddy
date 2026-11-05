#include <unity.h>
#include "logic/ota_policy.h"
void setUp() {}
void tearDown() {}

static uint8_t img[OTA_IMG_CHECK_BYTES];
static void make_img(uint16_t chip, const char *proj) {
  memset(img, 0, sizeof img);
  img[0] = 0xE9;
  img[12] = chip & 0xFF; img[13] = chip >> 8;
  img[32] = 0x32; img[33] = 0x54; img[34] = 0xCD; img[35] = 0xAB;
  strncpy((char *)img + 32 + 48, proj, 32);
  for (int i = 0; i < 32; i++) img[32 + 144 + i] = (uint8_t)i;
}
void test_image_ok_esp32() {
  make_img(0, "cyd");
  char elf[65];
  TEST_ASSERT_EQUAL(OTA_IMG_OK, ota_image_check(img, sizeof img, "cyd", elf));
  TEST_ASSERT_EQUAL_STRING("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", elf);
}
void test_image_rejects() {
  char elf[65];
  make_img(9, "cyd"); TEST_ASSERT_EQUAL(OTA_IMG_CHIP, ota_image_check(img, sizeof img, "cyd", elf));
  make_img(0, "x");   TEST_ASSERT_EQUAL(OTA_IMG_PROJECT, ota_image_check(img, sizeof img, "cyd", elf));
  make_img(0, "cyd"); img[0] = 0;  TEST_ASSERT_EQUAL(OTA_IMG_MAGIC, ota_image_check(img, sizeof img, "cyd", elf));
  make_img(0, "cyd"); img[32] = 0; TEST_ASSERT_EQUAL(OTA_IMG_DESC, ota_image_check(img, sizeof img, "cyd", elf));
  TEST_ASSERT_EQUAL(OTA_IMG_SHORT, ota_image_check(img, 100, "cyd", elf));
}
void test_needs_update() {
  const char *a = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
  const char *b = "ff0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
  TEST_ASSERT_FALSE(ota_needs_update(a, a));
  TEST_ASSERT_TRUE(ota_needs_update(a, b));
  TEST_ASSERT_FALSE(ota_needs_update(a, "kaputt"));
}
void test_redirect_authority_tricks() {
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com:@evil.com/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com:x@evil.com/"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com?@evil.com/"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com#@evil.com/"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://user@github.com/"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com:44a3/x"));
  TEST_ASSERT_TRUE(ota_redirect_ok("https://github.com:443/x"));
  TEST_ASSERT_TRUE(ota_redirect_ok("https://github.com:/x"));   // leerer Port ist gültig
}
void test_redirect_ok() {
  TEST_ASSERT_TRUE(ota_redirect_ok("https://github.com/M4dm0nky/madbuddy/releases/latest/download/manifest.json"));
  // Alter Repo-Name: GitHub leitet um, muss weiter akzeptiert werden.
  TEST_ASSERT_TRUE(ota_redirect_ok("https://github.com/M4dm0nky/cyd-buddy/releases/latest/download/manifest.json"));
  TEST_ASSERT_TRUE(ota_redirect_ok("https://release-assets.githubusercontent.com/github-production-release-asset/1?x=1"));
  TEST_ASSERT_TRUE(ota_redirect_ok("https://objects.githubusercontent.com/abc"));
  TEST_ASSERT_TRUE(ota_redirect_ok("https://GitHub.com/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("http://github.com/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://evilgithub.com/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com.evil.io/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://a.githubusercontent.com.evil.io/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://githubusercontent.com/x"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https:///x"));
  TEST_ASSERT_FALSE(ota_redirect_ok(nullptr));
}
void test_notice_and_progress() {
  TEST_ASSERT_FALSE(ota_notice_due(1000, 0, false));
  TEST_ASSERT_TRUE(ota_notice_due(1000, 0, true));
  TEST_ASSERT_FALSE(ota_notice_due(3000, 2000, true));
  TEST_ASSERT_EQUAL(50, ota_progress_pct(50, 100));
  TEST_ASSERT_EQUAL(0, ota_progress_pct(1, 0));
}

void test_notice_snooze_one_hour_and_2_31() {
  TEST_ASSERT_FALSE(ota_notice_due(1000 + OTA_NAG_MS - 1, 1000, true));
  TEST_ASSERT_TRUE(ota_notice_due(1000 + OTA_NAG_MS, 1000, true));
  TEST_ASSERT_TRUE(ota_notice_due(1000UL + OTA_NAG_MS + 0x80000000UL + 5, 1000, true));   // vor 24,8 Tagen geschlummert
  TEST_ASSERT_FALSE(ota_notice_due(1000UL + OTA_NAG_MS + 0x80000000UL + 5, 1000, false));
}
void test_image_chip_s3() {
  char elf[65];
  make_img(9, "cam");
  TEST_ASSERT_EQUAL(OTA_IMG_CHIP, ota_image_check(img, sizeof img, "cam", elf));                    // Vorgabe bleibt ESP32
  TEST_ASSERT_EQUAL(OTA_IMG_OK, ota_image_check(img, sizeof img, "cam", elf, OTA_CHIP_ESP32S3));
  make_img(0, "cam");
  TEST_ASSERT_EQUAL(OTA_IMG_CHIP, ota_image_check(img, sizeof img, "cam", elf, OTA_CHIP_ESP32S3));
}
void test_cam_newer() {
  TEST_ASSERT_TRUE(ota_cam_newer("0.1.0", "0.1.1"));
  TEST_ASSERT_TRUE(ota_cam_newer("0.1.1", "0.2.0"));
  TEST_ASSERT_TRUE(ota_cam_newer("0.2.0", "1.0.0"));
  TEST_ASSERT_TRUE(ota_cam_newer("0.9.0", "0.10.0"));            // numerisch, nicht als Text
  TEST_ASSERT_FALSE(ota_cam_newer("0.1.0", "0.1.0"));            // gleich
  TEST_ASSERT_FALSE(ota_cam_newer("1.0.0", "0.2.0"));            // Manifest älter
  TEST_ASSERT_FALSE(ota_cam_newer("", "1.0.0"));                 // keine Verbindung = kein Angebot
  TEST_ASSERT_FALSE(ota_cam_newer(nullptr, "1.0.0"));
  TEST_ASSERT_FALSE(ota_cam_newer("0.1.0", ""));
  TEST_ASSERT_FALSE(ota_cam_newer("0.1.0-broken", "0.1.0"));     // Zusatz hinter der Version = unbekannt
  TEST_ASSERT_FALSE(ota_cam_newer("0.1.0-broken", "0.2.0"));
  TEST_ASSERT_FALSE(ota_cam_newer("0.1.0", "0.2.0x"));
  TEST_ASSERT_FALSE(ota_cam_newer("abc", "1.0.0"));
  TEST_ASSERT_FALSE(ota_cam_newer("0.1", "1.0.0"));
  TEST_ASSERT_FALSE(ota_cam_newer("0.1.0", "1.0.0.0"));
}
void test_card_list() {
  // Zustände: 0 IDLE, 1 AVAILABLE, 2 DOWNLOADING, 3 VERIFYING, 4 RESTARTING, 5 FAILED
  TEST_ASSERT_TRUE(ota_card_list(0, false, true));
  TEST_ASSERT_TRUE(ota_card_list(1, false, true));
  TEST_ASSERT_FALSE(ota_card_list(1, true, true));      // Zustimmung gegeben: "Starte ..."
  for (int s = 2; s <= 5; s++) TEST_ASSERT_FALSE(ota_card_list(s, false, true));
  for (int s = 0; s <= 5; s++) TEST_ASSERT_FALSE(ota_card_list(s, false, false));   // ohne Angebot nie
}
void test_redirect_control_chars() {
  TEST_ASSERT_TRUE(ota_redirect_ok("https://github.com/a/b.bin"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com/a\tb"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com/a\nb"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com/a\rb"));
  TEST_ASSERT_FALSE(ota_redirect_ok("https://github.com/a b"));
}
void test_release_url_pinned() {
  const char *ok = "https://github.com/M4dm0nky/madbuddy/releases/download/v20261103-2108/madbuddy-cam.bin";
  TEST_ASSERT_TRUE(ota_release_url_ok(ok));
  TEST_ASSERT_FALSE(ota_release_url_ok("https://github.com/evil/madbuddy/releases/download/v1/x.bin"));
  TEST_ASSERT_FALSE(ota_release_url_ok("https://github.com/M4dm0nky/madbuddy-evil/releases/download/v1/x.bin"));
  TEST_ASSERT_FALSE(ota_release_url_ok("https://github.com/M4dm0nky/madbuddy/releases/download/../../../evil/x.bin"));
  TEST_ASSERT_FALSE(ota_release_url_ok("https://github.com/M4dm0nky/madbuddy/releases/download/v1/..%2f"));
  TEST_ASSERT_FALSE(ota_release_url_ok("http://github.com/M4dm0nky/madbuddy/releases/download/v1/x.bin"));
  TEST_ASSERT_FALSE(ota_release_url_ok("https://objects.githubusercontent.com/M4dm0nky/madbuddy/releases/download/v1/x.bin"));
  TEST_ASSERT_FALSE(ota_release_url_ok("https://github.com/M4dm0nky/madbuddy/releases/download/"));
  TEST_ASSERT_FALSE(ota_release_url_ok(nullptr));
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_image_ok_esp32);
  RUN_TEST(test_image_rejects);
  RUN_TEST(test_needs_update);
  RUN_TEST(test_redirect_ok);
  RUN_TEST(test_redirect_authority_tricks);
  RUN_TEST(test_notice_and_progress);
  RUN_TEST(test_notice_snooze_one_hour_and_2_31);
  RUN_TEST(test_image_chip_s3);
  RUN_TEST(test_cam_newer);
  RUN_TEST(test_card_list);
  RUN_TEST(test_redirect_control_chars);
  RUN_TEST(test_release_url_pinned);
  return UNITY_END();
}
