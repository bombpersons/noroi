#include <noroi/server/noroi_glfw_server.h>

#ifdef NOROI_USE_GLFW

#include <noroi/noroi.h>
#include <noroi/server/noroi_server_base.h>
#include <noroi/glfw/noroi_glfw_font.h>
#include <noroi/misc/noroi_event_queue.h>

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

typedef struct {
  // Keep a pointer to the base server so we can delete when we delete ourselves.
  NR_Server_Base baseServer;

  // Window.
  GLFWwindow* window;

  // Whether not to keep the window open.
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

    printf("Resizing buffers: %i, %i\n", internal->buffWidth, internal->buffHeight);

    // Resize the window to fit.
    //if (internal->fontWidth > 0 && internal->fontHeight > 0 && internal->buffWidth > 0 && internal->buffHeight)
      //glfwSetWindowSize(internal->window, internal->fontWidth * internal->buffWidth, internal->fontHeight * internal->buffHeight);
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
  event.data.mouseData.x = (int)x;
  event.data.mouseData.y = (int)y;
  NR_Server_Base_Event(internal->baseServer, &event);
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

  // Push an event with the new size.
  NR_Event event;
  event.type = NR_EVENT_RESIZE;
  event.data.resizeData.w = width;
  event.data.resizeData.h = height;
  NR_Server_Base_Event(internal->baseServer, &event);
}

void _setFontSize(InternalData* internal, int width, int height) {
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
}

// Set the font.
bool _setFont(InternalData* internal, const char* font) {
  // Make sure our opengl context is current.
  glfwMakeContextCurrent(internal->window);

  // Delete the current font if we have one.
  if (internal->font)
    NR_Font_Delete(internal->font);
  internal->font = NR_Font_Load(font);

  // Make sure the size is right.
  _setFontSize(internal, internal->fontWidth, internal->fontHeight);

  return internal->font;
}

// Set/get a glyph
void _setGlyph(InternalData* internal, int x, int y, const NR_Glyph* glyph) {
  if (x < 0 || x >= internal->buffWidth)
    return;
  if (y < 0 || y >= internal->buffHeight)
    return;

  (*internal->backBuff)[x + y * internal->buffWidth] = *glyph;
}
NR_Glyph _getGlyph(InternalData* internal, int x, int y) {
  assert(x >= 0 && x < internal->buffWidth);
  assert(y >= 0 && y < internal->buffHeight);

  return (*internal->backBuff)[x + y * internal->buffWidth];
}

// Apply any changes.
void _swapBuffers(InternalData* internal) {
  // Swap our glyph buffers.
  if (internal->frontBuff == &internal->buff1) {
    internal->frontBuff = &internal->buff2;
    internal->backBuff = &internal->buff1;
  } else {
    internal->frontBuff = &internal->buff1;
    internal->backBuff = &internal->buff2;
  }
}

// Keep track of how many servers are running so we know when to destroy our resources.
static int glfwServerCount = 0;
static bool _initialize() {
  // Set an error callback so we can get more error info!
  glfwSetErrorCallback(_glfwErrorCallback);

  // Intialize glfw.
  if (!glfwInit())
    return false;

  // Initialize fonts.
  NR_Font_Init();

  return true;
}
static void _uninitialize() {
  // De-initialize fonts.
  NR_Font_Shutdown();

  // Terminate glfw.
  glfwTerminate();
}

// Server callbacks.
static bool _initializer(void* userData) {
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

  // Some default starting values.
  _setFontSize(internal, 0, 25);
  _updateBufferSizes(internal, 10, 10);
  NR_Glyph glyph;
  glyph.codepoint = '5';
  _setGlyph(internal, 5, 5, &glyph);
  _swapBuffers(internal);

  return true;
}

static void _updater(NR_Server_Base server, void* userData) {
  InternalData* internal = (InternalData*)userData;

  // Frame rate.
  double deltaTime = glfwGetTime();
  glfwSetTime(0.0);
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
    // Draw it.
    NR_Font_Draw(internal->font, internal->drawBuff, internal->buffWidth, internal->buffHeight, width, height);
  }

  // Swap gl buffers.
  glfwSwapBuffers(internal->window);
}

static void _requestHandler(NR_Server_Base server, void* userData, void* data, unsigned int size) {
  InternalData* internal = (InternalData*)userData;
  NR_Request_Header* requestHeader = (NR_Request_Header*)data;

  // Figure out what the request was.
  switch (requestHeader->type) {
    case NR_Request_Type_SetFont:
      {
        NR_Response_Header header;
        header.type = _setFont(internal, requestHeader->contents) ? NR_Response_Type_Success : NR_Response_Type_Failure;
        header.size = 0;
        NR_Server_Base_Reply(server, &header, sizeof(header));
        break;
      }
    case NR_Request_Type_SetFontSize:
      {
        NR_Request_SetFontSize_Contents* contents = (NR_Request_SetFontSize_Contents*)requestHeader->contents;
        _setFontSize(internal, contents->width, contents->height);

        NR_Response_Header header;
        header.type = NR_Response_Type_Success;
        header.size = 0;
        NR_Server_Base_Reply(server, &header, sizeof(header));
        break;
      }
    default:
      {
        // Construct the message.
        char errorBuff[1024];
        sprintf(errorBuff, "Server has no handler for request type %i\n", requestHeader->type);
        unsigned int contentSize = sizeof(char) * strlen(errorBuff);
        unsigned int totalSize = sizeof(NR_Response_Header) + contentSize;

        // Allocate some memory to hold this.
        NR_Response_Header* header = malloc(totalSize);
        header->type = NR_Response_Type_Failure;
        header->size = contentSize;
        char* contents = (char*)header->contents;

        // Copy the error msg in.
        strcpy(contents, errorBuff);

        // Send the header and packet.
        NR_Server_Base_Reply(server, header, totalSize);

        // Delete it afterwards.
        free(header);
        break;
      }
  }
}

// Create / Destroy server instances.
NR_GLFW_Server NR_GLFW_Server_New(const char* replyAddress, const char* publisherAddress) {
  InternalData* internal = malloc(sizeof(InternalData));

  // Increase the server reference counter, initialize if it was 0
  if (glfwServerCount == 0) {
    if (!_initialize()) {
      printf("[GLFW ERROR] Initialization for glfw server did not succeed.\n");
      return (void*)0;
    }
  }
  glfwServerCount++;

  // Create a server.
  internal->baseServer = NR_Server_Base_New(replyAddress, publisherAddress,
                                            (void*)internal,
                                            _initializer,
                                            _updater,
                                            _requestHandler);

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

  // De-allocate front and back buffers.
  free(internal->buff1);
  free(internal->buff2);
  free(internal->drawBuff);

  // Free handle
  free(internal);

  // Decrease the server reference count.
  glfwServerCount--;
  if (glfwServerCount == 0) _uninitialize();
}

#endif
