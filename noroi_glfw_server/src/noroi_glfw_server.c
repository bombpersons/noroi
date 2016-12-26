#include <noroi/glfw_server/noroi_glfw_server.h>

#include <noroi/base/noroi.h>
#include <noroi/base/noroi_server_base.h>
#include <noroi/glfw_server/noroi_glfw_font.h>
#include <noroi/base/noroi_event_queue.h>

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

typedef struct {
  // Keep a pointer to the base server so we can delete when we delete ourselves.
  NR_Server_Base baseServer;

  // Window.
  GLFWwindow* window;

  // Whether not to keep the window open.
  bool running;

  // Font.
  NR_Font font;
  char* fontName;
  int fontWidth, fontHeight;

  // The caption on the window.
  char* caption;

  // Front and back buffers.
  NR_Glyph *buff1, *buff2;
  NR_Glyph **frontBuff, **backBuff;

  // The current buffer for actually drawing the text.
  // This takes into account stuff like flashing characters.
  unsigned int* drawBuff;

  // Width and height of our buffers.
  int buffWidth, buffHeight;
  bool buffSizeDirty;

  // The current mouse position (in grid coordinates)
  int cursorX;
  int cursorY;
} InternalData;

// Glfw errors...
static void _glfwErrorCallback(int error, const char* desc) {
  printf("[GLFW ERROR] %i: %s\n", error, desc);
}

static NR_Key ConvGLFWKey(int key) {
  return NR_KEY_UP;
}

static NR_KeyMod ConvGLFWKeyMod(int mods) {
  return NR_KEY_MOD_SHIFT;
}

static bool ConvPixelToGird(InternalData* internal, int* outX, int* outY, int inX, int inY) {
  // If we don't have a font width or height set, then we can't figure this out.
  if (internal->fontWidth <= 0 || internal->fontHeight <= 0)
    return false;

  // Get the window size.
  int width, height;
  glfwGetWindowSize(internal->window, &width, &height);

  // Get where we actually start drawing the grid.
  int totalPixelX = internal->fontWidth * internal->buffWidth;
  int totalPixelY = internal->fontHeight * internal->buffHeight;
  int diffX = width - totalPixelX;
  int diffY = height - totalPixelY;
  int startX = diffX / 2;
  int startY = diffY / 2;

  // Convert the pixel coordinates to grid coordinates.
  *outX = (inX - startX) / internal->fontWidth;
  *outY = (inY - startY) / internal->fontHeight;

  return true;
}

// Update buffer sizes..
static void _updateBufferSizes(InternalData* internal, int width, int height) {
  int oldWidth = internal->buffWidth;
  int oldHeight = internal->buffHeight;
  internal->buffWidth = width;
  internal->buffHeight = height;
  if (oldWidth != internal->buffWidth || oldHeight != internal->buffHeight) {

    // Recreate them at the right size.
    int bufSize = internal->buffWidth * internal->buffHeight;
    NR_Glyph* buff1 = malloc(sizeof(NR_Glyph) * bufSize);
    memset(buff1, 0, sizeof(NR_Glyph) * bufSize);

    NR_Glyph* buff2 = malloc(sizeof(NR_Glyph) * bufSize);
    memset(buff2, 0, sizeof(NR_Glyph) * bufSize);

    unsigned int* drawBuff = malloc(sizeof(unsigned int) * bufSize);
    memset(drawBuff, 0, sizeof(unsigned int) * bufSize);

    // COPY OVER THE CONTENTS OF THE OLD BUFFER
    for (int x = 0; x < oldWidth && x < internal->buffWidth; ++x) {
      for (int y = 0; y < oldHeight && y < internal->buffHeight; ++y) {
        buff1[x + y * internal->buffWidth] = internal->buff1[x + y * oldWidth];
        buff2[x + y * internal->buffWidth] = internal->buff2[x + y * oldWidth];
        drawBuff[x + y * internal->buffWidth] = internal->drawBuff[x + y * oldWidth];
      }
    }

    // Delete the old buffers
    if (internal->buff1) free(internal->buff1);
    if (internal->buff2) free(internal->buff2);
    if (internal->drawBuff) free(internal->drawBuff);

    // Assign the new buffers
    internal->buff1 = buff1;
    internal->buff2 = buff2;
    internal->drawBuff = drawBuff;

    // Push an event with the new size.
    NR_Event event;
    event.type = NR_EVENT_RESIZE;
    event.data.resizeData.w = internal->buffWidth;
    event.data.resizeData.h = internal->buffHeight;
    NR_Server_Base_Event(internal->baseServer, &event);
  }
}

