#include <fontconfig/fontconfig.h>

#include <string>

namespace fc {
inline auto find_fontpath_from_name(const char* const name) -> std::string {
    const auto config  = FcInitLoadConfigAndFonts();
    const auto pattern = FcNameParse((const FcChar8*)(name));
    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    auto       result = FcResult();
    const auto font   = FcFontMatch(config, pattern, &result);
    auto       path   = std::string();
    if(font) {
        auto file = (FcChar8*)(nullptr);
        if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            path = std::string((char*)file);
        }
        FcPatternDestroy(font);
    }
    FcPatternDestroy(pattern);
    FcConfigDestroy(config);
    return path;
}
} // namespace fc
