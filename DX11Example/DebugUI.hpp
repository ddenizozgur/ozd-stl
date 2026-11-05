#pragma once

#include "../Win32Stuff/WindowRelated.hpp"
#include "R.hpp"

struct Debug_Widget_Id
{
  const void *callSite;
  u32 repeat;
};

[[nodiscard]] constexpr bool DebugIdMatch(Debug_Widget_Id a, Debug_Widget_Id b)
{
  return a.callSite == b.callSite && a.repeat == b.repeat;
}

#define DEBUG_CALL_SITE ([] { static char callSite; return &callSite; }())

struct Debug_Renderer
{
  DX11_GPU gpu;
  R_Batch *batch;
  Glyph_Atlas *atlas;
  DX11_Texture2D atlasTexture;
  Win32_DWrite dwrite;
  Win32_DWrite_Font font;
  f32 fontSize;
  f32 dpiScale;
};

struct Debug_UI
{
  Debug_Widget_Id active;
  Vector2<f32> panelPosition;
  Vector2<f32> grabOffset;

  const Win32_Input_State *input;
  Debug_Renderer renderer;
  Font_Metrics fontMetrics;
  Vector2<f32> cursor;
  bool activeSeen;
  Debug_Widget_Id callSiteStorage[256];
  Fixed_List<Debug_Widget_Id> callSites;
};

struct Debug_Rect
{
  Vector2<f32> position;
  Vector2<f32> size;
};

struct Debug_Interaction
{
  bool hot;
  bool active;
  bool pressed;
  bool clicked;
};

inline constexpr f32 DEBUG_PANEL_WIDTH = 300;
inline constexpr f32 DEBUG_PADDING = 4;
inline constexpr f32 DEBUG_ROW_SPACING = 4;
inline constexpr f32 DEBUG_ROW_INNER = 2;
inline constexpr f32 DEBUG_CORNER_RADIUS = 4;

inline constexpr Vector4<u8> DEBUG_COLOR_PANEL = {24, 26, 32, 235};
inline constexpr Vector4<u8> DEBUG_COLOR_TITLE = {44, 48, 60, 255};
inline constexpr Vector4<u8> DEBUG_COLOR_WIDGET = {48, 53, 66, 255};
inline constexpr Vector4<u8> DEBUG_COLOR_WIDGET_HOT = {62, 68, 84, 255};
inline constexpr Vector4<u8> DEBUG_COLOR_WIDGET_ACTIVE = {78, 86, 106, 255};
inline constexpr Vector4<u8> DEBUG_COLOR_ACCENT = {65, 106, 163, 255};
inline constexpr Vector4<u8> DEBUG_COLOR_TEXT = {230, 232, 240, 255};

[[nodiscard]] inline f32 Debug_Px_(const Debug_UI *ui, f32 pixelsAt100)
{
  return RoundHalfDownF32(pixelsAt100 * ui->renderer.dpiScale);
}

[[nodiscard]] inline f32 Debug_RowHeight_(const Debug_UI *ui)
{
  return ui->fontMetrics.lineHeightRounded + Debug_Px_(ui, DEBUG_ROW_INNER) * 2;
}

[[nodiscard]] inline Debug_Widget_Id Debug_IdFromCallSite_(Debug_UI *ui, const void *callSite)
{
  for (auto &site : ui->callSites)
  {
    if (site.callSite == callSite)
    {
      site.repeat += 1;
      return site;
    }
  }
  Debug_Widget_Id id = {callSite, 0};
  usize appended = FixedListAppend(&ui->callSites, SliceMake(&id, 1));
  assert(appended == 1 && "too many debug UI call sites in one frame");
  (void)appended;
  return id;
}

[[nodiscard]] constexpr bool Debug_MouseIn_(const Debug_UI *ui, Debug_Rect rect)
{
  auto mouse = ui->input->mousePosition;
  auto rectRightBottom = rect.position + rect.size;
  return ui->input->mouseInsideClient &&
         mouse.x >= rect.position.x && mouse.x < rectRightBottom.x &&
         mouse.y >= rect.position.y && mouse.y < rectRightBottom.y;
}

