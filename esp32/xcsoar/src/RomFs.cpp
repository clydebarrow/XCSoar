// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * A read-only ESP-IDF VFS driver that serves files embedded in the
 * firmware, so code that opens files by path (FreeType, later the
 * profile and data loaders) works without a storage device.
 */

#include "XCSoarEsp32.hpp"

#include <esp_vfs.h>
#include <sdkconfig.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <mutex>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef CONFIG_VFS_SUPPORT_DIR
/* needed for stat(), which the font search uses */
#error XCSoar needs CONFIG_VFS_SUPPORT_DIR
#endif

namespace XCSoarEsp32 {

namespace {

struct OpenFile {
  const RomFile *file = nullptr;
  std::size_t position = 0;
};

struct RomFs {
  std::span<const RomFile> files;

  /* FreeType keeps a font file open for as long as the face is
     loaded, so allow for one per font */
  std::array<OpenFile, 32> open_files;
  std::mutex mutex;

  const RomFile *Find(const char *path) const noexcept {
    if (*path == '/')
      ++path;

    for (const auto &file : files)
      if (std::strcmp(file.name, path) == 0)
        return &file;

    return nullptr;
  }

  OpenFile *Get(int fd) noexcept {
    if (fd < 0 || std::size_t(fd) >= open_files.size() ||
        open_files[fd].file == nullptr) {
      errno = EBADF;
      return nullptr;
    }

    return &open_files[fd];
  }
};

} // anonymous namespace

static RomFs rom_fs;

static void
FillStat(const RomFile &file, struct stat *st) noexcept
{
  *st = {};
  st->st_mode = S_IFREG | 0444;
  st->st_size = file.data.size();
}

static int
RomOpen(void *ctx, const char *path, int flags, int) noexcept
{
  auto &fs = *static_cast<RomFs *>(ctx);

  if ((flags & O_ACCMODE) != O_RDONLY) {
    errno = EROFS;
    return -1;
  }

  const RomFile *file = fs.Find(path);
  if (file == nullptr) {
    errno = ENOENT;
    return -1;
  }

  const std::lock_guard lock{fs.mutex};
  for (std::size_t fd = 0; fd < fs.open_files.size(); ++fd) {
    if (fs.open_files[fd].file == nullptr) {
      fs.open_files[fd] = {file, 0};
      return fd;
    }
  }

  errno = ENFILE;
  return -1;
}

static ssize_t
RomRead(void *ctx, int fd, void *dest, size_t size) noexcept
{
  auto *f = static_cast<RomFs *>(ctx)->Get(fd);
  if (f == nullptr)
    return -1;

  const auto data = f->file->data;
  if (f->position >= data.size())
    return 0;

  size = std::min(size, data.size() - f->position);
  std::memcpy(dest, data.data() + f->position, size);
  f->position += size;
  return size;
}

static off_t
RomSeek(void *ctx, int fd, off_t offset, int whence) noexcept
{
  auto *f = static_cast<RomFs *>(ctx)->Get(fd);
  if (f == nullptr)
    return -1;

  off_t base;
  switch (whence) {
  case SEEK_SET:
    base = 0;
    break;

  case SEEK_CUR:
    base = f->position;
    break;

  case SEEK_END:
    base = f->file->data.size();
    break;

  default:
    errno = EINVAL;
    return -1;
  }

  if (base + offset < 0) {
    errno = EINVAL;
    return -1;
  }

  f->position = base + offset;
  return f->position;
}

static int
RomClose(void *ctx, int fd) noexcept
{
  auto &fs = *static_cast<RomFs *>(ctx);
  auto *f = fs.Get(fd);
  if (f == nullptr)
    return -1;

  const std::lock_guard lock{fs.mutex};
  *f = {};
  return 0;
}

static int
RomFstat(void *ctx, int fd, struct stat *st) noexcept
{
  auto *f = static_cast<RomFs *>(ctx)->Get(fd);
  if (f == nullptr)
    return -1;

  FillStat(*f->file, st);
  return 0;
}

static int
RomStat(void *ctx, const char *path, struct stat *st) noexcept
{
  const RomFile *file = static_cast<RomFs *>(ctx)->Find(path);
  if (file == nullptr) {
    errno = ENOENT;
    return -1;
  }

  FillStat(*file, st);
  return 0;
}

bool
MountRomFs(std::span<const RomFile> files) noexcept
{
  rom_fs.files = files;

  esp_vfs_t vfs{};
  vfs.flags = ESP_VFS_FLAG_CONTEXT_PTR;
  vfs.open_p = RomOpen;
  vfs.read_p = RomRead;
  vfs.lseek_p = RomSeek;
  vfs.close_p = RomClose;
  vfs.fstat_p = RomFstat;
  vfs.stat_p = RomStat;

  const esp_err_t err = esp_vfs_register(ROM_MOUNT_POINT, &vfs, &rom_fs);
  if (err != ESP_OK) {
    fprintf(stderr, "XCSoar: failed to mount %s: %s\n",
            ROM_MOUNT_POINT, esp_err_to_name(err));
    return false;
  }

  return true;
}

} // namespace XCSoarEsp32
