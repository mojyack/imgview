#pragma once
#include <format>

#include <gawl/graphic.hpp>

namespace graphic::message {
class MessageGraphic {
  private:
    std::string message;

  public:
    auto get_graphic() const -> gawl::Graphic* {
        return nullptr;
    }

    auto get_message() const -> const std::string& {
        return message;
    }

    MessageGraphic(const std::string_view message) : message(message) {}
};
}; // namespace graphic::message