inline Debug_Interaction Debug_Interact_(Debug_UI *ui, Debug_Widget_Id id, Debug_Rect rect)
{
  bool noneActive = ui->active.callSite == nullptr;
  bool pressedNow = Win32_ButtonPressed(ui->input, Mouse_Button_Left);

  Debug_Interaction interaction = {.active = DebugIdMatch(ui->active, id)};
  bool canHover = interaction.active || (noneActive && (!Win32_ButtonDown(ui->input, Mouse_Button_Left) || pressedNow));
  interaction.hot = canHover && Debug_MouseIn_(ui, rect);

  if (interaction.hot && noneActive && pressedNow)
  {
    ui->active = id;
    interaction.active = true;
    interaction.pressed = true;
  }
  if (interaction.active)
  {
    ui->activeSeen = true;
    if (Win32_ButtonReleased(ui->input, Mouse_Button_Left))
    {
      interaction.clicked = interaction.hot;
      ui->active = {};
    }
  }
  return interaction;
}

inline void Debug_Rect_(Debug_UI *ui, Debug_Rect rect, Vector4<u8> color, f32 cornerRadius = 0, f32 borderThickness = 0)
{
  R_PushRect(ui->renderer.gpu, ui->renderer.batch, rect.position, rect.size, color,
             Debug_Px_(ui, cornerRadius), Debug_Px_(ui, borderThickness));
}

inline void Debug_Text_(Debug_UI *ui, Vector2<f32> rowPosition, String text)
{
  auto renderer = &ui->renderer;
  f32 inner = Debug_Px_(ui, DEBUG_ROW_INNER);
  Vector2<f32> pen = {rowPosition.x + inner * 2, rowPosition.y + inner + ui->fontMetrics.ascent};
  R_PushText(renderer->gpu, renderer->batch, renderer->atlas, renderer->atlasTexture, renderer->dwrite, pen,
             renderer->font, renderer->fontSize, DEBUG_COLOR_TEXT, text);
}

[[nodiscard]] constexpr Vector4<u8> Debug_WidgetColor_(Debug_Interaction interaction)
{
  auto res = DEBUG_COLOR_WIDGET;
  if (interaction.active)
  {
    res = DEBUG_COLOR_WIDGET_ACTIVE;
  }
  else if (interaction.hot)
  {
    res = DEBUG_COLOR_WIDGET_HOT;
  }
  return res;
}

[[nodiscard]] inline Debug_Rect Debug_NextRow_(Debug_UI *ui, f32 height)
{
  f32 padding = Debug_Px_(ui, DEBUG_PADDING);
  f32 spacing = Debug_Px_(ui, DEBUG_ROW_SPACING);
  f32 width = Debug_Px_(ui, DEBUG_PANEL_WIDTH);
  Debug_Rect_(ui, {ui->cursor, {width, height + spacing}}, DEBUG_COLOR_PANEL);

  Debug_Rect row = {{ui->cursor.x + padding, ui->cursor.y}, {width - padding * 2, height}};
  ui->cursor.y += height + spacing;
  return row;
}

inline void Debug_Begin(Debug_UI *ui, const Win32_Input_State *input, Debug_Renderer renderer, String title)
{
  ui->input = input;
  ui->renderer = renderer;
  ui->fontMetrics = Win32_DWriteFontMetrics(renderer.font, renderer.fontSize);
  ui->activeSeen = false;
  ui->callSites = FixedListInit(SliceFromArray(ui->callSiteStorage));

  f32 width = Debug_Px_(ui, DEBUG_PANEL_WIDTH);
  f32 titleHeight = Debug_RowHeight_(ui);
  Debug_Widget_Id titleId = {ui, 0};
  auto interaction = Debug_Interact_(ui, titleId, {ui->panelPosition, {width, titleHeight}});
  if (interaction.pressed)
  {
    ui->grabOffset = input->mousePosition - ui->panelPosition;
  }
  if (interaction.active)
  {
    ui->panelPosition = input->mousePosition - ui->grabOffset;
  }

  Debug_Rect_(ui, {ui->panelPosition, {width, titleHeight}}, DEBUG_COLOR_TITLE);
  Debug_Text_(ui, ui->panelPosition, title);

  f32 padding = Debug_Px_(ui, DEBUG_PADDING);
  ui->cursor = {ui->panelPosition.x, ui->panelPosition.y + titleHeight};
  Debug_Rect_(ui, {ui->cursor, {width, padding}}, DEBUG_COLOR_PANEL);
  ui->cursor.y += padding;
}

