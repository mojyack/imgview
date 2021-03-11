#pragma once
#include <memory>
#include <vector>

#include "type.hpp"

namespace gawl {
enum class GraphicLoader {
    IMAGEMAGICK,
    DEVIL,
};

class GraphicData;

class GawlWindow;

class PixelBuffer {
  private:
    std::array<size_t, 2> size;
    std::vector<char>     data;

  public:
    bool        empty() const noexcept;
    size_t      get_width() const noexcept;
    size_t      get_height() const noexcept;
    const char* get_buffer() const noexcept;
    void        clear();
    PixelBuffer(){};
    PixelBuffer(const size_t width, const size_t height, const char* buffer);
    PixelBuffer(const size_t width, const size_t height, std::vector<char>& buffer);
    PixelBuffer(const char* file, GraphicLoader loader = GraphicLoader::IMAGEMAGICK);
    PixelBuffer(std::vector<char>& buffer);
};

class Graphic {
  private:
    std::shared_ptr<GraphicData> graphic_data;

  public:
    int  get_width(const GawlWindow* window) const;
    int  get_height(const GawlWindow* window) const;
    void draw(const GawlWindow* window, double x, double y);
    void draw_rect(const GawlWindow* window, Area area);
    void draw_fit_rect(const GawlWindow* window, Area area);
    void clear();
         operator bool();
    Graphic(const Graphic&);
    Graphic(Graphic&&);
    Graphic& operator=(const Graphic&);
    Graphic& operator=(Graphic&&);
    Graphic();
    Graphic(const char* file, GraphicLoader loader = GraphicLoader::IMAGEMAGICK);
    Graphic(std::vector<char>& buffer);
    Graphic(const PixelBuffer& buffer);
    Graphic(const PixelBuffer&& buffer);
};
} // namespace gawl