static void _updateDrawBuffer(InternalData* internal) {
  int buffSize = internal->buffWidth * internal->buffHeight;
  for (int i = 0; i < buffSize; ++i) {
    internal->drawBuff[i] = (*internal->frontBuff)[i].codepoint;
  }
}

static void _glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  InternalData* internal = (InternalData*)glfwGetWindowUserPointer(window);

  // Make a key event.
  NR_Event event;
  if (action == GLFW_PRESS) event.type = NR_EVENT_KEY_PRESS;
  else if (action == GLFW_RELEASE) event.type = NR_EVENT_KEY_RELEASE;

  event.data.keyData.key = ConvGLFWKey(key);
  event.data.keyData.mod = ConvGLFWKeyMod(mods);

  NR_Server_Base_Event(internal->baseServer, &event);
}

static void _glfwCursorPosCallback(GLFWwindow* window, double x, double y) {
  InternalData* internal = (InternalData*)glfwGetWindowUserPointer(window);

  NR_Event event;
  event.type = NR_EVENT_MOUSE_MOVE;

  // Convert the pixel coordinates into grid coordinates.
  int outX, outY;
  if (ConvPixelToGird(internal, &outX, &outY, (int)x, (int)y)) {
    if (outX != internal->cursorX || outY != internal->cursorY) {
      // Store the current cursor position.
      internal->cursorX = outX;
      internal->cursorY = outY;

      // Send the message.
      event.data.mouseData.x = outX;
      event.data.mouseData.y = outY;
      NR_Server_Base_Event(internal->baseServer, &event);
   }
  }
}

static NR_Button ConvGLFWButtonMod(int button) {
  switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT: return NR_BUTTON_RIGHT;
    case GLFW_MOUSE_BUTTON_MIDDLE: return NR_BUTTON_MIDDLE;
    case GLFW_MOUSE_BUTTON_LEFT: return NR_BUTTON_LEFT;
    default: return NR_BUTTON_LEFT;
  }
}

static void _glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  InternalData* internal = (InternalData*)glfwGetWindowUserPointer(window);

  NR_Event event;
  if (action == GLFW_PRESS) event.type = NR_EVENT_MOUSE_PRESS;
  else if (action == GLFW_RELEASE) event.type = NR_EVENT_MOUSE_RELEASE;
  event.data.buttonData.button = ConvGLFWButtonMod(button);
  NR_Server_Base_Event(internal->baseServer, &event);
}

static void _glfwScrollCallback(GLFWwindow* window, double x, double y) {
  InternalData* internal = (InternalData*)glfwGetWindowUserPointer(window);

  printf("Scrolled: %f\n", y);

  NR_Event event;
  event.type = NR_EVENT_MOUSE_SCROLL;
  event.data.scrollData.y = (int)y;
  NR_Server_Base_Event(internal->baseServer, &event);
}

static void _glfwShouldCloseCallback(GLFWwindow* window) {
  InternalData* internal = (InternalData*)glfwGetWindowUserPointer(window);

  // Set the flag back to false, so that if we decide later
  // in our event loop that we don't want to close, then
  // we can get this message again.
  glfwSetWindowShouldClose(window, GL_FALSE);

  // Make a close event.
  NR_Event event;
  event.type = NR_EVENT_QUIT;
  NR_Server_Base_Event(internal->baseServer, &event);
}

