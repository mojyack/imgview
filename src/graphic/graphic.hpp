#pragma once
#include <concepts>

#include <gawl/wayland/gawl.hpp>

namespace graphic {
template <class T>
concept Graphic = requires(T& graphic) {
    { graphic.get_graphic() } -> std::same_as<gawl::Graphic>;
};
} // namespace graphic
