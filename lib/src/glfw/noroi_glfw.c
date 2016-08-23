#include <noroi/noroi.h>

#ifdef NOROI_USE_GLFW

#include <noroi/glfw/noroi_glfw_font.h>
#include <noroi/misc/noroi_event_queue.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// threads
#include <tinycthread.h>

typedef struct {
  // Window.
  GLFWwindow* window;
  NR_EventQueue* events;

  // A thread to run polling and what not.
  thrd_t threadId;
  mtx_t mutexId;
  bool running;

  // Font.
  NR_Font font;
  int fontWidth, fontHeight;

  // Front and back buffers.
  NR_Glyph *buff1, *buff2;
  NR_Glyph **frontBuff, **backBuff;

  // The current buffer for actually drawing the text.
  // This takes into account stuff like flashing characters.
  unsigned int* drawBuff;

  // Width and height of our buffers.
  int buffWidth, buffHeight;
  bool buffSizeDirty;
} HandleType;

// Glfw errors...
static void _glfwErrorCallback(int error, const char* desc) {
  printf("glfw error %i: %s\n", error, desc);
}

static NR_Key ConvGLFWKey(int key) {
  return NR_KEY_UP;
}

static NR_KeyMod ConvGLFWKeyMod(int mods) {
  return NR_KEY_MOD_SHIFT;
}

// Update buffer sizes..
static void _updateBufferSizes(HandleType* hnd, int width, int height) {
  int oldWidth = hnd->buffWidth;
  int oldHeight = hnd->buffHeight;
  hnd->buffWidth = width;
  hnd->buffHeight = height;
  if (oldWidth != hnd->buffWidth || oldHeight != hnd->buffHeight) {

    // Recreate them at the right size.
    int bufSize = hnd->buffWidth * hnd->buffHeight;
    NR_Glyph* buff1 = malloc(sizeof(NR_Glyph) * bufSize);
    memset(buff1, 0, sizeof(NR_Glyph) * bufSize);

    NR_Glyph* buff2 = malloc(sizeof(NR_Glyph) * bufSize);
    memset(buff2, 0, sizeof(NR_Glyph) * bufSize);

    unsigned int* drawBuff = malloc(sizeof(unsigned int) * bufSize);
    memset(drawBuff, 0, sizeof(unsigned int) * bufSize);

    // COPY OVER THE CONTENTS OF THE OLD BUFFER
    for (int x = 0; x < oldWidth && x < hnd->buffWidth; ++x) {
      for (int y = 0; y < oldHeight && y < hnd->buffHeight; ++y) {
        buff1[x + y * hnd->buffWidth] = hnd->buff1[x + y * oldWidth];
        buff2[x + y * hnd->buffWidth] = hnd->buff2[x + y * oldWidth];
        drawBuff[x + y * hnd->buffWidth] = hnd->drawBuff[x + y * oldWidth];
      }
    }

    // Delete the old buffers
    if (hnd->buff1) free(hnd->buff1);
    if (hnd->buff2) free(hnd->buff2);
    if (hnd->drawBuff) free(hnd->drawBuff);

    // Assign the new buffers
    hnd->buff1 = buff1;
    hnd->buff2 = buff2;
    hnd->drawBuff = drawBuff;

    printf("Resizing buffers: %i, %i\n", hnd->buffWidth, hnd->buffHeight);
  }
}

static void _updateDrawBuffer(HandleType* hnd) {
  int buffSize = hnd->buffWidth * hnd->buffHeight;
  for (int i = 0; i < buffSize; ++i) {
    hnd->drawBuff[i] = (*hnd->frontBuff)[i].codepoint;
  }
}

static void _glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  HandleType* hnd = (HandleType*)glfwGetWindowUserPointer(window);

  // Make a key event.
  NR_Event event;
  if (action == GLFW_PRESS) event.type = NR_EVENT_KEY_PRESS;
  else if (action == GLFW_RELEASE) event.type = NR_EVENT_KEY_RELEASE;

  event.data.keyData.key = ConvGLFWKey(key);
  event.data.keyData.mod = ConvGLFWKeyMod(mods);

  NR_EventQueue_Push(hnd->events, &event);
}

static void _glfwCursorPosCallback(GLFWwindow* window, double x, double y) {
  HandleType* hnd = (HandleType*)glfwGetWindowUserPointer(window);

  NR_Event event;
  event.type = NR_EVENT_MOUSE_MOVE;
  event.data.mouseData.x = (int)x;
  event.data.mouseData.y = (int)y;
  NR_EventQueue_Push(hnd->events, &event);
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
  HandleType* hnd = (HandleType*)glfwGetWindowUserPointer(window);

  NR_Event event;
  if (action == GLFW_PRESS) event.type = NR_EVENT_MOUSE_PRESS;
  else if (action == GLFW_RELEASE) event.type = NR_EVENT_MOUSE_RELEASE;
  event.data.buttonData.button = ConvGLFWButtonMod(button);
  NR_EventQueue_Push(hnd->events, &event);
}