static void _glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
  InternalData* internal = (InternalData*)glfwGetWindowUserPointer(window);

  // Adjust our viewport
  glViewport(0, 0, width, height);

  // If we have a font loaded, then snap the size to the grid of characters.
  if (internal->fontWidth > 0 && internal->fontHeight > 0) {
    // Calculate how many characters
    width = (int)floor((float)width / (float)internal->fontWidth);
    height = (int)floor((float)height / (float)internal->fontHeight);

    // Potentially need to re-allocate buffers due to resize..
    _updateBufferSizes(internal, width, height);

    // Resize again so that we don't have any half characters visible.
    //if (internal->fontWidth > 0 && internal->fontHeight > 0 && internal->buffWidth > 0 && internal->buffHeight)
      //glfwSetWindowSize(window, width * internal->fontWidth, height * internal->fontHeight);
  }
}

// Keep track of how many servers are running so we know when to destroy our resources.
static int glfwServerCount = 0;
static bool _init() {
  // Set an error callback so we can get more error info!
  glfwSetErrorCallback(_glfwErrorCallback);

  // Intialize glfw.
  if (!glfwInit())
    return false;

  // Initialize fonts.
  NR_Font_Init();

  return true;
}
static void _uninit() {
  // De-initialize fonts.
  NR_Font_Shutdown();

  // Terminate glfw.
  glfwTerminate();
}

// Server callbacks.
static bool _initialize(void* userData) {
  InternalData* internal = (InternalData*)userData;

  // Set all the required options for GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  // Try to make a window.
  GLFWwindow* window = glfwCreateWindow(400, 400, "Title", (void*)0, (void*)0);
  if (!window) {
    return (void*)0;
  }

  // Make our opengl context current.
  glfwMakeContextCurrent(window);

  // Initialize glad.
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    return (void*)0;
  }

  // Print some opengl info
  printf("Using Opengl %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

  internal->window = window;

  // Set this handle to be associated with our window, so that we can update stuff
  // properly in callbacks.
  glfwSetWindowUserPointer(window, (void*)internal);
  glfwSetKeyCallback(window, _glfwKeyCallback);
  glfwSetCursorPosCallback(window, _glfwCursorPosCallback);
  glfwSetMouseButtonCallback(window, _glfwMouseButtonCallback);
  glfwSetScrollCallback(window, _glfwScrollCallback);
  glfwSetWindowCloseCallback(window, _glfwShouldCloseCallback);
  glfwSetWindowSizeCallback(window, _glfwWindowSizeCallback);

  // No font yet.
  internal->font = (void*)0;
  internal->fontWidth = 0;
  internal->fontHeight = 25;

  // Buffers
  internal->buff1 = (NR_Glyph*)0;
  internal->buff2 = (NR_Glyph*)0;
  internal->frontBuff = &internal->buff1;
  internal->backBuff = &internal->buff2;
  internal->drawBuff = (unsigned int*)0;

  internal->buffWidth = 0;
  internal->buffHeight = 0;

  return true;
}

static void _update(NR_Server_Base server, void* userData) {
  InternalData* internal = (InternalData*)userData;

  // Frame rate.
  //double deltaTime = glfwGetTime();
  //glfwSetTime(0.0);
  //printf("Delta: %f\n", 1.0/deltaTime);

  // Collect any events.
  glfwPollEvents();

  // Make our opengl context current.
  glfwMakeContextCurrent(internal->window);

  // Clear the background.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Get the size of the screen so we can scale things properly.
  int width, height;
  glfwGetWindowSize(internal->window, &width, &height);

  // Update our draw buffer.
  _updateDrawBuffer(internal);

  // Draw the grid of text.
  if (internal->buffWidth && internal->buffHeight && internal->font) {
    // Draw it in the center.
    int totalPixelX = internal->fontWidth * internal->buffWidth;
    int totalPixelY = internal->fontHeight * internal->buffHeight;
    int diffX = width - totalPixelX;
    int diffY = height - totalPixelY;
    int startX = diffX / 2;
    int startY = diffY / 2;

    NR_Font_Draw(internal->font, internal->drawBuff,
                 internal->buffWidth, internal->buffHeight,
                 startX, startY,
                 width, height);
  }

  // Swap gl buffers.
  glfwSwapBuffers(internal->window);
}

// Callbacks.
static bool _setSize(void* userData, unsigned int width, unsigned int height) {
  InternalData* internal = (InternalData*)userData;
  _updateBufferSizes(internal, width, height);
  return true;
}

