#ifndef NOROI_GUI_FRAME
#define NOROI_GUI_FRAME

#include <noroi/client/noroi_client.hpp>
#include <vector>

namespace noroi {
  namespace gui {
    class Frame {
    public:
      Frame(Frame* parent);
      virtual ~Frame();

      // Get the root frame.
      Frame* getRoot();

      // Draw the frame to a client.
      virtual void draw(NR_Client client);

      // Mouse clicks.
      virtual void mouseButton(NR_Button button, bool pressed);

      // Mouse move
      virtual void mouseMove(int x, int y);

      // Mouse enter / leave.
      virtual void mouseEnter();
      virtual void mouseLeave();

      // Keyboard focus.
      virtual void keyboardFocus(bool gained);

      // Keyboard key
      virtual void keyboardChar(char character);

      // Test whether or not we contain a point.
      virtual void containTest(int x, int y);

      // Set the position of the frame (ignored if using a layout).
      virtual void setPosition(int x, int y);

      // Set the size of the frame.
      virtual void setSize(int w, int h);

      // Resize the frame (includes resizing any child frames)
      virtual void resize();

    private:
      // The parent frame.
      Frame* m_parent;

      // Child frames.
      std::vector<Frame*> m_children;

      // The size of the frame.
      Rect m_rect;
    };
  }
}

#endif
