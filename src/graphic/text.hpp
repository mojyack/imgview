#pragma once
#include <fstream>

#include <gawl/wayland/gawl.hpp>

#include "../util/error.hpp"

namespace graphic::text {
class TextGraphic {
  private:
    std::string       text;
    gawl::WrappedText wrapped_text;

    TextGraphic(std::string text) : text(std::move(text)) {}

  public:
    auto get_graphic() const -> gawl::Graphic* {
        return nullptr;
    }

    auto get_text() -> std::tuple<std::string&, gawl::WrappedText&> {
        return {text, wrapped_text};
    }

    static auto from_file(const std::string_view path) -> Result<TextGraphic> {
        auto in = std::ifstream();
        try {
            in.open(path, std::ios::binary);
        } catch(const std::runtime_error& e) {
            return Error(e.what());
        }

        return TextGraphic(std::string(std::istreambuf_iterator<char>(in), {}));
    }
};
} // namespace graphic::text
