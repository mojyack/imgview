#pragma once
#include <vector>

namespace gawl {
class GawlWindow;

class GawlApplication {
  private:
    std::vector<GawlWindow*> windows;

    void register_window(GawlWindow* window);

  protected:
    const std::vector<GawlWindow*>& get_windows() const noexcept;
    void                            unregister_window(GawlWindow* window);
    void                            close_all_windows();

  public:
    template <typename T, typename... A>
    void open_window(A... args) {
        register_window(new T(*this, args...));
    }
    void         close_window(GawlWindow* window);
    virtual void tell_event(GawlWindow* window) = 0;
    virtual void run()                          = 0;
    virtual void quit()                         = 0;
    virtual bool is_running() const             = 0;
    virtual ~GawlApplication() {}
};
} // namespace gawl
