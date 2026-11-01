#include <unity.h>
#include "logic/face_model.h"
void setUp() {}
void tearDown() {}

void test_table_complete_for_all_emotions() {
  for (int i = 0; i < (int)Emotion::COUNT; i++) {
    FaceExpr x = face_expression((Emotion)i);
    TEST_ASSERT_TRUE(x.s.eye.w > 0 && x.s.eye.h > 0);
    TEST_ASSERT_TRUE(x.s.eye.open >= 0 && x.s.eye.open <= 1);
    TEST_ASSERT_TRUE(x.s.pupil_r >= 0);
    TEST_ASSERT_TRUE(x.s.mouth_w > 0);
  }
}
void test_happy_values_from_mockup() {
  const FaceShape s = face_expression(Emotion::HAPPY).s;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.45f, s.eye.lid_bottom_cut);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, s.mouth_curve);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, s.mouth_open);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 70.0f, s.mouth_w);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -16.0f, s.brow_y);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 64.0f, s.eye.w);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 72.0f, s.eye.h);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, s.eye.radius);
}
void test_annoyed_values_from_mockup() {
  const FaceExpr x = face_expression(Emotion::ANNOYED);
  TEST_ASSERT_EQUAL_UINT8(255, x.color.r);
  TEST_ASSERT_EQUAL_UINT8(50, x.color.g);
  TEST_ASSERT_EQUAL_UINT8(40, x.color.b);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.2f, x.s.brow_tilt);
  TEST_ASSERT_EQUAL((int)Mouth::ZIG, (int)x.s.mouth);
}
void test_mouth_shapes_and_wink() {
  TEST_ASSERT_EQUAL((int)Mouth::O, (int)face_expression(Emotion::SURPRISED).s.mouth);
  TEST_ASSERT_EQUAL((int)Mouth::TONGUE, (int)face_expression(Emotion::CHEEKY).s.mouth);
  TEST_ASSERT_TRUE(face_expression(Emotion::CHEEKY).s.wink);
  TEST_ASSERT_EQUAL((int)Mouth::KISS, (int)face_expression(Emotion::KISS).s.mouth);
  TEST_ASSERT_EQUAL((int)Mouth::WAVE, (int)face_expression(Emotion::CONFUSED).s.mouth);
  TEST_ASSERT_FALSE(face_expression(Emotion::CONTENT).s.wink);
}
void test_tween_dt0_and_converges() {
  const FaceShape a = face_expression(Emotion::CONTENT).s, b = face_expression(Emotion::HAPPY).s;
  FaceShape c = face_tween(a, b, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, a.mouth_w, c.mouth_w);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, a.brow_y, c.brow_y);
  for (int i = 0; i < 75; i++) c = face_tween(c, b, 20);
  TEST_ASSERT_FLOAT_WITHIN(0.15f, b.mouth_w, c.mouth_w);
  TEST_ASSERT_FLOAT_WITHIN(0.15f, b.brow_y, c.brow_y);
  TEST_ASSERT_FLOAT_WITHIN(0.15f, b.eye.lid_bottom_cut, c.eye.lid_bottom_cut);
  TEST_ASSERT_FLOAT_WITHIN(0.15f, b.pupil_r, c.pupil_r);
}
void test_discrete_fields_jump_in_second_half() {
  const FaceShape a = face_expression(Emotion::CONTENT).s, b = face_expression(Emotion::CHEEKY).s;
  TEST_ASSERT_TRUE(a.mouth != b.mouth && a.wink != b.wink);
  FaceShape c = face_tween(a, b, 20);
  TEST_ASSERT_EQUAL((int)a.mouth, (int)c.mouth);
  TEST_ASSERT_EQUAL(a.wink, c.wink);
  for (int i = 0; i < 75; i++) c = face_tween(c, b, 20);
  TEST_ASSERT_EQUAL((int)b.mouth, (int)c.mouth);
  TEST_ASSERT_EQUAL(b.wink, c.wink);
}
void test_eye_expression_matches_face() {
  for (int i = 0; i < (int)Emotion::COUNT; i++) {
    const Emotion e = (Emotion)i;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, face_expression(e).s.eye.w, eye_expression(e).shape.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, face_expression(e).s.eye.open, eye_expression(e).shape.open);
    TEST_ASSERT_EQUAL_UINT8(face_expression(e).color.g, eye_expression(e).color.g);
  }
}

