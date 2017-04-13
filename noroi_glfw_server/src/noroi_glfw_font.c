#include <noroi/glfw_server/noroi_glfw_font.h>

#include <noroi/glfw_server/noroi_font_retriever.h>
#include <noroi/glfw_server/noroi_glyphpacker.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/glad.h>

#include <stddef.h>

// Freetype.
#include <ft2build.h>
#include FT_FREETYPE_H

// Include shaders.
#include <noroi/glfw_server/shaders/fragment.h>
#include <noroi/glfw_server/shaders/geometry.h>
#include <noroi/glfw_server/shaders/vertex.h>

#define PAGE_WIDTH 1024
#define PAGE_HEIGHT 1024
#define PAGE_COUNT 10
#define MAX_VERTICES_PER_FLUSH 2048

// Utility function for loading a shader.
GLuint loadShader(const char* source, GLenum type) {
  // Create a shader to draw with.
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, (const GLchar**)&source, (void*)0);
  glCompileShader(shader);

  printf("Compiling shader: %s\n", source);

  // Compile it.
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    GLint maxLength = 0;
  	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    // Allocate space for log.
    char* log = malloc(sizeof(char) * maxLength);

    // Get name of shader type.
    const char* shaderTypeString;
    if (type == GL_GEOMETRY_SHADER) shaderTypeString = "geometry";
    if (type == GL_VERTEX_SHADER) shaderTypeString = "vertex";
    if (type == GL_FRAGMENT_SHADER) shaderTypeString = "fragment";

    glGetShaderInfoLog(shader, sizeof(char) * maxLength, &maxLength, log);
    printf("Error compiling %s shader: %s\n", shaderTypeString, log);

    // Free log
    free(log);

    return 0;
  }

  return shader;
}

// Link shaders together into a single program.
GLuint linkProgram(GLuint* shaders, int count) {
  // Create the program, attache the shaders and link the program.
  GLuint program = glCreateProgram();
  for (int i = 0; i < count; ++i) {
    glAttachShader(program, shaders[i]);
  }
  glLinkProgram(program);

  // Get the status of the linking.
  GLint success;
  GLchar log[512];
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, sizeof(log), (void*)0, log);
    printf("Font shader link error:\n %s\n", log);
    return 0;
  }

  return program;
}

// Vectors
typedef struct {
  GLfloat x, y;
} Vec2d;

typedef struct {
  GLfloat x, y, z;
} Vec3d;

typedef struct {
  GLfloat x, y, z, w;
} Vec4d;

// Vertex flags.
typedef enum {
  VERTEX_FLAGS_FLASHING = 1,
  VERTEX_FLAGS_ITALICS = 2,
  VERTEX_FLAGS_BOLD = 4
} VertexFlags;

// Define a vertex in our vertex buffer.
typedef struct {
  // Colors for the ghyph and the background.
  Vec3d color;
  Vec3d bgColor;

  // Rect for the glyph.
  Vec4d glyphRect;

  // Our texture coordinates.
  Vec4d textureRect;

  // Rect for the background color rect.
  Vec4d bgRect;

  // Glyph options.
  unsigned char flags;
} Vertex;

// Defines a single page of glyphs.
typedef struct {
  GLuint texture;
  GLuint vao;
  GLuint vbo;
  unsigned int current;
} Page;

// Internal representation of NR_Font
typedef struct {
  // The actual font.
  FT_Face face;

  // Maximum char width and height.
  int charWidth, charHeight;

  // Packed textures containing the font glyphs.
  NR_GlyphPacker* glyphpacker;

  // Pages
  Page* pages[PAGE_COUNT];

  // A VBO and VAO to draw the background colors.
  GLuint bgVao;
  GLuint bgVbo;

  // A shader to draw characters with.
  GLuint program;
} HandleType;

// Initialize freetype.
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

// Shutdown freetype.
void NR_Font_Shutdown() {
  if (g_initialized) {
    // De-initialize freetype.
    FT_Done_FreeType(g_freetypeLibrary);

    g_initialized = false;
  }
}