[[nodiscard]] inline bool Debug_End(Debug_UI *ui)
{
  f32 width = Debug_Px_(ui, DEBUG_PANEL_WIDTH);
  f32 bottomPadding = Debug_Px_(ui, DEBUG_PADDING) - Debug_Px_(ui, DEBUG_ROW_SPACING);
  Debug_Rect_(ui, {ui->cursor, {width, bottomPadding}}, DEBUG_COLOR_PANEL);
  ui->cursor.y += bottomPadding;

  Debug_Rect panel = {ui->panelPosition, {width, ui->cursor.y - ui->panelPosition.y}};
  bool hasMouse = Debug_MouseIn_(ui, panel) || ui->activeSeen;

  if (!ui->activeSeen || Win32_ButtonReleased(ui->input, Mouse_Button_Left))
  {
    ui->active = {};
  }
  return hasMouse;
}

// Widgets
inline void Debug_Label(Debug_UI *ui, String text)
{
  Debug_Rect row = Debug_NextRow_(ui, Debug_RowHeight_(ui));
  Debug_Text_(ui, row.position, text);
}

#define Debug_Button(ui, label) Debug_Button_((ui), DEBUG_CALL_SITE, (label))
inline bool Debug_Button_(Debug_UI *ui, const void *callSite, String label)
{
  Debug_Widget_Id id = Debug_IdFromCallSite_(ui, callSite);
  Debug_Rect row = Debug_NextRow_(ui, Debug_RowHeight_(ui));
  Debug_Interaction interaction = Debug_Interact_(ui, id, row);

  Debug_Rect_(ui, row, Debug_WidgetColor_(interaction), DEBUG_CORNER_RADIUS);
  Debug_Text_(ui, row.position, label);
  return interaction.clicked;
}

#define Debug_Checkbox(ui, value, label) Debug_Checkbox_((ui), DEBUG_CALL_SITE, (value), (label))
inline bool Debug_Checkbox_(Debug_UI *ui, const void *callSite, bool *value, String label)
{
  Debug_Widget_Id id = Debug_IdFromCallSite_(ui, callSite);
  f32 rowHeight = Debug_RowHeight_(ui);
  Debug_Rect row = Debug_NextRow_(ui, rowHeight);
  Debug_Interaction interaction = Debug_Interact_(ui, id, row);
  if (interaction.clicked)
  {
    *value = !*value;
  }

  if (interaction.hot || interaction.active)
  {
    Debug_Rect_(ui, row, Debug_WidgetColor_(interaction), DEBUG_CORNER_RADIUS);
  }
  Debug_Rect box = {row.position, {rowHeight, rowHeight}};
  Debug_Rect_(ui, box, DEBUG_COLOR_WIDGET_ACTIVE, DEBUG_CORNER_RADIUS, 1);
  if (*value)
  {
    f32 inset = Debug_Px_(ui, 4);
    Debug_Rect_(ui, {box.position + inset, box.size - inset * 2}, DEBUG_COLOR_ACCENT, DEBUG_CORNER_RADIUS / 2);
  }
  Debug_Text_(ui, {row.position.x + rowHeight, row.position.y}, label);
  return interaction.clicked;
}

#define Debug_SliderF32(ui, value, min, max, label) Debug_SliderF32_((ui), DEBUG_CALL_SITE, (value), (min), (max), (label))
inline bool Debug_SliderF32_(Debug_UI *ui, const void *callSite, f32 *value, f32 min, f32 max, String label)
{
  Debug_Widget_Id id = Debug_IdFromCallSite_(ui, callSite);
  Debug_Rect row = Debug_NextRow_(ui, Debug_RowHeight_(ui));
  Debug_Interaction interaction = Debug_Interact_(ui, id, row);

  f32 old = *value;
  if (interaction.active)
  {
    f32 t = Clamp((ui->input->mousePosition.x - row.position.x) / row.size.width, 0.0f, 1.0f);
    *value = LerpF32(min, max, t);
  }

  Debug_Rect_(ui, row, Debug_WidgetColor_(interaction), DEBUG_CORNER_RADIUS);
  f32 filled = Clamp((*value - min) / (max - min), 0.0f, 1.0f);
  Debug_Rect_(ui, {row.position, {RoundHalfDownF32(row.size.width * filled), row.size.height}}, DEBUG_COLOR_ACCENT, DEBUG_CORNER_RADIUS);
  Debug_Text_(ui, row.position, label);
  return *value != old;
}
