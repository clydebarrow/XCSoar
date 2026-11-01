// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficAlertWidget.hpp"
#include "TrafficMonitor.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Look/FontDescription.hpp"
#include "Form/Button.hpp"
#include "FLARM/Details.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "util/StaticString.hxx"

#include <array>
#include <cmath>

/* FLARM alarm type of a collision alarm with an aircraft */
static constexpr uint8_t ALARM_TYPE_AIRCRAFT = 2;

/* within this height difference the traffic counts as level, in
   meters */
static constexpr double LEVEL_HEIGHT = 30;

/* a climb or descent faster than this is shown, in meters per
   second */
static constexpr double MIN_CLIMB_RATE = 1;

static constexpr Color IMPORTANT_BACKGROUND = Color(0xff, 0xb0, 0x00);
static constexpr Color URGENT_BACKGROUND = Color(0xc8, 0x00, 0x00);

class TrafficAlertWindow final : public ContainerWindow {
  TrafficMonitor &monitor;

  Font large_font, text_font, small_font;

  Button dismiss_button, mute_button;

  /** the part above the buttons */
  PixelRect content;

public:
  TrafficAlertWindow(ContainerWindow &parent, const PixelRect &rc,
                     TrafficMonitor &_monitor) noexcept;

private:
  void Layout(PixelSize size) noexcept;

  void PaintHeight(Canvas &canvas, const PixelRect &rc,
                   const FlarmStatus &status, const FlarmTraffic *traffic,
                   Color color) noexcept;
  void PaintDirection(Canvas &canvas, const PixelRect &rc,
                      const FlarmStatus &status, Color color) noexcept;
  void PaintText(Canvas &canvas, const PixelRect &rc,
                 const FlarmStatus &status, const FlarmTraffic *traffic,
                 Color color) noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};

TrafficAlertWindow::TrafficAlertWindow(ContainerWindow &parent,
                                       const PixelRect &rc,
                                       TrafficMonitor &_monitor) noexcept
  :monitor(_monitor)
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();
  Create(parent, rc, style);

  large_font.Load(FontDescription(Layout::FontScale(24), true));
  text_font.Load(FontDescription(Layout::FontScale(16), true));
  small_font.Load(FontDescription(Layout::FontScale(12), true));

  const ButtonLook &look = UIGlobals::GetDialogLook().button;
  WindowStyle button_style;
  button_style.TabStop();
  dismiss_button.Create(*this, look, _("Dismiss"), PixelRect{}, button_style,
                        [this]{ monitor.DismissAlert(); });
  mute_button.Create(*this, look, _("Mute 1 min"), PixelRect{}, button_style,
                     [this]{ monitor.MuteAlert(); });

  Layout(rc.GetSize());
}

void
TrafficAlertWindow::Layout(PixelSize size) noexcept
{
  /* large buttons, for gloves and turbulence */
  const unsigned margin = Layout::GetTextPadding() * 4;
  const unsigned button_height =
    std::max(Layout::GetMaximumControlHeight() * 3 / 2,
             unsigned(size.height / 6));

  PixelRect buttons{0, int(size.height - button_height - margin),
                    int(size.width), int(size.height - margin)};
  content = PixelRect{0, 0, int(size.width), buttons.top - int(margin)};

  const int middle = int(size.width) / 2;
  dismiss_button.Move({int(margin), buttons.top,
                       middle - int(margin / 2), buttons.bottom});
  mute_button.Move({middle + int(margin / 2), buttons.top,
                    int(size.width - margin), buttons.bottom});
  dismiss_button.Show();
  mute_button.Show();
}

void
TrafficAlertWindow::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);
  Layout(new_size);
  Invalidate();
}

/**
 * The hour on a clock face for a bearing relative to the track: 12 is
 * ahead, 3 to the right.
 */
static unsigned
ClockPosition(int bearing) noexcept
{
  const int hour = ((bearing % 360 + 360 + 15) % 360) / 30;
  return hour == 0 ? 12 : unsigned(hour);
}

/**
 * Draw #text centered horizontally on #x, with #font if it fits in
 * #width, otherwise with #fallback.
 */
static void
DrawCentered(Canvas &canvas, int x, int y, unsigned width, const char *text,
             const Font &font, const Font &fallback) noexcept
{
  canvas.Select(font);
  PixelSize size = canvas.CalcTextSize(text);
  if (size.width > width) {
    canvas.Select(fallback);
    size = canvas.CalcTextSize(text);
  }
  canvas.DrawText({x - int(size.width) / 2, y}, text);
}