static bool _getSize(void* userData, unsigned int* width, unsigned int* height) {
  InternalData* internal = (InternalData*)userData;
  *width = internal->buffWidth;
  *height = internal->buffHeight;
  return true;
}

static bool _setCaption(void* userData, const char* caption, unsigned int size) {
  InternalData* internal = (InternalData*)userData;
  glfwSetWindowTitle(internal->window, caption);

  // Store the name.
  if (internal->caption)
    internal->caption = realloc(internal->caption, size);
  else
    internal->caption = malloc(size);
  memcpy(internal->caption, caption, size);

  return true;
}

static bool _getCaption(void* userData, char* buf, unsigned int size, unsigned int* bytesWritten) {
  InternalData* internal = (InternalData*)userData;
  if (!internal->caption)
    return false;

  if (strlen(internal->caption) < size) {
    strcpy(buf, internal->caption);
    *bytesWritten = strlen(internal->caption);
    return true;
  }

  return false;
}

static bool _setFontSize(void* userData, unsigned int width, unsigned int height) {
  InternalData* internal = (InternalData*)userData;

  // Make sure our context is current in this thread.
  glfwMakeContextCurrent(internal->window);

  // Store the size.
  internal->fontWidth = width;
  internal->fontHeight = height;

  // Make sure we have a font set..
  if (internal->font) {
    NR_Font_SetSize(internal->font, internal->fontWidth, internal->fontHeight);
    NR_Font_GetSize(internal->font, &internal->fontWidth, &internal->fontHeight);
  }

  // Update the size of the window.
  if (internal->fontWidth > 0 && internal->fontHeight > 0 && internal->buffWidth > 0 && internal->buffHeight > 0)
    glfwSetWindowSize(internal->window, internal->fontWidth * internal->buffWidth, internal->fontHeight * internal->buffHeight);

  return true;
}

static bool _getFontSize(void* userData, unsigned int* width, unsigned int* height) {
  InternalData* internal = (InternalData*)userData;

  *width = internal->fontWidth;
  *height = internal->fontHeight;

  return true;
}

static bool _setFont(void* userData, const char* font) {
  InternalData* internal = (InternalData*)userData;

  // Make sure our opengl context is current.
  glfwMakeContextCurrent(internal->window);

  // Delete the current font if we have one.
  if (internal->font)
    NR_Font_Delete(internal->font);
  internal->font = NR_Font_Load(font);

  // Store the name.
  if (internal->fontName)
    internal->fontName = realloc(internal->fontName, strlen(font));
  else
    internal->fontName = malloc(strlen(font));
  memcpy(internal->fontName, font, strlen(font));

  // Make sure the size is right.
  _setFontSize(userData, internal->fontWidth, internal->fontHeight);

  return internal->font;
}

static bool _getFont(void* userData, char* fontName, unsigned int size) {
  InternalData* internal = (InternalData*)userData;
  if (!internal->fontName)
    return false;

  if (strlen(internal->fontName) < size) {
    strcpy(fontName, internal->fontName);
    return true;
  }

  return false;
}

static bool _setGlyph(void* userData, unsigned int x, unsigned int y, const NR_Glyph* glyph) {
  InternalData* internal = (InternalData*)userData;

  if (x >= internal->buffWidth)
    return false;
  if (y >= internal->buffHeight)
    return false;

  (*internal->backBuff)[x + y * internal->buffWidth] = *glyph;
  return true;
}

static bool _getGlyph(void* userData, unsigned int x, unsigned int y, NR_Glyph* glyph) {
  InternalData* internal = (InternalData*)userData;

  if (x >= internal->buffWidth)
    return false;
  if (y >= internal->buffHeight)
    return false;

  *glyph = (*internal->backBuff)[x + y * internal->buffWidth];
  return true;
}

static bool _text(void* userData, unsigned int x, unsigned int y, const char* text) {
  // Decode the text from utf-8.
  uint32_t codepoint;
  uint32_t state = 0;
  uint32_t count = 0;

  int stringSize = strlen(text);
  for (int i = 0; i < stringSize; ++i, ++count)
    if (!decode(&state, &codepoint, text[i])) {
      NR_Glyph glyph;
      glyph.codepoint = codepoint;
      _setGlyph(userData, x + count, y, &glyph);
    }

  if (state != UTF8_ACCEPT)
    return false;
  return true;
}

