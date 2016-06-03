#include <noroi/noroi.h>

#ifdef NOROI_USE_GLFW

#include <noroi/misc/noroi_event_queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

typedef struct {
  GLFWwindow* window;
  NR_EventQueue* events;
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

  NR_Event event;
  event.type = NR_EVENT_RESIZE;
  event.data.resizeData.w = width;
  event.data.resizeData.h = height;
  NR_EventQueue_Push(hnd->events, &event);
}

// Initialize noroi
bool NR_Init() {
  // Set an error callback so we can get more error info!
  glfwSetErrorCallback(_glfwErrorCallback);

  // Intialize glfw.
  if (!glfwInit())
    return false;
  return true;
}
void NR_Shutdown() {
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
  GLFWwindow* window = glfwCreateWindow(640, 480, "Title", (void*)0, (void*)0);
  if (!window) {
    return (void*)0;
  }

  // Make our opengl context current.
  glfwMakeContextCurrent(window);

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

  return (void*)handle;
}
void NR_DestroyHandle(NR_Handle hnd) {
  // Delete the event queue we allocated.
  HandleType* h = (HandleType*)hnd;
  NR_EventQueue_Delete(h->events);

  // Free the memory we allocated for our handle.
  free((void*)hnd);
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
void NR_SetSize(NR_Handle hnd, int width, int height) {}
void NR_GetSize(NR_Handle hnd, int* width, int* height) {}

// Set / get the caption of the window.
void NR_SetCaption(NR_Handle hnd, const char* caption) {}
int NR_GetCaptionSize(NR_Handle hnd) { return 0; }
void NR_GetCaption(NR_Handle hnd, char* buf) {}

// Set/get a glyph
void NR_SetGlyph(NR_Handle hnd, int x, int y, const NR_Glyph* glyph) {}
NR_Glyph NR_GetGlyph(NR_Handle hnd, int x, int y) { NR_Glyph glyph = {}; return glyph; }

// Draw a square
void NR_RectangleFill(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph) {}
void NR_Rectangle(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph) {}

// Draw text
void NR_Text(NR_Handle hnd, int x, int y, const char* text) {}

// Clear everything.
void NR_Clear(NR_Handle hnd, const NR_Glyph* glyph) {}

// Apply any changes.
void NR_SwapBuffers(NR_Handle hnd) {

}

#endif
