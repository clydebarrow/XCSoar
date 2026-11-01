// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StartupDialog.hpp"
#include "ClubConfig.hpp"
#include "Error.hpp"
#include "Message.hpp"
#include "TextEntry.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/File.hpp"
#include "Form/Form.hpp"
#include "Gauge/LogoView.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Look/DialogLook.hpp"
#include "Profile/File.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Map.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "ProfileListDialog.hpp"
#include "ProfilePasswordDialog.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "WidgetDialog.hpp"
#include "system/FileUtil.hpp"
#include "ui/canvas/Canvas.hpp"
#include "util/StaticString.hxx"
#include "util/StringAPI.hxx"

class LogoWindow final : public PaintWindow {
  LogoView logo;
  bool dark_mode;

public:
  explicit LogoWindow(bool _dark_mode = false,
                     Color _background_color = COLOR_WHITE) noexcept
    :dark_mode(_dark_mode), background_color(_background_color) {}

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.Clear(background_color);
    logo.draw(canvas, GetClientRect(), dark_mode);
  }

private:
  Color background_color;
};

class LogoQuitWidget final : public NullWidget {
  const ButtonLook &look;
  WndForm &dialog;

  LogoWindow logo;
  Button quit;

public:
  LogoQuitWidget(const ButtonLook &_look, WndForm &_dialog,
                 bool dark_mode, Color background_color) noexcept
    :look(_look), dialog(_dialog), logo(dark_mode, background_color) {}

private:
  PixelRect GetButtonRect(PixelRect rc) noexcept {
    rc.left = rc.right - Layout::Scale(75);
    rc.bottom = rc.top + Layout::GetMaximumControlHeight();
    return rc;
  }

public:
  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override {
    return { 150, 150 };
  }

  PixelSize GetMaximumSize() const noexcept override {
    /* use as much as possible */
    return { 8192, 8192 };
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    WindowStyle button_style(style);
    button_style.Hide();
    button_style.TabStop();

    quit.Create(parent, look, _("Quit"), rc,
                button_style, dialog.MakeModalResultCallback(mrCancel));
    logo.Create(parent, rc, style);
  }

  void Show(const PixelRect &rc) noexcept override {
    quit.MoveAndShow(GetButtonRect(rc));
    logo.MoveAndShow(rc);
  }

  void Hide() noexcept override {
    quit.FastHide();
    logo.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    quit.Move(GetButtonRect(rc));
    logo.Move(rc);
  }
};

class StartupWidget final : public RowFormWidget {
  enum Controls {
    PROFILE,
    CONTINUE,
  };

  WndForm &dialog;
  DataField *const df;

public:
  StartupWidget(const DialogLook &look, WndForm &_dialog,
                DataField *_df) noexcept
    :RowFormWidget(look), dialog(_dialog), df(_df) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  bool SetFocus() noexcept override {
    /* focus the "Continue" button by default */
    GetRow(CONTINUE).SetFocus();
    return true;
  }
};

static bool
SelectProfileCallback([[maybe_unused]] const char *caption, [[maybe_unused]] DataField &_df,
                      [[maybe_unused]] const char *help_text) noexcept
{
  FileDataField &df = (FileDataField &)_df;

  const auto path = SelectProfileDialog(df.GetValue());
  if (path == nullptr)
    return false;

  df.ForceModify(path);
  return true;
}

void
StartupWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                       [[maybe_unused]] const PixelRect &rc) noexcept
{
  auto *pe = Add(_("Profile"), nullptr, df);
  pe->SetEditCallback(SelectProfileCallback);

  AddButton(_("Continue"), dialog.MakeModalResultCallback(mrOK));
}