// Load a freetype font.
NR_Font NR_Font_Load(const char* path) {
  // Attempt to load the font.
  FT_Face face;
  FT_Error err = FT_New_Face(g_freetypeLibrary, path, 0, &face);

  if (err != 0) {
    // Try treating the path as a font descriptor
    // Get the font file data and attempt to load that.
    unsigned int size = NR_FontRetrieval_GetFontDataSize(path);
    if (size > 0) {
      char* buff = malloc(sizeof(char) * size);
      if (NR_FontRetrieval_GetFontData(path, buff, size)) {
       err = FT_New_Memory_Face(g_freetypeLibrary, (unsigned char*)buff, size, 0, &face);
      }
      free(buff);
    }

    // Still couldn't load anything.
    if (err != 0) {
      return (void*)0;
    }
  }

  // Make sure we're using unicode mappings.
  FT_Select_Charmap(face, FT_ENCODING_UNICODE);

  // Allocate some memory for our handle.
  HandleType* hnd = malloc(sizeof(HandleType));
  memset(hnd, 0, sizeof(HandleType));
  hnd->face = face;

  // Create a glyphpacker
  hnd->glyphpacker = NR_GlyphPacker_New(PAGE_WIDTH, PAGE_HEIGHT, PAGE_COUNT);

  // Set a default size.
  NR_Font_SetResolution((void*)hnd, 0, 25);
  NR_Font_SetSize((void*)hnd, 0, 25);

  // Create a shader to draw with.
  GLuint vertexShader = loadShader(vertex_src, GL_VERTEX_SHADER);
  GLuint geometryShader = loadShader(geometry_src, GL_GEOMETRY_SHADER);
  GLuint fragShader = loadShader(fragment_src, GL_FRAGMENT_SHADER);

  GLuint shaders[] = { vertexShader, geometryShader, fragShader };
  hnd->program = linkProgram(shaders, 3);

  glDeleteShader(vertexShader);
  glDeleteShader(geometryShader);
  glDeleteShader(fragShader);

  return (void*)hnd;
}

// Delete a font.
void NR_Font_Delete(NR_Font font) {
  // Delete the font.
  HandleType* hnd = (HandleType*)font;

  // Delete the face.
  FT_Done_Face(hnd->face);

  // Delete our glyphpacker
  NR_GlyphPacker_Delete(hnd->glyphpacker);

  // Delete opengl resources
  for (int i = 0; i < PAGE_COUNT; ++i) {
    if (hnd->pages[i]) {
      glDeleteTextures(1, &hnd->pages[i]->texture);
      glDeleteVertexArrays(1, &hnd->pages[i]->vao);
      glDeleteBuffers(1, &hnd->pages[i]->vbo);
    }

    free(hnd->pages[i]);
  }
  glDeleteProgram(hnd->program);

  // De-allocate our handle.
  free(hnd);
}

// Set the resolution of each
void NR_Font_SetResolution(NR_Font font, int width, int height) {
  HandleType* hnd = (HandleType*)font;
  FT_Set_Pixel_Sizes(hnd->face, width, height);

  // Invalidate all of the pages we have cached.
  NR_GlyphPacker_Delete(hnd->glyphpacker);
  hnd->glyphpacker = NR_GlyphPacker_New(PAGE_WIDTH, PAGE_HEIGHT, PAGE_COUNT);
}

void NR_Font_SetSize(NR_Font font, int width, int height) {
  HandleType* hnd = (HandleType*)font;

  // Get the current size of a glyph in the font.
  float maxWidth = hnd->face->bbox.xMax - hnd->face->bbox.xMin;
  float maxHeight = hnd->face->bbox.yMax - hnd->face->bbox.yMin;

  // Width is zero, so use the height and the aspect ratio to automatically calculate the width.
  if (width == 0 && height != 0) {
    width = (int)((float)height * (maxWidth / maxHeight));

  // Height is zero, so use the width and the aspect ratio to automatically calculate the height.
  } else if (height == 0 && width != 0) {
    height = (int)((float)width * (maxHeight / maxWidth));
  }

  hnd->charWidth = width;
  hnd->charHeight = height;
}

