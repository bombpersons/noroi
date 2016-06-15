#include <noroi/glfw/noroi_glfw_font.h>

#ifdef NOROI_USE_GLFW

#include <noroi/misc/noroi_glyphpacker.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Freetype.
#include <ft2build.h>
#include FT_FREETYPE_H

#define PAGE_WIDTH 1024
#define PAGE_HEIGHT 1024
#define PAGE_COUNT 10
#define MAX_QUADS_PER_FLUSH 2048

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

static uint32_t inline
decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}

FT_Library g_freetypeLibrary;
bool g_initialized = false;
bool NR_Font_Init() {
  // Only initialize once.
  if (!g_initialized) {
    // Intialize freetype.
    if (FT_Init_FreeType(&g_freetypeLibrary) != 0) {
      return false;
    }

    g_initialized = true;
  }

  return true;
}

void NR_Font_Shutdown() {
  if (g_initialized) {
    // De-initialize freetype.
    FT_Done_FreeType(g_freetypeLibrary);

    g_initialized = false;
  }
}

// Internal representation of NR_Font
typedef struct {
  GLfloat x, y;
} Vec2d;

typedef struct {
  Vec2d pos;
  Vec2d texcoord;
} Vertex;

typedef struct {
  Vertex vertices[6];
} Quad;

typedef struct {
  GLuint texture;

  GLuint vao;
  GLuint vbo;
  unsigned int curQuad;
} Page;

typedef struct {
  FT_Face face;
  NR_GlyphPacker* glyphpacker;

  // Pages
  Page* pages[PAGE_COUNT];

  // A shader to draw it with.
  GLuint program;
} HandleType;

NR_Font* NR_Font_Load(const char* path) {
  // Attempt to load the font.
  FT_Face face;
  FT_Error err = FT_New_Face(g_freetypeLibrary, path, 0, &face);
  if (err != 0)
    return (void*)0;

  // Make sure we're using unicode mappings.
  FT_Select_Charmap(face, FT_ENCODING_UNICODE);

  // Allocate some memory for our handle.
  HandleType* hnd = malloc(sizeof(HandleType));
  memset(hnd, 0, sizeof(HandleType));
  hnd->face = face;

  // Create a glyphpacker
  hnd->glyphpacker = NR_GlyphPacker_New(PAGE_WIDTH, PAGE_HEIGHT, PAGE_COUNT);

  // Set a default size.
  NR_Font_SetSize((void*)hnd, 0, 100);

  // Create a shader to draw with.
  const GLchar* vertexSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 posAttr;\n"
    "layout (location = 1) in vec2 texAttr;\n"
    "uniform mat4 proj;\n"
    "out vec2 texcoord;\n"
    "void main() {\n"
    "  gl_Position = proj * vec4(posAttr.x, posAttr.y, 0.0, 1.0);\n"
    "  texcoord = texAttr;\n"
    "}";

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, (void*)0);
  glCompileShader(vertexShader);

  {
    GLint success;
    GLchar log[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(vertexShader, sizeof(log), (void*)0, log);
      printf("Font vertex shader compile error:\n %s\n", log);
      return (void*)0;
    }
  }

  const GLchar* fragSource =
    "#version 330 core\n"
    "in vec2 texcoord;\n"
    "uniform sampler2D sampler;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  vec4 texCol = texture(sampler, texcoord);\n"
    "  color = vec4(1.0f, 1.0f, 1.0f, texCol.r);\n"
    "}";

  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fragSource, (void*)0);
  glCompileShader(fragShader);

  {
    GLint success;
    GLchar log[512];
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragShader, sizeof(log), (void*)0, log);
      printf("Font fragment shader compile error:\n %s\n", log);
      return (void*)0;
    }
  }

  hnd->program = glCreateProgram();
  glAttachShader(hnd->program, vertexShader);
  glAttachShader(hnd->program, fragShader);
  glLinkProgram(hnd->program);

  {
    GLint success;
    GLchar log[512];
    glGetShaderiv(fragShader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragShader, sizeof(log), (void*)0, log);
      printf("Font shader link error:\n %s\n", log);
      return (void*)0;
    }
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragShader);

  return (void*)hnd;
}

void NR_Font_Delete(NR_Font* font) {
  // Delete the font.
  HandleType* hnd = (HandleType*)font;

  // Delete the face.
  FT_Done_Face(hnd->face);

  // Delete our glyphpacker
  NR_GlyphPacker_Delete(hnd->glyphpacker);

  // Delete opengl resources
  for (int i = 0; i < PAGE_COUNT; ++i) {
    if (hnd->pages[i]) {
      glDeleteTextures(PAGE_COUNT, &hnd->pages[i]->texture);
      glDeleteVertexArrays(1, &hnd->pages[i]->vao);
      glDeleteBuffers(1, &hnd->pages[i]->vbo);
    }

    free(hnd->pages[i]);
  }
  glDeleteProgram(hnd->program);

  // De-allocate our handle.
  free(hnd);
}