static bool
SelectProfile(Path path) noexcept
{
  try {
    if (!CheckProfilePasswordResult(CheckProfileFilePassword(path)))
      return false;
  } catch (...) {
    ShowError(std::current_exception(), _("Password"));
    return false;
  }

  Profile::SetFiles(path);

  if (RelativePath(path) == nullptr)
    /* When a profile from a secondary data path is used, this path
       becomes the primary data path */
    SetPrimaryDataPath(path.GetParent());

  File::Touch(path);
  return true;
}

bool
StartupWidget::Save(bool &changed) noexcept
{
  const auto &dff = (const FileDataField &)GetDataField(PROFILE);
  if (!SelectProfile(dff.GetValue()))
    return false;

  changed = true;

  return true;
}

/**
 * The index of the most recently used profile.
 */
[[gnu::pure]]
static unsigned
MostRecentProfile(const FileDataField &profiles) noexcept
{
  unsigned best_index = 0;
  std::chrono::system_clock::time_point best_timestamp =
    std::chrono::system_clock::time_point::min();

  for (unsigned i = 0, n = profiles.size(); i < n; ++i) {
    const auto timestamp =
      File::GetLastModification(Path(profiles.GetItem(i).path));
    if (timestamp > best_timestamp) {
      best_timestamp = timestamp;
      best_index = i;
    }
  }

  return best_index;
}

/**
 * The pilots of a club glider, one profile each, in rows large
 * enough to tap.  Tapping one selects it.
 */
class PilotListWidget final : public ListWidget {
  const FileDataField &profiles;
  WndForm *form = nullptr;
  unsigned selected = 0;

public:
  explicit PilotListWidget(const FileDataField &_profiles) noexcept
    :profiles(_profiles) {}

  void SetForm(WndForm &_form) noexcept {
    form = &_form;
  }

  Path GetSelectedPath() const noexcept {
    return profiles.GetItem(selected).path;
  }

private:
  [[gnu::pure]]
  static unsigned GetRowHeight() noexcept {
    const Font &font = *UIGlobals::GetDialogLook().list.font_bold;
    return std::max(Layout::GetMaximumControlHeight(), 2 * font.GetHeight());
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    ListWidget::Prepare(parent, rc);
    auto &list = CreateList(parent, UIGlobals::GetDialogLook(), rc,
                            GetRowHeight());
    list.SetLength(profiles.size());
    if (profiles.size() > 0)
      list.SetCursorIndex(MostRecentProfile(profiles));
  }

  /* virtual methods from ListItemRenderer */
  unsigned OnListResized() noexcept override {
    return GetRowHeight();
  }

  void OnPaintItem(Canvas &canvas, PixelRect rc,
                   unsigned i) noexcept override {
    /* the file name without ".prf" */
    StaticString<64> name(profiles.GetItem(i).filename.c_str());
    if (const char *dot = StringFindLast(name.c_str(), '.'))
      name.Truncate(dot - name.c_str());

    const Font &font = *UIGlobals::GetDialogLook().list.font_bold;
    canvas.Select(font);
    canvas.DrawText({rc.left + int(Layout::GetTextPadding()) * 2,
                     rc.top + int(rc.GetHeight() - font.GetHeight()) / 2},
                    name);
  }

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem(unsigned) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    selected = index;
    form->SetModalResult(mrOK);
  }
};

/**
 * Ask for a pilot's name and create a profile for that pilot: a copy
 * of #template_path without its password and the last pilot's
 * personal settings, so the glider and device settings carry over.
 *
 * @return the new profile, or nullptr if none was created
 */
