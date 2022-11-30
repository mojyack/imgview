#pragma once
#include "graphic/layer.hpp"
#include "graphic/message.hpp"
#include "graphic/single.hpp"
#include "graphic/text.hpp"
#include "util/variant.hpp"

struct Caption {
    std::array<size_t, 4> area;
    std::string           text;
    gawl::WrappedText     wrapped_text;

    int         size;
    gawl::Align alignx;
    gawl::Align aligny;
};

struct CaptionDrawHint {
    Caption&        caption;
    gawl::Rectangle area;
    double          ext;
};

using Graphic = Variant<graphic::single::SingleGraphic, graphic::layer::LayerGraphic, graphic::message::MessageGraphic, graphic::text::TextGraphic>;

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
