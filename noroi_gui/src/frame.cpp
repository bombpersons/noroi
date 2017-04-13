#include <noroi/gui/frame.hpp>

namespace noroi {
  namespace gui {

    Frame::Frame(Frame* frame) {
      m_parent = frame;
    }

    Frame::~Frame() {
    }

    // Just return the first parent frame that has
    Frame* Frame::getRoot() {
      if (m_parent == nullptr) return this;
      else return m_parent->getRoot();
    }

    void draw(NR_Client client) {
      // Draw children.
      for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        (*it)->draw();
      }

      // Unless we were marked as dirty, stop here.
      if (!m_dirty)
        return;

      // Just draw a rectangle.
      NR_Client_Rectangle(client, m_rect.x, m_rect.y, m_rect.w, m_rect.h);
    }

    // Handle an event from the client.
    void handleEvent(NR_Event event) {
      switch (event.type) {
        case NR_EVENT_MOUSE_MOVE:
          break;

        case NR_EVENT_MOUSE_SCROLL:
          break;

        case NR_EVENT_MOUSE_PRESS:
          break;

        case NR_EVENT_MOUSE_RELEASE:
          break;

        case NR_EVENT_CHARACTER:
          break;
      }
    }

    // Mouse clicks.
    void mouseButton(NR_Button button, bool pressed);

    // Mouse move
    void mouseMove(int x, int y);

    // Mouse enter / leave.
    void mouseEnter();
    void mouseLeave();

    // Keyboard focus.
    void keyboardFocus(bool gained);

    // Keyboard key
    void keyboardChar(char character);

    // Test whether or not we contain a point.
    void containTest(int x, int y);

    // Set the position of the frame (ignored if using a layout).
    void setPosition(int x, int y) {
      m_rect.x = x;
      m_rect.y = y;
    }

    // Set the size of the frame.
    void setSize(int w, int h) {
      m_rect.w = w;
      m_rect.h = h;
    }

    // Resize the frame (includes resizing any child frames)
    void resize() {
      // Since we're just a basic frame, let the children do their own thing.
      for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        (*it)->resize();
      }
    }

  }
}