void
TrafficAlertWindow::PaintHeight(Canvas &canvas, const PixelRect &rc,
                                const FlarmStatus &status,
                                const FlarmTraffic *traffic,
                                Color color) noexcept
{
  if (!status.has_vertical)
    return;

  const double height = status.relative_vertical;
  const bool level = std::fabs(height) < LEVEL_HEIGHT;

  /* a triangle pointing up for traffic above, down for below, a bar
     for level */
  const int x = rc.left + int(rc.GetWidth()) / 2;
  const int half = int(rc.GetWidth()) * 35 / 100;
  const int symbol_height = half * 2;
  const int top = rc.top + int(Layout::GetTextPadding()) * 4;

  const Brush brush(color);
  canvas.SelectNullPen();
  canvas.Select(brush);
  if (level) {
    const int bar = std::max(symbol_height / 5, int(Layout::Scale(6)));
    const int y = top + symbol_height / 2 - bar / 2;
    canvas.DrawFilledRectangle({x - half, y, x + half, y + bar}, brush);
  } else {
    const int tip = height > 0 ? top : top + symbol_height;
    const int base = height > 0 ? top + symbol_height : top;
    const BulkPixelPoint points[] = {
      {x, tip}, {x + half, base}, {x - half, base},
    };
    canvas.DrawPolygon(points, std::size(points));
  }

  int y = top + symbol_height + int(Layout::GetTextPadding()) * 2;
  StaticString<32> text;
  if (level)
    text = _("Level");
  else
    text = FormatUserAltitude(std::fabs(height)).c_str();
  DrawCentered(canvas, x, y, rc.GetWidth(), text.c_str(), large_font,
               text_font);
  y += int(large_font.GetHeight());

  if (traffic != nullptr && traffic->climb_rate_received &&
      std::fabs(traffic->climb_rate) >= MIN_CLIMB_RATE)
    DrawCentered(canvas, x, y, rc.GetWidth(),
                 traffic->climb_rate > 0 ? _("climbing") : _("descending"),
                 text_font, small_font);
}

void
TrafficAlertWindow::PaintDirection(Canvas &canvas, const PixelRect &rc,
                                   const FlarmStatus &status,
                                   Color color) noexcept
{
  const unsigned text_height = text_font.GetHeight();
  const unsigned size = std::min(rc.GetWidth(),
                                 rc.GetHeight() - text_height);
  const unsigned radius = size * 45 / 100;
  const PixelPoint center{rc.left + int(rc.GetWidth()) / 2,
                          rc.top + int(size) / 2};

  const Pen circle_pen(Layout::Scale(3), color);
  canvas.SelectHollowBrush();
  canvas.Select(circle_pen);
  canvas.DrawCircle(center, radius);

  /* the glider, at the center, flying up */
  const int g = int(radius) / 8;
  const Pen glider_pen(Layout::Scale(4), color);
  canvas.Select(glider_pen);
  canvas.DrawLine(center.At(-2 * g, 0), center.At(2 * g, 0));
  canvas.DrawLine(center.At(0, -g), center.At(0, 2 * g));

  StaticString<32> clock;

  if (status.has_bearing) {
    /* an arrow from the center towards the traffic */
    const float angle = float(status.relative_bearing) * float(M_PI) / 180;
    const float s = std::sin(angle), c = std::cos(angle);
    const float r = float(radius);
    static constexpr std::array<std::pair<float, float>, 7> shape{{
        {-0.10f, -0.30f}, {-0.10f, -0.55f}, {-0.28f, -0.55f},
        {0.0f, -0.95f},
        {0.28f, -0.55f}, {0.10f, -0.55f}, {0.10f, -0.30f},
      }};

    std::array<BulkPixelPoint, shape.size()> points;
    for (std::size_t i = 0; i < shape.size(); ++i) {
      const float x = shape[i].first * r, y = shape[i].second * r;
      points[i] = BulkPixelPoint(center.x + int(std::lround(x * c - y * s)),
                                 center.y + int(std::lround(x * s + y * c)));
    }

    const Brush arrow_brush(color);
    canvas.SelectNullPen();
    canvas.Select(arrow_brush);
    canvas.DrawPolygon(points.data(), points.size());

    clock.Format(_("%u o'clock"), ClockPosition(status.relative_bearing));
  } else
    clock = _("Direction unknown");

  canvas.Select(text_font);
  const PixelSize clock_size = canvas.CalcTextSize(clock.c_str());
  canvas.DrawText({center.x - int(clock_size.width) / 2,
                   rc.top + int(size)},
                  clock.c_str());
}

/**
 * A plain name for the kind of aircraft, or nullptr if not known.
 */
static const char *
GetTypeName(FlarmTraffic::AircraftType type) noexcept
{
  using Type = FlarmTraffic::AircraftType;
  switch (type) {
  case Type::GLIDER: return _("Glider");
  case Type::TOW_PLANE: return _("Tow plane");
  case Type::HELICOPTER: return _("Helicopter");
  case Type::PARACHUTE: return _("Parachute");
  case Type::DROP_PLANE: return _("Drop plane");
  case Type::HANG_GLIDER: return _("Hang glider");
  case Type::PARA_GLIDER: return _("Paraglider");
  case Type::POWERED_AIRCRAFT: return _("Powered aircraft");
  case Type::JET_AIRCRAFT: return _("Jet");
  case Type::BALLOON: return _("Balloon");
  case Type::AIRSHIP: return _("Airship");
  case Type::UAV: return _("Drone");
  case Type::STATIC_OBJECT: return _("Obstacle");
  case Type::UNKNOWN:
  case Type::FLYING_SAUCER:
    break;
  }

  return nullptr;
}