static void _glfwScrollCallback(GLFWwindow* window, double x, double y) {
  HandleType* hnd = (HandleType*)glfwGetWindowUserPointer(window);

  printf("Scrolled: %f\n", y);

  NR_Event event;
  event.type = NR_EVENT_MOUSE_SCROLL;
  event.data.scrollData.y = (int)y;
  NR_EventQueue_Push(hnd->events, &event);
}

static void _glfwShouldCloseCallback(GLFWwindow* window) {
  HandleType* hnd = (HandleType*)glfwGetWindowUserPointer(window);

  // Set the flag back to false, so that if we decide later
  // in our event loop that we don't want to close, then
  // we can get this message again.
  glfwSetWindowShouldClose(window, GL_FALSE);

  // Make a close event.
  NR_Event event;
  event.type = NR_EVENT_QUIT;
  NR_EventQueue_Push(hnd->events, &event);
}

static void _glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
  HandleType* hnd = (HandleType*)glfwGetWindowUserPointer(window);

  // Adjust our viewport
  glViewport(0, 0, width, height);

  // Resize again so that we don't have any half characters visible.
  int rWidth = (int)ceil((float)width / (float)hnd->fontWidth);
  int rHeight = (int)ceil((float)height / (float)hnd->fontHeight);
  glfwSetWindowSize(window, rWidth * hnd->fontWidth, rHeight * hnd->fontHeight);

  // Potentially need to re-allocate buffers due to resize..
  _updateBufferSizes(hnd, rWidth, rHeight);

  // Push an event with the new size.
  NR_Event event;
  event.type = NR_EVENT_RESIZE;
  event.data.resizeData.w = rWidth;
  event.data.resizeData.h = rHeight;
  NR_EventQueue_Push(hnd->events, &event);
}

static int _threadPoll(void* data) {
  HandleType* hnd = (HandleType*)data;

  hnd->running = true;
  while (hnd->running) {
    // We don't want this running at the same time as loading a new font, etc.
    mtx_lock(&hnd->mutexId);

    // Make our opengl context current.
    glfwMakeContextCurrent(hnd->window);

    // Clear the background.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Get the size of the screen so we can scale things properly.
    int width, height;
    glfwGetWindowSize(hnd->window, &width, &height);

    // Update our draw buffer.
    _updateDrawBuffer(hnd);

    // Draw the grid of text.
    if (hnd->font) {
      // Draw it.
      NR_Font_Draw(hnd->font, hnd->drawBuff, hnd->buffWidth, hnd->buffHeight, width, height);
    }

    // Swap gl buffers.
    glfwSwapBuffers(hnd->window);

    // Unlock
    mtx_unlock(&hnd->mutexId);
  }

  return 1;
}

// Initialize noroi
bool NR_Init() {
  // Set an error callback so we can get more error info!
  glfwSetErrorCallback(_glfwErrorCallback);

  // Intialize glfw.
  if (!glfwInit())
    return false;

  // Initialize fonts.
  NR_Font_Init();

  return true;
}
void NR_Shutdown() {
  // De-initialize fonts.
  NR_Font_Shutdown();

  // Terminate glfw.
  glfwTerminate();
}

// Create / destroy a handle.
NR_Handle NR_CreateHandle() {
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

  // Allocate some memory for our handle.
  HandleType* handle = malloc(sizeof(HandleType));
  memset(handle, 0, sizeof(HandleType));
  handle->window = window;
  handle->events = NR_EventQueue_New();

  // Set this handle to be associated with our window, so that we can update stuff
  // properly in callbacks.
  glfwSetWindowUserPointer(window, (void*)handle);

  // Set key callback
  glfwSetKeyCallback(window, _glfwKeyCallback);

  // Set mouse move callback
  glfwSetCursorPosCallback(window, _glfwCursorPosCallback);

  // Set mouse button callback
  glfwSetMouseButtonCallback(window, _glfwMouseButtonCallback);

  // Mouse scroll
  glfwSetScrollCallback(window, _glfwScrollCallback);

  // Close callback
  glfwSetWindowCloseCallback(window, _glfwShouldCloseCallback);

  // Resize callback
  glfwSetWindowSizeCallback(window, _glfwWindowSizeCallback);

  // No font yet.
  handle->font = (void*)0;
  handle->fontWidth = 0;
  handle->fontHeight = 25;

  // Buffers
  handle->buff1 = (NR_Glyph*)0;
  handle->buff2 = (NR_Glyph*)0;
  handle->frontBuff = &handle->buff1;
  handle->backBuff = &handle->buff2;
  handle->drawBuff = (unsigned int*)0;

  handle->buffWidth = 0;
  handle->buffHeight = 0;

  // Create a mutex so we can prevent the two threads from interfering.
  mtx_init(&handle->mutexId, mtx_plain | mtx_recursive);

  // Create a thread to poll the window.
  thrd_create(&handle->threadId, _threadPoll, (void*)handle);

  // Return the handle.
  return (void*)handle;
}
void NR_DestroyHandle(NR_Handle hnd) {
  // Delete the event queue we allocated.
  HandleType* h = (HandleType*)hnd;

  // Stop the thread and wait for it to finish.
  h->running = false;
  thrd_join(h->threadId, NULL);

  // Delete the event queue.
  NR_EventQueue_Delete(h->events);

  // Delete the font we potentially have allocated.
  if (h->font)
    NR_Font_Delete(h->font);

  // De-allocate front and back buffers.
  free(h->buff1);
  free(h->buff2);
  free(h->drawBuff);

  // Free the memory we allocated for our handle.
  free((void*)hnd);
}