void test_new_emotions_f3_f4() {
  FaceExpr x = face_expression(Emotion::HUNGRY);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, x.s.pupil_dy);
  TEST_ASSERT_EQUAL((int)Mouth::WAVE, (int)x.s.mouth);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 40.0f, x.s.mouth_w);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, x.s.brow_tilt);
  TEST_ASSERT_EQUAL_UINT8(255, x.color.r); TEST_ASSERT_EQUAL_UINT8(150, x.color.g); TEST_ASSERT_EQUAL_UINT8(40, x.color.b);
  x = face_expression(Emotion::FULL);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, x.s.eye.open);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.3f, x.s.eye.lid_top_cut);
  TEST_ASSERT_EQUAL((int)Mouth::WAVE, (int)x.s.mouth);
  TEST_ASSERT_EQUAL_UINT8(140, x.color.r); TEST_ASSERT_EQUAL_UINT8(200, x.color.g); TEST_ASSERT_EQUAL_UINT8(80, x.color.b);
  x = face_expression(Emotion::SICK);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.55f, x.s.eye.open);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.4f, x.s.eye.lid_top_tilt);
  TEST_ASSERT_EQUAL((int)Mouth::CURVE, (int)x.s.mouth);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.4f, x.s.mouth_curve);
  TEST_ASSERT_EQUAL_UINT8(120, x.color.r); TEST_ASSERT_EQUAL_UINT8(170, x.color.g); TEST_ASSERT_EQUAL_UINT8(120, x.color.b);
  x = face_expression(Emotion::FOCUSED);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.7f, x.s.eye.open);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, x.s.eye.lid_top_cut);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.3f, x.s.brow_tilt);
  TEST_ASSERT_EQUAL((int)Mouth::CURVE, (int)x.s.mouth);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, x.s.mouth_curve);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, x.s.mouth_w);
  TEST_ASSERT_EQUAL_UINT8(255, x.color.r); TEST_ASSERT_EQUAL_UINT8(90, x.color.g); TEST_ASSERT_EQUAL_UINT8(140, x.color.b);
}

void test_thinking_claude_mode() {
  const FaceExpr x = face_expression(Emotion::THINKING);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f, x.s.pupil_dx);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f, x.s.pupil_dy);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, x.s.brow_asym);
  TEST_ASSERT_EQUAL((int)Mouth::CURVE, (int)x.s.mouth);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, x.s.mouth_curve);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, x.s.mouth_w);
  TEST_ASSERT_EQUAL_UINT8(217, x.color.r); TEST_ASSERT_EQUAL_UINT8(119, x.color.g); TEST_ASSERT_EQUAL_UINT8(87, x.color.b);
  TEST_ASSERT_TRUE((int)Emotion::THINKING < (int)Emotion::COUNT);
}

void test_near_schielend() {
  const FaceExpr n = face_expression(Emotion::NEAR);
  TEST_ASSERT_TRUE(n.s.cross > 0);
  TEST_ASSERT_TRUE(n.s.eye.open >= face_expression(Emotion::SURPRISED).s.eye.open);
  TEST_ASSERT_EQUAL((int)Mouth::O, (int)n.s.mouth);
  // alle anderen Ausdrücke schielen nicht
  for (int i = 0; i < (int)Emotion::COUNT; i++)
    if ((Emotion)i != Emotion::NEAR) TEST_ASSERT_EQUAL_FLOAT(0, face_expression((Emotion)i).s.cross);
  // Tween mischt cross mit
  FaceShape c = face_expression(Emotion::CONTENT).s;
  for (int i = 0; i < 60; i++) c = face_tween(c, n.s, 50);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, n.s.cross, c.cross);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_table_complete_for_all_emotions);
  RUN_TEST(test_happy_values_from_mockup);
  RUN_TEST(test_annoyed_values_from_mockup);
  RUN_TEST(test_mouth_shapes_and_wink);
  RUN_TEST(test_tween_dt0_and_converges);
  RUN_TEST(test_discrete_fields_jump_in_second_half);
  RUN_TEST(test_eye_expression_matches_face);
  RUN_TEST(test_new_emotions_f3_f4);
  RUN_TEST(test_thinking_claude_mode);
  RUN_TEST(test_near_schielend);
  return UNITY_END();
}