void NR_Font_SetSize(NR_Font* font, int width, int height) {
  HandleType* hnd = (HandleType*)font;
  FT_Set_Pixel_Sizes(hnd->face, width, height);
}

static void _getOrthographicProjection(float left, float right, float bottom, float top, float near, float far, float* out) {
  out[0] = 2.0f / (right - left);
  out[1] = 0; out[2] = 0;
  out[3] = - (right + left) / (right - left);
  out[4] = 0;
  out[5] = 2.0f / (top - bottom);
  out[6] = 0;
  out[7] = - (top + bottom) / (top - bottom);
  out[8] = 0; out[9] = 0;
  out[10] = -2.0f / (far - near);
  out[11] = (far + near) / (far - near);
  out[12] = 0; out[13] = 0; out[14] = 0;
  out[15] = 1;
}

static void _flush(NR_Font* font, Page* page) {
  HandleType* hnd = (HandleType*)font;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Use our shader
  glUseProgram(hnd->program);

  // Use our texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, page->texture);
  glUniform1i(glGetUniformLocation(hnd->program, "sampler"), 0);

  GLfloat proj[16];
  _getOrthographicProjection(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, proj);
  glUniformMatrix4fv(glGetUniformLocation(hnd->program, "proj"), 1, true, proj);

  glBindVertexArray(page->vao);
  glDrawArrays(GL_TRIANGLES, 0, page->curQuad * (sizeof(Quad) / sizeof(Vertex)));
  glBindVertexArray(0);

  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);

  // Stop using our shader
  glUseProgram(0);

  // Reset this page.
  page->curQuad = 0;
}

