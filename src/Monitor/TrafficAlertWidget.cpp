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

  Font large_font, text_font;

  Button dismiss_button, mute_button;

  /** the part above the buttons */
  PixelRect content;

public:
  TrafficAlertWindow(ContainerWindow &parent, const PixelRect &rc,
                     TrafficMonitor &_monitor) noexcept;

private:
  void Layout(PixelSize size) noexcept;

  void PaintDirection(Canvas &canvas, const PixelRect &rc,
                      const FlarmStatus &status, Color color) noexcept;
  void PaintText(Canvas &canvas, const PixelRect &rc,
                 const FlarmStatus &status, Color color) noexcept;

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

void
TrafficAlertWindow::PaintText(Canvas &canvas, const PixelRect &rc,
                              const FlarmStatus &status,
                              Color color) noexcept
{
  canvas.SetTextColor(color);

  const int padding = int(Layout::GetTextPadding()) * 2;
  PixelPoint p{rc.left + padding, rc.top + padding};

  canvas.Select(large_font);
  canvas.DrawText(p, status.alarm_level == FlarmTraffic::AlarmType::URGENT
                  ? _("Traffic urgent")
                  : _("Traffic"));
  p.y += int(large_font.GetHeight()) + padding;

  const auto &basic = CommonInterface::Basic();
  const FlarmTraffic *traffic =
    basic.flarm.traffic.FindTraffic(status.target_id);

  StaticString<64> line;

  if (status.has_vertical) {
    const double height = status.relative_vertical;
    if (std::fabs(height) < LEVEL_HEIGHT)
      line = _("Level");
    else
      line.Format("%s %s", FormatUserAltitude(std::fabs(height)).c_str(),
                  height > 0 ? _("above") : _("below"));

    if (traffic != nullptr && traffic->climb_rate_received &&
        std::fabs(traffic->climb_rate) >= MIN_CLIMB_RATE)
      line.AppendFormat(", %s", traffic->climb_rate > 0
                        ? _("climbing") : _("descending"));

    canvas.DrawText(p, line.c_str());
    p.y += int(large_font.GetHeight()) + padding;
  }

  canvas.Select(text_font);

  line = FormatUserDistanceSmart(status.relative_distance).c_str();
  if (status.has_proximity_time)
    line.AppendFormat(", %s %u s", _("close in"),
                      unsigned(std::lround(status.proximity_time)));
  canvas.DrawText(p, line.c_str());
  p.y += int(text_font.GetHeight()) + padding;

  const char *name = FlarmDetails::LookupCallsign(status.target_id);
  if (name == nullptr && traffic != nullptr && traffic->HasName())
    name = traffic->name.c_str();
  if (name != nullptr)
    canvas.DrawText(p, name);
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

    const auto [left, right] = content.VerticalSplit();
    PaintDirection(canvas, left, status, color);
    PaintText(canvas, right, status, color);
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
