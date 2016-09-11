#include <unity.h>
#include <noroi/misc/noroi_glyphpacker.h>

#include <stdlib.h>

#define START_QUEUE_TEST(width, height, maxPage) \
  NR_GlyphPacker* g = NR_GlyphPacker_New(width, height, maxPage);

#define END_QUEUE_TEST \
  NR_GlyphPacker_Delete(g);

void test_add_remove() {
  START_QUEUE_TEST(1024, 1024, 5)

  // Add some things to test.
  const int toAdd = 4000;
  for (int i = 0; i < toAdd; ++i) {
    NR_GlyphPacker_Glyph glyph;
    glyph.width = 10 + rand() % 30;
    glyph.height = 10 + rand() % 30;
    glyph.codepoint = i;
    NR_GlyphPacker_Add(g, &glyph);
  }

  // Check if they are there.
  for (int i = 0; i < toAdd; ++i) {
    NR_GlyphPacker_Glyph glyph;
    TEST_ASSERT_MESSAGE(NR_GlyphPacker_Find(g, i, &glyph), "Couldn't find glyph in map!");
    TEST_ASSERT_MESSAGE(glyph.codepoint == i, "Found glyph doesn't match the one we were searching for!");
  }

  END_QUEUE_TEST
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_add_remove);
  return UNITY_END();
}
