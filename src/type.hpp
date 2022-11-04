#pragma once
#include "graphic/layer.hpp"
#include "graphic/message.hpp"
#include "graphic/single.hpp"
#include "util/variant.hpp"

struct Caption {
    std::array<size_t, 4> area;
    std::string           text;

    int         size;
    gawl::Align alignx;
    gawl::Align aligny;
};

using Graphic = Variant<graphic::single::SingleGraphic, graphic::layer::LayerGraphic, graphic::message::MessageGraphic>;

struct Image {
    Graphic              graphic;
    std::vector<Caption> captions;
};

enum class Actions {
    None,
    QuitApp,
    NextWork,
    PrevWork,
    NextPage,
    PrevPage,
    RefreshFiles,
    PageSelectOn,
    PageSelectOff,
    PageSelectNum,
    PageSelectNumDel,
    PageSelectApply,
    ToggleShowInfo,
    MoveDrawPos,
    ResetDrawPos,
    FitWidth,
    FitHeight,
};

enum class InfoFormats {
    None,
    Short,
    Long,
};

struct CaptionDrawHint {
    const Caption&  caption;
    gawl::Rectangle area;
    double          ext;
};