void NR_Font_GetSize(NR_Font font, int* width, int* height) {
  HandleType* hnd = (HandleType*)font;
  *width = hnd->charWidth;
  *height = hnd->charHeight;
}

#undef near
#undef far
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

static void _flush(NR_Font font, Page* page, int width, int height) {
  HandleType* hnd = (HandleType*)font;

  // Enable blending.
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Use our shader
  glUseProgram(hnd->program);

  // Use our texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, page->texture);
  glUniform1i(glGetUniformLocation(hnd->program, "sampler"), 0);

  // Set our projection matrix.
  GLfloat proj[16];
  _getOrthographicProjection(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, proj);
  glUniformMatrix4fv(glGetUniformLocation(hnd->program, "proj"), 1, true, proj);

  // Set the current time (useful for effets)
  glUniform1f(glGetUniformLocation(hnd->program, "timer"), (float)glfwGetTime());

  // Bind and draw the vertex array.
  glBindVertexArray(page->vao);
  glDrawArrays(GL_POINTS, 0, page->current);
  glBindVertexArray(0);

  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);

  // Stop using our shader
  glUseProgram(0);

  // Reset this page.
  page->current = 0;
}

// Draw a grid of characters.
bool NR_Font_Draw(NR_Font font, NR_Glyph* data, int dataWidth, int dataHeight, int startX, int startY, int width, int height) {
  HandleType* hnd = (HandleType*)font;

  // Where to start drawing.
  float destX = (float)startX;
  float destY = (float)startY;

  // The size of each cell in the grid
  GLfloat cellWidth = (GLfloat)hnd->charWidth;
  GLfloat cellHeight = (GLfloat)hnd->charHeight;

  // Loop through each item in the data we are trying to draw.
  int totalSize = dataWidth * dataHeight;
  for (int i = 0; i < totalSize; ++i) {
    // Get the grid coordinates of this character.
    int x = i % dataWidth;
    int y = i / dataWidth;

    // Get the codepoint to search for.
    unsigned int codepoint = data[i].codepoint;

    // Convert the colors to vectors.
    unsigned int color = data[i].color;
    unsigned int bgColor = data[i].bgColor;

    Vec3d colorVec;
    colorVec.x = (float)((color & (0xFF000000)) >> 24) / 255.0;
    colorVec.y = (float)((color & (0x00FF0000)) >> 16) / 255.0;
    colorVec.z = (float)((color & (0x0000FF00)) >> 8) / 255.0;
    Vec3d bgColorVec;
    bgColorVec.x = (float)((bgColor & (0xFF000000)) >> 24) / 255.0;
    bgColorVec.y = (float)((bgColor & (0x00FF0000)) >> 16) / 255.0;
    bgColorVec.z = (float)((bgColor & (0x0000FF00)) >> 8) / 255.0;

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
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MAX_VERTICES_PER_FLUSH, (void*)0, GL_DYNAMIC_DRAW);

        // Specify the buffer format...
        GLint colorAttrib = glGetAttribLocation(hnd->program, "vColor");
        glEnableVertexAttribArray(colorAttrib);
        glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        GLint bgColorAttrib = glGetAttribLocation(hnd->program, "vBgColor");
        glEnableVertexAttribArray(bgColorAttrib);
        glVertexAttribPointer(bgColorAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bgColor));

        GLint glyphRectAttrib = glGetAttribLocation(hnd->program, "vGlyphRect");
        glEnableVertexAttribArray(glyphRectAttrib);
        glVertexAttribPointer(glyphRectAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, glyphRect));

        GLint textureRectAttrib = glGetAttribLocation(hnd->program, "vTextureRect");
        glEnableVertexAttribArray(textureRectAttrib);
        glVertexAttribPointer(textureRectAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, textureRect));

        GLint bgRectAttrib = glGetAttribLocation(hnd->program, "vBgRect");
        glEnableVertexAttribArray(bgRectAttrib);
        glVertexAttribPointer(bgRectAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bgRect));

        GLint flagsAttrib = glGetAttribLocation(hnd->program, "vFlags");
        glEnableVertexAttribArray(flagsAttrib);
        glVertexAttribIPointer(flagsAttrib, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, flags));

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

    // We already have this glyph in our atlas..
    if (found) {
      Page* curPage = hnd->pages[glyph.page];

      // If we've gone over our maximum, flush first so we can draw some more!
      if (curPage->current >= MAX_VERTICES_PER_FLUSH)
        _flush(font, curPage, width, height);

      // Get the rect for this glyph on the texture.
      GLfloat glyphStartTexX = (GLfloat)glyph.x / (GLfloat)PAGE_WIDTH;
      GLfloat glyphStartTexY = (GLfloat)glyph.y / (GLfloat)PAGE_HEIGHT;
      GLfloat glyphEndTexX = glyphStartTexX + (GLfloat)glyph.width / (GLfloat)PAGE_WIDTH;
      GLfloat glyphEndTexY = glyphStartTexY + (GLfloat)glyph.height / (GLfloat)PAGE_HEIGHT;

      // Now get the rect to for the size of the glyph.

      // Get the maximum size for a glyph (in pixels)
      GLfloat maxWidth = ((GLfloat)(hnd->face->bbox.xMax - hnd->face->bbox.xMin) / (GLfloat)hnd->face->units_per_EM) * (GLfloat)hnd->face->size->metrics.x_ppem;
      GLfloat maxHeight = ((GLfloat)(hnd->face->bbox.yMax - hnd->face->bbox.yMin) / (GLfloat)hnd->face->units_per_EM) * (GLfloat)hnd->face->size->metrics.y_ppem;
      GLfloat ascender = ((GLfloat)hnd->face->ascender / (GLfloat)hnd->face->units_per_EM) * (GLfloat)hnd->face->size->metrics.y_ppem;

      // Where we will put our baseline.
      GLfloat baseline = ascender / maxHeight;

      // Left and top bearing
      GLfloat bearingX = ((maxWidth - (GLfloat)glyph.width) / 2.0f) / maxWidth;
      GLfloat bearingY = (GLfloat)glyph.bearingY / maxHeight;

      GLfloat glyphWidth = (GLfloat)glyph.width / maxWidth;
      GLfloat glyphHeight = (GLfloat)glyph.height / maxHeight;

      // The start pos of this glyph
      GLfloat startX = destX + ((float)x + bearingX) * cellWidth;
      GLfloat startY = destY + ((float)y + baseline - bearingY) * cellHeight;
      GLfloat endX = startX + glyphWidth * cellWidth;
      GLfloat endY = startY + glyphHeight * cellHeight;

      // First Vertex.
      Vertex vertex;
      vertex.color = colorVec;
      vertex.bgColor = bgColorVec;
      vertex.glyphRect = (Vec4d) { startX, startY,
                                   endX, endY };
      vertex.textureRect = (Vec4d) { glyphStartTexX, glyphStartTexY,
                                   glyphEndTexX, glyphEndTexY };
      vertex.bgRect = (Vec4d) { destX + cellWidth * x, destY + cellHeight * y,
                                destX + cellWidth * (x+1), destY + cellHeight * (y+1) };

      vertex.flags = 0;
      if (data[i].flashing)
        vertex.flags |= VERTEX_FLAGS_FLASHING;

      // Map the vertex buffer and add vertex data at the correct location.
      glBindBuffer(GL_ARRAY_BUFFER, curPage->vbo);
      glBufferSubData(GL_ARRAY_BUFFER, curPage->current * sizeof(Vertex), sizeof(Vertex), &vertex);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      // We've added a vertex.
      curPage->current += 1;
    }
  }

  // Do a final flush for each page.
  for (int i = 0; i < PAGE_COUNT; ++i) {
    if (hnd->pages[i]) {
      _flush(font, hnd->pages[i], width, height);
    }
  }

  return true;
}