static AllocatedPath
CreatePilotProfile(Path template_path) noexcept
{
  StaticString<64> name;
  name.clear();
  if (!TextEntryDialog(name, _("Pilot name")) || name.empty())
    return nullptr;

  StaticString<80> filename;
  filename = name;
  filename += ".prf";

  auto path = LocalPath(AllocatedPath::Build(
    GetFileTypeDefaultDir(FileType::PROFILE), filename));
  if (File::ExistsAny(path)) {
    ShowMessageBox(name, _("File exists already."), MB_OK|MB_ICONEXCLAMATION);
    return nullptr;
  }

  ProfileMap data;
  if (template_path != nullptr) {
    try {
      Profile::LoadFile(data, template_path);
    } catch (...) {
      LogError(std::current_exception(), "Failed to load the last pilot's profile");
      data.Clear();
    }

    using namespace ProfileKeys;
    for (const auto key : {
        Password, CoPilotName, WeGlidePilotID, WeGlidePilotBirthDate,
        SkyLinesTrackingKey, CloudKey, LiveTrack24Username,
        LiveTrack24Password, PCMetUsername, PCMetPassword,
        PCMetFtpUsername, PCMetFtpPassword, SkySightEmail,
        SkySightPassword, LegacySkySightEmail, LegacySkySightPassword,
        XCThermEmail, XCThermPassword,
      })
      data.Remove(key);
  }

  data.Set(ProfileKeys::PilotName, name.c_str());

  try {
    if (const auto parent = path.GetParent(); parent != nullptr)
      Directory::CreateRecursive(parent);
    Profile::SaveFile(data, path);
  } catch (...) {
    ShowError(std::current_exception(), _("Failed to save file."));
    return nullptr;
  }

  return path;
}

/**
 * With a club configuration: always ask which pilot flies, and offer
 * to add one.
 */
static bool
dlgPilotShowModal() noexcept
{
  while (true) {
    FileDataField profiles;
    profiles.SetFileType(FileType::PROFILE);
    profiles.ScanDirectoryTop(GetFileTypePatterns(FileType::PROFILE));

    const Path last_pilot = profiles.size() > 0
      ? Path(profiles.GetItem(MostRecentProfile(profiles)).path)
      : Path(nullptr);

    TWidgetDialog<PilotListWidget>
      dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
             UIGlobals::GetDialogLook(), _("Who is flying?"));
    dialog.SetWidget(profiles);
    dialog.GetWidget().SetForm(dialog);

    AllocatedPath new_pilot;
    dialog.AddButton(_("New pilot"), [&](){
      new_pilot = CreatePilotProfile(last_pilot);
      if (new_pilot != nullptr)
        dialog.SetModalResult(mrOK);
    });
    dialog.AddButton(_("Quit"), mrCancel);

    if (dialog.ShowModal() != mrOK)
      return false;

    if (SelectProfile(new_pilot != nullptr
                        ? Path(new_pilot)
                        : dialog.GetWidget().GetSelectedPath()))
      return true;
  }
}

bool
dlgStartupShowModal() noexcept
{
  LogString("Startup dialog");

  if (GetClubConfig().IsDefined())
    return dlgPilotShowModal();

  /* scan all profile files */
  auto *dff = new FileDataField();
  dff->SetFileType(FileType::PROFILE);
  dff->ScanDirectoryTop(GetFileTypePatterns(FileType::PROFILE));

  if (dff->GetNumFiles() == 1) {
    /* skip this dialog if there is only one */
    const auto path = dff->GetValue();
    if (ProfileFileHasPassword(path) == TriState::FALSE &&
        SelectProfile(path)) {
      delete dff;
      return true;
    }
  } else if (dff->GetNumFiles() == 0) {
    /* no profile exists yet: create default profile */
    Profile::SetFiles(nullptr);
    return true;
  }

  /* preselect the most recently used profile */
  dff->SetIndex(MostRecentProfile(*dff));

  /* show the dialog */
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<TwoWidgets> dialog(WidgetDialog::Full{},
                                   UIGlobals::GetMainWindow(),
                                   UIGlobals::GetDialogLook(),
                                   nullptr);

  dialog.SetWidget(std::make_unique<LogoQuitWidget>(look.button, dialog,
                                                    look.dark_mode, look.background_color),
                   std::make_unique<StartupWidget>(look, dialog, dff));

  return dialog.ShowModal() == mrOK;
}
