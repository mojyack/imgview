#pragma once
#include <gawl/wayland/gawl.hpp>

namespace graphic::message {
class MessageGraphic {
  private:
    static inline gawl::Graphic graphic;

    std::string message;

  public:
    auto get_graphic() -> gawl::Graphic {
        return graphic;
    }

    auto get_message() const -> const std::string& {
        return message;
    }

    MessageGraphic(const std::string_view message) : message(message) {}
};
}; // namespace graphic::message