bool NR_Font_Draw(NR_Font* font, unsigned int* data, int width, int height) {
  HandleType* hnd = (HandleType*)font;

  // HACK
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Where we are drawing to.
  // We don't need a matrix multiplacation in the shader
  // if we just generate the quads to fit the whole screen...
  float destX = 0;
  float destY = 0;
  float destWidth = 1;
  float destHeight = 1;

  // The size of cell in the grid
  GLfloat cellWidth = (float)destWidth / (float)width;
  GLfloat cellHeight = (float)destHeight / (float)height;

  int totalSize = width * height;
  for (int i = 0; i < totalSize; ++i) {
    int x = i % width;
    int y = i / width;
    unsigned int codepoint = data[i];

    // Check if we can find this codepoint.
    NR_GlyphPacker_Glyph glyph;
    glyph.codepoint = codepoint;
    bool found = NR_GlyphPacker_Find(hnd->glyphpacker, codepoint, &glyph);
    if (!found) {
      // We need to generate a glyph using freetype
      // And add it to our glypmap.
      unsigned long c = FT_Get_Char_Index(hnd->face, codepoint);
      FT_Error err = FT_Load_Glyph(hnd->face, c, FT_LOAD_RENDER);
      if (err != 0) return false;

      // Add it to our glyphmap.
      glyph.width = hnd->face->glyph->bitmap.width;
      glyph.height = hnd->face->glyph->bitmap.rows;
      glyph.advance = hnd->face->glyph->advance.x >> 6;
      glyph.bearingX = hnd->face->glyph->bitmap_left;
      glyph.bearingY = hnd->face->glyph->bitmap_top;
      NR_GlyphPacker_Add(hnd->glyphpacker, &glyph);
      found = NR_GlyphPacker_Find(hnd->glyphpacker, codepoint, &glyph);

      // Disable byte alignment restrictions for this
      // since our textures are only 8bit color!
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      // Add it to our texture
      if (!hnd->pages[glyph.page]) {
        // Allocate memory for a page.
        hnd->pages[glyph.page] = malloc(sizeof(Page));
        memset(hnd->pages[glyph.page], 0, sizeof(Page));

        // Create a texture for this page.
        GLuint texId;
        glGenTextures(1, &texId);
        hnd->pages[glyph.page]->texture = texId;

        // Bind it
        glBindTexture(GL_TEXTURE_2D, texId);

        // Allocate the texture memory.
        unsigned char data[PAGE_WIDTH * PAGE_HEIGHT];
        memset(data, 0, PAGE_WIDTH * PAGE_HEIGHT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, PAGE_WIDTH, PAGE_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, (void*)data);

        // Texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Unbind
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create a vertex buffer for this page.
        glGenVertexArrays(1, &hnd->pages[glyph.page]->vao);
        glGenBuffers(1, &hnd->pages[glyph.page]->vbo);
        glBindVertexArray(hnd->pages[glyph.page]->vao);
        glBindBuffer(GL_ARRAY_BUFFER, hnd->pages[glyph.page]->vbo);

        // Data
        glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * MAX_QUADS_PER_FLUSH, (void*)0, GL_DYNAMIC_DRAW);

        // Postions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Texcoords
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(Vec2d));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
      }

      // Either we managed to generate one, or there was already one generated.
      if (hnd->pages[glyph.page]) {
        // Bind the texture, and blit the image onto it.
        glBindTexture(GL_TEXTURE_2D, hnd->pages[glyph.page]->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, glyph.x, glyph.y, glyph.width, glyph.height, GL_RED, GL_UNSIGNED_BYTE, hnd->face->glyph->bitmap.buffer);
        glBindTexture(GL_TEXTURE_2D, 0);
      }
    }

    if (found) {
      Page* curPage = hnd->pages[glyph.page];

      // If we've gone over our maximum, flush first so we can draw some more!
      if (curPage->curQuad >= MAX_QUADS_PER_FLUSH)
        _flush(font, curPage);

      // Get the position of this glyph on texture in texture coordinates.
      GLfloat glyphTexX = (GLfloat)glyph.x / (GLfloat)PAGE_WIDTH;
      GLfloat glyphTexY = (GLfloat)glyph.y / (GLfloat)PAGE_HEIGHT;
      GLfloat glyphTexWidth = (GLfloat)glyph.width / (GLfloat)PAGE_WIDTH;
      GLfloat glyphTexHeight = (GLfloat)glyph.height / (GLfloat)PAGE_HEIGHT;

      // Get the maximum size for a glyph (in pixels)
      GLfloat maxWidth = ((GLfloat)(hnd->face->bbox.xMax - hnd->face->bbox.xMin) / (GLfloat)hnd->face->units_per_EM) * (GLfloat)hnd->face->size->metrics.x_ppem;
      GLfloat maxHeight = ((GLfloat)(hnd->face->bbox.yMax - hnd->face->bbox.yMin) / (GLfloat)hnd->face->units_per_EM) * (GLfloat)hnd->face->size->metrics.y_ppem;
      GLfloat ascender = ((GLfloat)hnd->face->ascender / (GLfloat)hnd->face->units_per_EM) * (GLfloat)hnd->face->size->metrics.y_ppem;

      // Where we will put our baseline.
      GLfloat baseline = ascender / maxHeight;

      // Left and top bearing
      //GLfloat bearingX = (GLfloat)glyph.bearingX / maxWidth;
      GLfloat bearingX = ((maxWidth - (GLfloat)glyph.width) / 2.0f) / maxWidth; // For now, just center the character.
      GLfloat bearingY = (GLfloat)glyph.bearingY / maxHeight;

      GLfloat glyphWidth = (GLfloat)glyph.width / maxWidth;
      GLfloat glyphHeight = (GLfloat)glyph.height / maxHeight;

      // The default line spacing (distance between lines, from the bottom of one line to the start of the next)
      //GLfloat defLineSpacingEM = (GLfloat)(hnd->face->height) - (maxHeightEM);

      // The start pos of this glyph
      GLfloat startX = destX + ((float)x + bearingX) * cellWidth;
      GLfloat startY = destY + ((float)y + baseline - bearingY) * cellHeight;
      GLfloat endX = startX + glyphWidth * cellWidth;
      GLfloat endY = startY + glyphHeight * cellHeight;

      // First triangle.
      Quad quad;
      quad.vertices[0].pos = (Vec2d){ startX, startY };
      quad.vertices[0].texcoord = (Vec2d){ glyphTexX, glyphTexY };
      quad.vertices[1].pos = (Vec2d){ endX, startY };
      quad.vertices[1].texcoord = (Vec2d){ glyphTexX + glyphTexWidth, glyphTexY };
      quad.vertices[2].pos = (Vec2d){ endX, endY };
      quad.vertices[2].texcoord = (Vec2d){ glyphTexX + glyphTexWidth, glyphTexY + glyphTexHeight };

      // Second triangle.
      quad.vertices[3] = quad.vertices[0];
      quad.vertices[4] = quad.vertices[2];
      quad.vertices[5].pos = (Vec2d){ startX, endY };
      quad.vertices[5].texcoord = (Vec2d){ glyphTexX, glyphTexY + glyphTexHeight };

      // Map the vertex buffer and add quad data at the correct location.
      glBindBuffer(GL_ARRAY_BUFFER, curPage->vbo);
      glBufferSubData(GL_ARRAY_BUFFER, curPage->curQuad * sizeof(Quad), sizeof(Quad), &quad);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      // We've added a quad.
      curPage->curQuad += 1;
    }
  }

  // Do a final flush for each page.
  for (int i = 0; i < PAGE_COUNT; ++i) {
    if (hnd->pages[i]) {
      _flush(font, hnd->pages[i]);
    }
  }

  return true;
}

#endif
