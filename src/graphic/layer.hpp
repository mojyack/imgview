#include <filesystem>
#include <fstream>

#include <ImageMagick-7/Magick++.h>
#include <gawl/graphic.hpp>

#include "../util/error.hpp"
#include "../util/string-map.hpp"
#include "../util/thread.hpp"

namespace graphic::layer {
struct PixelBuffer {
    size_t                width;
    size_t                height;
    std::vector<uint32_t> data;

    auto overlay(const PixelBuffer& o) -> Error {
        if(width != o.width || height != o.height) {
            return Error("overlay size mismatched");
        }
        for(auto i = size_t(0); i < width * height; i += 1) {
            auto f  = (o.data[i] & 0xFF000000) / 0xFF000000;
            data[i] = o.data[i] * f + data[i] * (1 - f);
        }
        return Error();
    }

    PixelBuffer(const size_t width, const size_t height) : width(width), height(height) {
        data.resize(width * height);
    }

    PixelBuffer(const char* const path) {
        auto image = Magick::Image(path);
        width      = image.columns();
        height     = image.rows();
        data.resize(width * height);
        image.write(0, 0, width, height, "RGBA", Magick::CharPixel, data.data());
    }
};

class LayerGraphicFactory;
class LayerGraphic {
    friend class LayerGraphicFactory;

  private:
    gawl::Graphic                             graphic;
    std::vector<std::shared_ptr<PixelBuffer>> buffers;

    static auto from_pixel_buffers(std::vector<std::shared_ptr<PixelBuffer>> buffers) -> Result<LayerGraphic> {
        auto buffer = *buffers[0];
        for(auto i = buffers.begin() + 1; i != buffers.end(); i += 1) {
            if(const auto e = buffer.overlay(**i)) {
                return e;
            }
        }
        const auto pixelbuffer = gawl::PixelBuffer::from_raw(buffer.width, buffer.height, reinterpret_cast<const std::byte*>(buffer.data.data()));
        return LayerGraphic(std::move(buffers), pixelbuffer);
    }

    LayerGraphic(std::vector<std::shared_ptr<PixelBuffer>> buffers, const gawl::PixelBuffer& composed) : graphic(composed), buffers(std::move(buffers)) {}

  public:
    auto get_graphic() -> gawl::Graphic* {
        return &graphic;
    }
};

struct LayerScript {
    std::vector<std::string> files;

    auto canonicalize(const std::string_view path) -> Error {
        const auto base = std::filesystem::path(path).parent_path();
        for(auto& f : files) {
            const auto abs = std::filesystem::path(base).append(f);
            try {
                f = std::filesystem::canonical(abs);
            } catch(const std::filesystem::filesystem_error& e) {
                return Error(e.what());
            }
        }
        return Error();
    }

    static auto from_file(const std::string_view path) -> Result<LayerScript> {
        if(!std::filesystem::is_regular_file(path)) {
            return Error("no such file");
        }

        auto source = std::fstream(path);
        auto line   = std::string();
        if(!std::getline(source, line)) {
            return Error("failed to read line");
        }
        if(line != "layer0") {
            return Error("unsupported layer script magic");
        }
        auto script = LayerScript();
        while(std::getline(source, line)) {
            script.files.emplace_back(std::move(line));
        }
        return script;
    }
};

class LayerGraphicFactory {
  private:
    Critical<StringMap<std::weak_ptr<PixelBuffer>>> critical_fragments;

    auto clear_freed_fragment_info(StringMap<std::weak_ptr<PixelBuffer>>& fragments) -> void {
        for(auto i = fragments.begin(); i != fragments.end(); i = std::next(i)) {
            if(i->second.expired()) {
                i = fragments.erase(i);
                if(i == fragments.end()) {
                    break;
                }
            }
        }
    }

  public:
    auto create_graphic(const std::string_view path) -> Result<LayerGraphic> {
        auto script_result = LayerScript::from_file(path);
        if(!script_result) {
            return script_result.as_error();
        }

        auto& script = script_result.as_value();
        if(script.files.empty()) {
            return Error("no layers");
        }
        if(const auto& e = script.canonicalize(path)) {
            return e;
        }

        auto buffers = StringMap<std::shared_ptr<PixelBuffer>>();

        {
            auto [lock, fragments] = critical_fragments.access();
            clear_freed_fragment_info(fragments);
            for(const auto& f : script.files) {
                if(const auto p = fragments.find(f); p != fragments.end()) {
                    auto buf = p->second.lock();
                    if(buf) {
                        buffers[f] = std::move(buf);
                    }
                } else {
                    auto buf     = std::shared_ptr<PixelBuffer>(new PixelBuffer(f.data()));
                    fragments[f] = buf;
                    buffers[f]   = std::move(buf);
                }
            }
        }

        auto ordered_buffers = std::vector<std::shared_ptr<PixelBuffer>>();
        ordered_buffers.reserve(script.files.size());
        for(auto i = script.files.begin(); i != script.files.end(); i += 1) {
            ordered_buffers.emplace_back(std::move(buffers[*i]));
        }

        return LayerGraphic::from_pixel_buffers(std::move(ordered_buffers));
    }
};
} // namespace graphic::layer