// Set the font.
bool NR_SetFont(NR_Handle hnd, const char* font) {
  HandleType* h = (HandleType*)hnd;

  // Lock
  mtx_lock(&h->mutexId);

  // Make sure our opengl context is current.
  glfwMakeContextCurrent(h->window);

  // Delete the current font if we have one.
  if (h->font)
    NR_Font_Delete(h->font);
  h->font = NR_Font_Load(font);

  // Make sure the size is right.
  NR_SetFontSize(hnd, h->fontWidth, h->fontHeight);

  // Unlock
  mtx_unlock(&h->mutexId);

  return true;
}

void NR_SetFontSize(NR_Handle hnd, int width, int height) {
  HandleType* h = (HandleType*)hnd;

  mtx_lock(&h->mutexId);

  // Make sure our context is current in this thread.
  glfwMakeContextCurrent(h->window);

  // Store the size.
  h->fontWidth = width;
  h->fontHeight = height;

  // Make sure we have a font set..
  if (h->font) {
    NR_Font_SetSize(h->font, h->fontWidth, h->fontHeight);
    NR_Font_GetSize(h->font, &h->fontWidth, &h->fontHeight);
  }

  mtx_unlock(&h->mutexId);
}

// Events
bool NR_PollEvent(NR_Handle hnd, NR_Event* event) {
  // Make glfw update for any events.
  glfwPollEvents();

  // Pop of an event if we have any.
  HandleType* h = (HandleType*)hnd;
  return NR_EventQueue_Pop(h->events, event);
}

// Set / get the size of the window.
void NR_SetSize(NR_Handle hnd, int width, int height) {
  HandleType* h = (HandleType*)hnd;

  // Get the required size of the window.
  int reqWidth = h->fontWidth * width;
  int reqHeight = h->fontHeight * height;

  // Resize the window.
  glfwSetWindowSize(h->window, reqWidth, reqHeight);
}
void NR_GetSize(NR_Handle hnd, int* width, int* height) {
  HandleType* h = (HandleType*)hnd;
  glfwGetWindowSize(h->window, width, height);

  *width /= h->fontWidth;
  *height /= h->fontHeight;
}

// Set / get the caption of the window.
void NR_SetCaption(NR_Handle hnd, const char* caption) {}
int NR_GetCaptionSize(NR_Handle hnd) { return 0; }
void NR_GetCaption(NR_Handle hnd, char* buf) {}

// Set/get a glyph
void NR_SetGlyph(NR_Handle hnd, int x, int y, const NR_Glyph* glyph) {
  HandleType* h = (HandleType*)hnd;
  if (x < 0 || x >= h->buffWidth)
    return;
  if (y < 0 || y >= h->buffHeight)
    return;

  (*h->backBuff)[x + y * h->buffWidth] = *glyph;
}
NR_Glyph NR_GetGlyph(NR_Handle hnd, int x, int y) {
  HandleType* h = (HandleType*)hnd;
  assert(x >= 0 && x < h->buffWidth);
  assert(y >= 0 && y < h->buffHeight);

  return (*h->backBuff)[x + y * h->buffWidth];
}

// Apply any changes.
void NR_SwapBuffers(NR_Handle hnd) {
  HandleType* h = (HandleType*)hnd;

  // Swap our glyph buffers.
  if (h->frontBuff == &h->buff1) {
    h->frontBuff = &h->buff2;
    h->backBuff = &h->buff1;
  } else {
    h->frontBuff = &h->buff1;
    h->backBuff = &h->buff2;
  }
}

// void NR_Render(NR_Handle hnd) {
//   HandleType* h = (HandleType*)hnd;
//
//   // Clear the background.
//   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//   glClear(GL_COLOR_BUFFER_BIT);
//
//   // Get the size of the screen so we can scale things properly.
//   int width, height;
//   glfwGetWindowSize(h->window, &width, &height);
//
//   // Update our draw buffer.
//   _updateDrawBuffer(h);
//
//   // Draw the grid of text.
//   if (h->font) {
//     // Draw it.
//     NR_Font_Draw(h->font, h->drawBuff, h->buffWidth, h->buffHeight, width, height);
//   }
//
//   // Swap gl buffers.
//   glfwSwapBuffers(h->window);
// }

#endif
