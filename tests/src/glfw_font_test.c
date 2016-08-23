#include <unity.h>
#include <noroi/noroi.h>
#include <noroi/glfw/noroi_glfw_font.h>

#include <glad/glad.h>

#ifdef NOROI_USE_GLFW

void test_draw() {
  TEST_ASSERT_MESSAGE(NR_Init(), "Couldn't initialize noroi!");

  NR_Handle hnd = NR_CreateHandle();
  TEST_ASSERT_MESSAGE(hnd, "Couldn't create a noroi handle!");

  TEST_ASSERT_MESSAGE(NR_Font_Init(), "Couldn't initialize freetype!");
  NR_Font* f = NR_Font_Load("data/font.ttf");
  TEST_ASSERT_MESSAGE(f, "Couldn't load data/font.ttf");

  //NR_Font_Draw(f, "Test!", 1024, 1024);

  NR_Font_Delete(f);

  f = 0;
  f = NR_Font_Load("tahoma");
  TEST_ASSERT_MESSAGE(f, "Couldn't load tahoma font!");

  NR_Font_Shutdown();

  NR_DestroyHandle(hnd);

  NR_Shutdown();
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_draw);
  return UNITY_END();
}

#else

int main(int argc, char** argv) {
  return 1;
}

#endif
