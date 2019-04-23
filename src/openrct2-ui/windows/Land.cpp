/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <algorithm>
#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2-ui/interface/LandTool.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Context.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/TerrainEdgeObject.h>
#include <openrct2/object/TerrainSurfaceObject.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/Surface.h>

using namespace OpenRCT2;

// clang-format off
enum WINDOW_LAND_WIDGET_IDX {
    WIDX_BACKGROUND,
    WIDX_TITLE,
    WIDX_CLOSE,
    WIDX_MOUNTAINMODE,
    WIDX_PAINTMODE,
    WIDX_PREVIEW,
    WIDX_DECREMENT,
    WIDX_INCREMENT,
    WIDX_FLOOR,
    WIDX_WALL,
};

static rct_widget window_land_widgets[] = {
    { WWT_FRAME,    0,  0,  97, 0,  159,        0xFFFFFFFF,                             STR_NONE },                     // panel / background
    { WWT_CAPTION,  0,  1,  96, 1,  14,         STR_LAND,                               STR_WINDOW_TITLE_TIP },         // title bar
    { WWT_CLOSEBOX, 0,  85, 95, 2,  13,         STR_CLOSE_X,                            STR_CLOSE_WINDOW_TIP },         // close x button

    { WWT_FLATBTN,  1,  19, 42, 19, 42,         SPR_RIDE_CONSTRUCTION_SLOPE_UP,         STR_ENABLE_MOUNTAIN_TOOL_TIP }, // mountain mode
    { WWT_FLATBTN,  1,  55, 78, 19, 42,         SPR_PAINTBRUSH,                         STR_DISABLE_ELEVATION },        // paint mode

    { WWT_IMGBTN,   0,  27, 70, 48, 79,         SPR_LAND_TOOL_SIZE_0,                   STR_NONE },                     // preview box
    { WWT_TRNBTN,   1,  28, 43, 49, 64,         IMAGE_TYPE_REMAP | SPR_LAND_TOOL_DECREASE,    STR_ADJUST_SMALLER_LAND_TIP },  // decrement size
    { WWT_TRNBTN,   1,  54, 69, 63, 78,         IMAGE_TYPE_REMAP | SPR_LAND_TOOL_INCREASE,    STR_ADJUST_LARGER_LAND_TIP },   // increment size

    { WWT_FLATBTN,  1,  2,  48, 106,    141,    0xFFFFFFFF,                             STR_CHANGE_BASE_LAND_TIP },     // floor texture
    { WWT_FLATBTN,  1,  49, 95, 106,    141,    0xFFFFFFFF,                             STR_CHANGE_VERTICAL_LAND_TIP }, // wall texture
    { WIDGETS_END },
};

static void window_land_close(rct_window *w);
static void window_land_mouseup(rct_window *w, rct_widgetindex widgetIndex);
static void window_land_mousedown(rct_window *w, rct_widgetindex widgetIndex, rct_widget* widget);
static void window_land_dropdown(rct_window *w, rct_widgetindex widgetIndex, int32_t dropdownIndex);
static void window_land_update(rct_window *w);
static void window_land_invalidate(rct_window *w);
static void window_land_paint(rct_window *w, rct_drawpixelinfo *dpi);
static void window_land_textinput(rct_window *w, rct_widgetindex widgetIndex, char *text);
static void window_land_inputsize(rct_window *w);

static rct_window_event_list window_land_events = {
    window_land_close,
    window_land_mouseup,
    nullptr,
    window_land_mousedown,
    window_land_dropdown,
    nullptr,
    window_land_update,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_land_textinput,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_land_invalidate,
    window_land_paint,
    nullptr
};
// clang-format on

static int32_t _selectedFloorTexture;
static int32_t _selectedWallTexture;

/**
 *
 *  rct2: 0x00663E7D
 */
rct_window* window_land_open()
{
    rct_window* window;

    // Check if window is already open
    window = window_find_by_class(WC_LAND);
    if (window != nullptr)
        return window;

    window = window_create(context_get_width() - 98, 29, 98, 160, &window_land_events, WC_LAND, 0);
    window->widgets = window_land_widgets;
    window->enabled_widgets = (1 << WIDX_CLOSE) | (1 << WIDX_DECREMENT) | (1 << WIDX_INCREMENT) | (1 << WIDX_FLOOR)
        | (1 << WIDX_WALL) | (1 << WIDX_MOUNTAINMODE) | (1 << WIDX_PAINTMODE) | (1 << WIDX_PREVIEW);
    window->hold_down_widgets = (1 << WIDX_DECREMENT) | (1 << WIDX_INCREMENT);
    window_init_scroll_widgets(window);
    window_push_others_below(window);

    gLandToolSize = 1;
    gLandToolTerrainSurface = 255;
    gLandToolTerrainEdge = 255;
    gLandMountainMode = false;
    gLandPaintMode = false;
    _selectedFloorTexture = TERRAIN_GRASS;
    _selectedWallTexture = TERRAIN_EDGE_ROCK;
    gLandToolRaiseCost = MONEY32_UNDEFINED;
    gLandToolLowerCost = MONEY32_UNDEFINED;

    return window;
}

