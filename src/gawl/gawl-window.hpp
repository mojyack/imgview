#pragma once
#include <cstdint>

#include "type.hpp"

namespace gawl {
struct BufferSize {
    size_t size[2] = {800, 600};
    size_t scale   = 1.0;
};

class GawlApplication;

class GawlWindow {
    friend class GawlApplication;

  private:
    enum class Status {
        PREPARE,
        READY,
        CLOSE,
    };
    Status             status              = Status::PREPARE;
    bool               follow_buffer_scale = true;
    double             specified_scale     = 0;
    double             draw_scale          = 1.0;
    bool               event_driven        = false;
    BufferSize         buffer_size;
    std::array<int, 2> window_size;

  protected:
    GawlApplication& app;

    // used by backend.
    void on_buffer_resize(const size_t width, const size_t height, const size_t scale);
    bool is_running() const noexcept;
    void init_global();
    void init_complete();

    // used by user.
    const std::array<int, 2>& get_window_size() const noexcept;
    void                      set_follow_buffer_scale(const bool flag);
    bool                      get_follow_buffer_scale() const noexcept;
    void                      set_scale(const double scale);
    void                      set_event_driven(const bool flag);
    bool                      get_event_driven() const noexcept;
    virtual void              refresh_callback(){};
    virtual void              window_resize_callback() {}
    virtual void              keyboard_callback(const uint32_t /* key */, const gawl::ButtonState /* state */) {}
    virtual void              pointermove_callback(const double /* x */, const double /* y */) {}
    virtual void              click_callback(const uint32_t /* button */, const gawl::ButtonState /* state */) {}
    virtual void              scroll_callback(const gawl::WheelAxis /* axis */, const double /* value */) {}
    virtual void              close_request_callback();

  public:
    virtual void      refresh() = 0;
    bool              is_close_pending() const noexcept;
    double            get_scale() const noexcept;
    const BufferSize& get_buffer_size() const noexcept;
    void              close_window();
    void              quit_application();

    GawlWindow(const GawlWindow&)     = delete;
    GawlWindow(GawlWindow&&) noexcept = delete;
    GawlWindow& operator=(const GawlWindow&) = delete;
    GawlWindow& operator=(GawlWindow&&) noexcept = delete;

    GawlWindow(GawlApplication& app);
    virtual ~GawlWindow();
};
} // namespace gawl