static bool _rectangle(void* userdata, unsigned int x, unsigned int y, unsigned int w, unsigned int h, bool fill, const NR_Glyph* glyph) {
  if (fill) {
    // Filled rectangle.
    for (int i = x; i < x+w; i++) {
      for (int j = y; j < y+h; j++) {
        _setGlyph(userdata, i, j, glyph);
      }
    }
  } else {
    // Hollow rectangle.
    for (int i = x; i <= x+w; i++) {
      _setGlyph(userdata, i, y, glyph);
      _setGlyph(userdata, i, y+h, glyph);
    }
    for (int i = y; i <= y+h; i++) {
      _setGlyph(userdata, x, i, glyph);
      _setGlyph(userdata, x+w, i, glyph);
    }
  }

  return true;
}

static bool _swapBuffers(void* userData) {
  InternalData* internal = (InternalData*)userData;

  // Swap the buffers by copying the back buffer into the front buffer.
  memcpy(*internal->frontBuff, *internal->backBuff, sizeof(NR_Glyph) * internal->buffWidth * internal->buffHeight);

  // if (internal->frontBuff == &internal->buff1) {
  //   internal->frontBuff = &internal->buff2;
  //   internal->backBuff = &internal->buff1;
  // } else {
  //   internal->frontBuff = &internal->buff1;
  //   internal->backBuff = &internal->buff2;
  // }

  return true;
}

static bool _clear(void* userData, const NR_Glyph* glyph) {
  InternalData* internal = (InternalData*)userData;

  unsigned int buffSize = internal->buffWidth * internal->buffHeight;
  for (unsigned int i = 0; i < buffSize; ++i) {
    (*internal->backBuff)[i] = *glyph;
  }

  return true;
}

// Create / Destroy server instances.
NR_GLFW_Server NR_GLFW_Server_New(NR_Context* context, const char* replyAddress, const char* publisherAddress) {
  InternalData* internal = malloc(sizeof(InternalData));

  // Increase the server reference counter, initialize if it was 0
  if (glfwServerCount == 0) {
    if (!_init()) {
      printf("[GLFW ERROR] Initialization for glfw server did not succeed.\n");
      return (void*)0;
    }
  }
  glfwServerCount++;

  // Initialization of internal variables.
  internal->caption = (void*)0;
  internal->fontName = (void*)0;

  // Create a server.
  NR_Server_Base_Callbacks callbacks;
  callbacks.initialize = _initialize;
  callbacks.update = _update;
  callbacks.handleRequest = (void*)0;

  callbacks.setSize = _setSize;
  callbacks.getSize = _getSize;

  callbacks.setCaption = _setCaption;
  callbacks.getCaption = _getCaption;

  callbacks.setFont = _setFont;
  callbacks.getFont = _getFont;

  callbacks.setFontSize = _setFontSize;
  callbacks.getFontSize = _getFontSize;

  callbacks.setGlyph = _setGlyph;
  callbacks.getGlyph = _getGlyph;

  callbacks.text = _text;
  callbacks.rectangle = _rectangle;

  callbacks.clear = _clear;
  callbacks.swapBuffers = _swapBuffers;

  internal->baseServer = NR_Server_Base_New(context, replyAddress, publisherAddress,
                                            (void*)internal,
                                            callbacks);

  // Return our handle.
  return (void*)internal;
}

void NR_GLFW_Server_Delete(NR_GLFW_Server server) {
  InternalData* internal = (InternalData*)server;

  // Close server.
  NR_Server_Base_Delete(internal->baseServer);

  // Delete the font we potentially have allocated.
  if (internal->font)
    NR_Font_Delete(internal->font);

  // Delete the font name
  if (internal->fontName)
    free(internal->fontName);
  if (internal->caption)
    free(internal->caption);

  // De-allocate front and back buffers.
  free(internal->buff1);
  free(internal->buff2);
  free(internal->drawBuff);

  // Free handle
  free(internal);

  // Decrease the server reference count.
  glfwServerCount--;
  if (glfwServerCount == 0) _uninit();
}
