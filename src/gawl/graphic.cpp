#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <IL/il.h>
#include <ImageMagick-7/Magick++.h>

#include "global.hpp"
#include "graphic-base.hpp"
#include "graphic.hpp"

namespace gawl {
namespace {
PixelBuffer load_texture_imagemagick(Magick::Image&& image) {
    image.depth(8);
    Magick::Blob blob;
    image.write(&blob, "RGBA");
    return PixelBuffer(image.columns(), image.rows(), static_cast<const char*>(blob.data()));
}
PixelBuffer load_texture_devil(const char* file) {
    std::vector<char> buffer;
    auto              image = ilGenImage();
    ilBindImage(image);
    ilLoadImage(file);
    if(auto err = ilGetError(); err != IL_NO_ERROR) {
        ilDeleteImage(image);
        return PixelBuffer();
    }
    const auto width  = ilGetInteger(IL_IMAGE_WIDTH);
    const auto height = ilGetInteger(IL_IMAGE_HEIGHT);
    buffer.resize(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, buffer.data());
    ilDeleteImage(image);
    return PixelBuffer(width, height, buffer);
}
} // namespace
extern GlobalVar* global;

// ====== PixelBuffer ====== //
bool PixelBuffer::empty() const noexcept {
    return data.empty();
}
size_t PixelBuffer::get_width() const noexcept {
    return size[0];
}
size_t PixelBuffer::get_height() const noexcept {
    return size[1];
}
const char* PixelBuffer::get_buffer() const noexcept {
    return data.data();
}
void PixelBuffer::clear() {
    size = {0, 0};
    data.clear();
}
PixelBuffer::PixelBuffer(const size_t width, const size_t height, const char* buffer) : size({width, height}) {
    size_t len = size[0] * size[1] * 4;
    data.resize(len);
    std::memcpy(data.data(), buffer, len);
}
PixelBuffer::PixelBuffer(const size_t width, const size_t height, std::vector<char>& buffer) : size({width, height}) {
    data = std::move(buffer);
}
PixelBuffer::PixelBuffer(const char* file, GraphicLoader loader) {
    bool        magick = loader == GraphicLoader::IMAGEMAGICK;
    PixelBuffer buf;
    if(!magick) {
        buf = load_texture_devil(file);
        if(buf.empty()) {
            magick = true; // fallback to imagemagick.
        }
    }
    if(magick) {
        buf = load_texture_imagemagick(Magick::Image(file));
    }
    if(!buf.empty()) {
        *this = std::move(buf);
    }
}
PixelBuffer::PixelBuffer(std::vector<char>& buffer) {
    Magick::Blob blob(buffer.data(), buffer.size());
    *this = load_texture_imagemagick(Magick::Image(blob));
}

// ====== GraphicData ====== //
class GraphicData : public GraphicBase {
  public:
    GraphicData(const PixelBuffer& buffer);
    GraphicData(const PixelBuffer&& buffer);
    ~GraphicData() {}
};

GraphicData::GraphicData(const PixelBuffer& buffer) : GraphicData(std::move(buffer)) {}
GraphicData::GraphicData(const PixelBuffer&& buffer) : GraphicBase(*global->graphic_shader) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    width  = buffer.get_width();
    height = buffer.get_height();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.get_buffer());
}

// ====== Graphic ====== //
int Graphic::get_width(const GawlWindow* window) const {
    if(!graphic_data) {
        return 0;
    }
    return reinterpret_cast<GraphicData*>(graphic_data.get())->get_width(window);
}
int Graphic::get_height(const GawlWindow* window) const {
    if(!graphic_data) {
        return 0;
    }
    return reinterpret_cast<GraphicData*>(graphic_data.get())->get_height(window);
}
void Graphic::draw(const GawlWindow* window, double x, double y) {
    if(!graphic_data) {
        return;
    }
    reinterpret_cast<GraphicData*>(graphic_data.get())->draw(window, x, y);
}
void Graphic::draw_rect(const GawlWindow* window, Area area) {
    if(!graphic_data) {
        return;
    }
    graphic_data.get()->draw_rect(window, area);
}
void Graphic::draw_fit_rect(const GawlWindow* window, Area area) {
    if(!graphic_data) {
        return;
    }
    graphic_data.get()->draw_fit_rect(window, area);
}
void Graphic::clear() {
    *this = Graphic();
}
Graphic::Graphic(const Graphic& src) {
    *this = src;
}
Graphic::Graphic(Graphic&& src) {
    *this = src;
}
Graphic& Graphic::operator=(const Graphic& src) {
    this->graphic_data = src.graphic_data;
    return *this;
}
Graphic& Graphic::operator=(Graphic&& src) {
    this->graphic_data = src.graphic_data;
    return *this;
}
Graphic::Graphic(const char* file, GraphicLoader loader) {
    GraphicData* data;
    try {
        data = new GraphicData(PixelBuffer(file, loader));
    } catch(std::exception&) {
        std::cout << file << " is not valid image file." << std::endl;
        return;
    }
    graphic_data.reset(data);
}
Graphic::Graphic(std::vector<char>& buffer) {
    GraphicData* data;
    try {
        data = new GraphicData(PixelBuffer(buffer));
    } catch(std::exception&) {
        std::cout << "invalid buffer." << std::endl;
        return;
    }
    graphic_data.reset(data);
}
Graphic::Graphic(const PixelBuffer& buffer) {
    graphic_data.reset(new GraphicData(buffer));
}
Graphic::Graphic(const PixelBuffer&& buffer) {
    graphic_data.reset(new GraphicData(std::forward<decltype(buffer)>(buffer)));
}
Graphic::Graphic() {}
Graphic::operator bool() {
    return static_cast<bool>(graphic_data);
}
} // namespace gawl
