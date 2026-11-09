// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * GDL90 serial device (Garmin ICD + optional ForeFlight 0x65 extensions).
 * Wire format and payload layouts: see Protocol.md in this directory.
 */

#include "Device/Driver.hpp"
#include <cstdint>
#include <span>
#include <vector>

struct NMEAInfo;

class GDL90Device final : public AbstractDevice {
public:
  struct GDL90Settings {
    uint16_t hrange;
    uint16_t vrange;
    /** Fill calendar date from host UTC after heartbeat TOD (see ParseHeartbeat). */
    bool use_system_utc_date;
  };

private:
  std::vector<uint8_t> buffer;
  /** Bytes between 0x7E flags (still stuffed); copy before mutating buffer. */
  std::vector<uint8_t> escaped_frame;
  std::vector<uint8_t> unescaped;

  /**
   * ForeFlight 0x65 ID message capability bit 0:
   * geometric altitude datum used in 0x0B.
   *
   * false = WGS-84 ellipsoid (per GDL90 ICD)
   * true  = MSL (per ForeFlight extension)
   */
  bool geo_altitude_is_msl = false;

  /**
   * The validity of the ownship position.  The heartbeat's "GPS
   * position valid" flag and a NIC of 0 count only once the sender has
   * shown that it uses them, by setting the flag or sending another NIC.
   */
  bool heartbeat_position_valid = false;
  bool heartbeat_position_flag_seen = false;
  bool nic_seen = false;
  bool ownship_position_valid = true;

  bool IsHeartbeatPositionValid() const noexcept {
    return heartbeat_position_valid || !heartbeat_position_flag_seen;
  }

  /** UTC seconds since midnight of the latest heartbeat, if any */
  unsigned heartbeat_seconds = 0;
  bool heartbeat_valid = false;

  /**
   * The ownship pressure altitude at #vario_seconds, for a vertical
   * speed when the ownship report has none.  The arrival times of the
   * reports are useless for this: Wi-Fi delivers them in bursts.
   */
  double vario_altitude = 0;
  unsigned vario_seconds = 0;
  bool vario_altitude_valid = false;
  double vario = 0;

  void ProvideOwnshipVario(uint16_t v_code, bool pressure_altitude_valid,
                           double pressure_altitude,
                           NMEAInfo &info) noexcept;

  void ParseTrafficReport(std::span<const uint8_t> payload,
                          NMEAInfo &info) noexcept;

  void ParseOwnshipReport(std::span<const uint8_t> payload,
                          NMEAInfo &info) noexcept;

  void ParseOwnshipGeometricAltitude(std::span<const uint8_t> payload,
                                     NMEAInfo &info) noexcept;

  void ParseHeartbeat(std::span<const uint8_t> payload,
                      NMEAInfo &info) noexcept;

  void ParseForeFlight(std::span<const uint8_t> payload,
                       NMEAInfo &info) noexcept;

public:
  bool DataReceived(std::span<const std::byte> s,
                    NMEAInfo &info) noexcept override;
};

extern GDL90Device::GDL90Settings gdl90_settings;

void LoadFromProfile(GDL90Device::GDL90Settings &settings) noexcept;

void SaveToProfile(GDL90Device::GDL90Settings &settings) noexcept;
