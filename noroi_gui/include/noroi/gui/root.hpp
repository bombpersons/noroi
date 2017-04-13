#ifndef NOROI_GUI_ROOT
#define NOROI_GUI_ROOT

#include <stack>

#include <noroi/gui/rect.hpp>

namespace noroi {
  namespace gui {
    class Root {
    public:
      // Specify the client this root frame should draw to.
      Root(NR_Client client);
      virtual ~Root();

      // Handle an event from the client.
      virtual void handleEvent(NR_Event event);

      // Draw all frames.
      virtual void draw();

      // Push focus onto the focus stack.
      virtual void pushFocus(Frame* frame);

      // Pop focus.
      virtual void popFocus();

      // Dirty a rectangle and mark it for being re-drawn.
      // Find all frames that are contained by the rect and dirty them.
      virtual void dirtyRect(const Rect& rect);

    private:
      // A stack of frames that have focus.
      std::stack<Frame*> m_focusStack;

    };
  }
}

#endif