/**
 *
 *  rct2: 0x006640A5
 */
static void window_land_close(rct_window* w)
{
    // If the tool wasn't changed, turn tool off
    if (land_tool_is_active())
        tool_cancel();
}

/**
 *
 *  rct2: 0x00664064
 */
static void window_land_mouseup(rct_window* w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex)
    {
        case WIDX_CLOSE:
            window_close(w);
            break;
        case WIDX_MOUNTAINMODE:
            gLandMountainMode ^= 1;
            gLandPaintMode = 0;
            window_invalidate(w);
            break;
        case WIDX_PAINTMODE:
            gLandMountainMode = 0;
            gLandPaintMode ^= 1;
            window_invalidate(w);
            break;
        case WIDX_PREVIEW:
            window_land_inputsize(w);
            break;
    }
}

/**
 *
 *  rct2: 0x0066407B
 */
static void window_land_mousedown(rct_window* w, rct_widgetindex widgetIndex, rct_widget* widget)
{
    switch (widgetIndex)
    {
        case WIDX_FLOOR:
            land_tool_show_surface_style_dropdown(w, widget, _selectedFloorTexture);
            break;
        case WIDX_WALL:
            land_tool_show_edge_style_dropdown(w, widget, _selectedWallTexture);
            break;
        case WIDX_PREVIEW:
            window_land_inputsize(w);
            break;
        case WIDX_DECREMENT:
            // Decrement land tool size
            gLandToolSize = std::max(MINIMUM_TOOL_SIZE, gLandToolSize - 1);

            // Invalidate the window
            window_invalidate(w);
            break;
        case WIDX_INCREMENT:
            // Increment land tool size
            gLandToolSize = std::min(MAXIMUM_TOOL_SIZE, gLandToolSize + 1);

            // Invalidate the window
            window_invalidate(w);
            break;
    }
}

/**
 *
 *  rct2: 0x00664090
 */
static void window_land_dropdown(rct_window* w, rct_widgetindex widgetIndex, int32_t dropdownIndex)
{
    int32_t type;

    switch (widgetIndex)
    {
        case WIDX_FLOOR:
            if (dropdownIndex == -1)
                dropdownIndex = gDropdownHighlightedIndex;

            type = (dropdownIndex == -1) ? _selectedFloorTexture : dropdownIndex;

            if (gLandToolTerrainSurface == type)
            {
                gLandToolTerrainSurface = 255;
            }
            else
            {
                gLandToolTerrainSurface = type;
                _selectedFloorTexture = type;
            }
            window_invalidate(w);
            break;
        case WIDX_WALL:
            if (dropdownIndex == -1)
                dropdownIndex = gDropdownHighlightedIndex;

            type = (dropdownIndex == -1) ? _selectedWallTexture : dropdownIndex;

            if (gLandToolTerrainEdge == type)
            {
                gLandToolTerrainEdge = 255;
            }
            else
            {
                gLandToolTerrainEdge = type;
                _selectedWallTexture = type;
            }
            window_invalidate(w);
            break;
    }
}

static void window_land_textinput(rct_window* w, rct_widgetindex widgetIndex, char* text)
{
    int32_t size;
    char* end;

    if (widgetIndex != WIDX_PREVIEW || text == nullptr)
        return;

    size = strtol(text, &end, 10);
    if (*end == '\0')
    {
        size = std::max(MINIMUM_TOOL_SIZE, size);
        size = std::min(MAXIMUM_TOOL_SIZE, size);
        gLandToolSize = size;

        window_invalidate(w);
    }
}

static void window_land_inputsize(rct_window* w)
{
    TextInputDescriptionArgs[0] = MINIMUM_TOOL_SIZE;
    TextInputDescriptionArgs[1] = MAXIMUM_TOOL_SIZE;
    window_text_input_open(w, WIDX_PREVIEW, STR_SELECTION_SIZE, STR_ENTER_SELECTION_SIZE, STR_NONE, STR_NONE, 3);
}

/**
 *
 *  rct2: 0x00664272
 */