void
TrafficAlertWindow::PaintText(Canvas &canvas, const PixelRect &rc,
                              const FlarmStatus &status,
                              const FlarmTraffic *traffic,
                              Color color) noexcept
{
  canvas.SetTextColor(color);

  const int padding = int(Layout::GetTextPadding()) * 2;
  PixelPoint p{rc.left + padding, rc.top + padding};

  const char *headline = status.alarm_level == FlarmTraffic::AlarmType::URGENT
    ? _("Traffic urgent")
    : _("Traffic");
  canvas.Select(large_font);
  if (canvas.CalcTextWidth(headline) + 2 * unsigned(padding) > rc.GetWidth())
    canvas.Select(text_font);
  canvas.DrawText(p, headline);
  p.y += int(large_font.GetHeight()) + padding;

  canvas.Select(text_font);

  StaticString<64> line;
  line = FormatUserDistanceSmart(status.relative_distance).c_str();
  if (status.has_proximity_time)
    line.AppendFormat(", %s %u s", _("close in"),
                      unsigned(std::lround(status.proximity_time)));
  canvas.DrawText(p, line.c_str());
  p.y += int(text_font.GetHeight()) + padding;

  const ResolvedInfo info = FlarmDetails::ResolveInfo(status.target_id);

  const char *name = !info.callsign.empty() ? info.callsign.c_str() : nullptr;
  if (name == nullptr && traffic != nullptr && traffic->HasName())
    name = traffic->name.c_str();
  if (name != nullptr) {
    canvas.DrawText(p, name);
    p.y += int(text_font.GetHeight()) + padding;
  }

  /* the model from the FLARM databases, or else the kind the aircraft
     reports */
  const char *type = !info.plane_type.empty() ? info.plane_type.c_str() : nullptr;
  if (type == nullptr && traffic != nullptr)
    type = GetTypeName(traffic->type);
  if (type != nullptr)
    canvas.DrawText(p, type);
}

void
TrafficAlertWindow::OnPaint(Canvas &canvas) noexcept
{
  const FlarmStatus &status = CommonInterface::Basic().flarm.status;
  canvas.SetBackgroundTransparent();

  if (!status.available || status.alarm_type != ALARM_TYPE_AIRCRAFT ||
      status.alarm_level == FlarmTraffic::AlarmType::NONE) {
    /* shown for a few seconds before the alert closes */
    canvas.Clear(COLOR_DARK_GRAY);
    canvas.SetTextColor(COLOR_WHITE);
    canvas.Select(large_font);
    const char *text = _("Alarm cleared");
    const PixelSize size = canvas.CalcTextSize(text);
    const PixelPoint center = content.GetCenter();
    canvas.DrawText({center.x - int(size.width) / 2,
                     center.y - int(size.height) / 2}, text);
  } else {
    const bool urgent =
      status.alarm_level == FlarmTraffic::AlarmType::URGENT;
    const Color color = urgent ? COLOR_WHITE : COLOR_BLACK;

    canvas.Clear(urgent ? URGENT_BACKGROUND : IMPORTANT_BACKGROUND);
    canvas.SetTextColor(color);

    const FlarmTraffic *traffic =
      CommonInterface::Basic().flarm.traffic.FindTraffic(status.target_id);

    /* the graphics beside the text in landscape, above it in portrait */
    const bool portrait = content.GetHeight() > content.GetWidth();
    const auto [graphics, text] = portrait
      ? content.HorizontalSplit(content.top + int(content.GetHeight()) * 3 / 5)
      : content.VerticalSplit();
    const auto [height, direction] =
      graphics.VerticalSplit(graphics.left + int(graphics.GetWidth()) * 3 / 10);
    PaintHeight(canvas, height, status, traffic, color);
    PaintDirection(canvas, direction, status, color);
    PaintText(canvas, text, status, traffic, color);
  }

  ContainerWindow::OnPaint(canvas);
}

TrafficAlertWidget::TrafficAlertWidget(TrafficMonitor &_monitor) noexcept
  :monitor(_monitor),
   timer([this]{ monitor.CheckAlert(); })
{
}

TrafficAlertWidget::~TrafficAlertWidget() noexcept
{
  monitor.OnAlertClosed(*this);
}

void
TrafficAlertWidget::Update() noexcept
{
  if (IsDefined())
    GetWindow().Invalidate();
}

void
TrafficAlertWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  SetWindow(std::make_unique<TrafficAlertWindow>(parent, rc, monitor));
}

void
TrafficAlertWidget::Show(const PixelRect &rc) noexcept
{
  WindowWidget::Show(rc);
  timer.Schedule(std::chrono::seconds(1));
}

void
TrafficAlertWidget::Hide() noexcept
{
  timer.Cancel();
  WindowWidget::Hide();
}