static void window_land_update(rct_window* w)
{
    if (!land_tool_is_active())
        window_close(w);
}

/**
 *
 *  rct2: 0x00663F20
 */
static void window_land_invalidate(rct_window* w)
{
    auto surfaceImage = (uint32_t)SPR_NONE;
    auto edgeImage = (uint32_t)SPR_NONE;

    auto& objManager = GetContext()->GetObjectManager();
    const auto surfaceObj = static_cast<TerrainSurfaceObject*>(
        objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_SURFACE, _selectedFloorTexture));
    if (surfaceObj != nullptr)
    {
        surfaceImage = surfaceObj->IconImageId;
        if (surfaceObj->Colour != 255)
        {
            surfaceImage |= surfaceObj->Colour << 19 | IMAGE_TYPE_REMAP;
        }
    }
    const auto edgeObj = static_cast<TerrainEdgeObject*>(
        objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_EDGE, _selectedWallTexture));
    if (edgeObj != nullptr)
    {
        edgeImage = edgeObj->IconImageId;
    }

    w->pressed_widgets = (1 << WIDX_PREVIEW);
    if (gLandToolTerrainSurface != 255)
        w->pressed_widgets |= (1 << WIDX_FLOOR);
    if (gLandToolTerrainEdge != 255)
        w->pressed_widgets |= (1 << WIDX_WALL);
    if (gLandMountainMode)
        w->pressed_widgets |= (1 << WIDX_MOUNTAINMODE);
    if (gLandPaintMode)
        w->pressed_widgets |= (1 << WIDX_PAINTMODE);

    window_land_widgets[WIDX_FLOOR].image = surfaceImage;
    window_land_widgets[WIDX_WALL].image = edgeImage;
    // Update the preview image (for tool sizes up to 7)
    window_land_widgets[WIDX_PREVIEW].image = land_tool_size_to_sprite_index(gLandToolSize);
}

/**
 *
 *  rct2: 0x00663F7C
 */
static void window_land_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    int32_t x, y, numTiles;
    money32 price;
    rct_widget* previewWidget = &window_land_widgets[WIDX_PREVIEW];

    window_draw_widgets(w, dpi);

    // Draw number for tool sizes bigger than 7
    if (gLandToolSize > MAX_TOOL_SIZE_WITH_SPRITE)
    {
        x = w->x + (previewWidget->left + previewWidget->right) / 2;
        y = w->y + (previewWidget->top + previewWidget->bottom) / 2;
        gfx_draw_string_centred(dpi, STR_LAND_TOOL_SIZE_VALUE, x, y - 2, COLOUR_BLACK, &gLandToolSize);
    }
    else if (gLandMountainMode)
    {
        x = w->x + previewWidget->left;
        y = w->y + previewWidget->top;
        gfx_draw_sprite(dpi, SPR_LAND_TOOL_SIZE_0, x, y, 0);
    }

    x = w->x + (previewWidget->left + previewWidget->right) / 2;
    y = w->y + previewWidget->bottom + 5;

    if (!(gParkFlags & PARK_FLAGS_NO_MONEY))
    {
        // Draw raise cost amount
        if (gLandToolRaiseCost != MONEY32_UNDEFINED && gLandToolRaiseCost != 0)
            gfx_draw_string_centred(dpi, STR_RAISE_COST_AMOUNT, x, y, COLOUR_BLACK, &gLandToolRaiseCost);
        y += 10;

        // Draw lower cost amount
        if (gLandToolLowerCost != MONEY32_UNDEFINED && gLandToolLowerCost != 0)
            gfx_draw_string_centred(dpi, STR_LOWER_COST_AMOUNT, x, y, COLOUR_BLACK, &gLandToolLowerCost);
        y += 50;

        // Draw paint price
        numTiles = gLandToolSize * gLandToolSize;
        price = 0;
        if (gLandToolTerrainSurface != 255)
        {
            auto& objManager = GetContext()->GetObjectManager();
            const auto surfaceObj = static_cast<TerrainSurfaceObject*>(
                objManager.GetLoadedObject(OBJECT_TYPE_TERRAIN_SURFACE, gLandToolTerrainSurface));
            if (surfaceObj != nullptr)
            {
                price += numTiles * surfaceObj->Price;
            }
        }

        if (gLandToolTerrainEdge != 255)
            price += numTiles * 100;

        if (price != 0)
        {
            set_format_arg(0, money32, price);
            gfx_draw_string_centred(dpi, STR_COST_AMOUNT, x, y, COLOUR_BLACK, gCommonFormatArgs);
        }
    }
}
