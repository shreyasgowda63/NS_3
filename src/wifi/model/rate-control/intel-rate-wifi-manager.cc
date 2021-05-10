/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright(c) 2005 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016 - 2017 Intel Deutschland GmbH
 * Copyright(c) 2018 - 2019 Intel Corporation
 * Copyright(c) 2019 - Rémy Grünblatt <remy@grunblatt.org>
 * Copyright(c) 2021 - Alexander Krotov <krotov@iitp.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Rémy Grünblatt <remy@grunblatt.org>
 *          Alexander Krotov <krotov@iitp.ru>
 */
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "intel-rate-wifi-manager.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-utils.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-phy.h"

#include <iomanip>
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
#include <utility>
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IntelWifiManager");
NS_OBJECT_ENSURE_REGISTERED(IntelWifiManager);

#define INVALID_THROUGHPUT                     -1
#define INVALID_INDEX                          -1

#define IWL_MVM_RS_RATE_MIN_FAILURE_TH          3
#define IWL_MVM_RS_RATE_MIN_SUCCESS_TH          8
#define IWL_MVM_RS_SR_FORCE_DECREASE           15
#define IWL_MVM_RS_SR_NO_DECREASE              85
#define IWL_MVM_RS_STAY_IN_COLUMN_TIMEOUT       5

#define IWL_MVM_RS_LEGACY_FAILURE_LIMIT       160
#define IWL_MVM_RS_LEGACY_SUCCESS_LIMIT       480
#define IWL_MVM_RS_LEGACY_TABLE_COUNT         160
#define IWL_MVM_RS_NON_LEGACY_FAILURE_LIMIT   400
#define IWL_MVM_RS_NON_LEGACY_SUCCESS_LIMIT  4500
#define IWL_MVM_RS_NON_LEGACY_TABLE_COUNT    1500

#define RS_PERCENT(x) (128 * x)

/* Right now, the algorithm only supports up to 3 antennas. It's more a limit of
   the Intel hardware, and might be extended in the future to support more
   antennas */
enum IntelWifiAntenna
{
  A,
  B,
  C
};

/* Intel can either use a LEGACY transmission mode (802.11a or 802.11g), or a
   non-legacy transmission mode (SISO if you have one spatial stream, or MIMO if
   you have multiple spatial steams). The intel driver only supports 2 spatial
   streams */
enum class ColumnMode
{
  LEGACY,
  SISO,
  MIMO
};

std::ostream&
operator<<(std::ostream &out, ColumnMode &mode)
{
  switch (mode) {
  case ColumnMode::LEGACY:
    out << "LEGACY";
    break;
  case ColumnMode::SISO:
    out << "SISO";
    break;
  case ColumnMode::MIMO:
    out << "MIMO";
    break;
  default:
    out << "UNKNOWN";
  }
  return out;
}

enum RateType
{
  NONE,
  LEGACY_G
};

/// Guard Interval Duration
enum class GuardInterval
{
  SGI,
  LGI
};

/* MCS Scaling actions (decreasing the MCS index, maintaining the MCS index, or
   increasing the MCS index). */
enum class RsAction
{
  STAY,
  DOWNSCALE,
  UPSCALE
};

/// Bandwidth
enum Bandwidth
{
  BW_20 = 20,
  BW_40 = 40,
  BW_80 = 80,
  BW_160 = 160
};

/* Whether AMPDU aggregation is enabled or not */
enum class Aggregation
{
  NO_AGG,
  AGG
};

enum class RsState
{
  SEARCH_CYCLE_STARTED,
  SEARCH_CYCLE_ENDED,
  STAY_IN_COLUMN,
};

std::ostream&
operator<<(std::ostream &out, RsState &state)
{
  switch (state) {
  case RsState::SEARCH_CYCLE_STARTED:
    out << "SEARCH_CYCLE_STARTED";
    break;
  case RsState::SEARCH_CYCLE_ENDED:
    out << "SEARCH_CYCLE_ENDED";
    break;
  case RsState::STAY_IN_COLUMN:
    out << "STAY_IN_COLUMN";
    break;
  default:
    out << "UNKNOWN";
  }
  return out;
}

/// Theoretical throughput for each MCS. Extracted from the driver
/// source code.
///
/// Each table encodes the maximum theoretical throughput for each
/// combination of column mode (LEGACY, SISO, MIMO), Bandwidth (20MHz,
/// 40Mhz, 80Mhz, 160Mhz), guard interval duration (Long or Short),
/// and AMPDU aggregation.
static const std::map<std::tuple<ColumnMode, Bandwidth, GuardInterval, Aggregation>, const std::array<int, 15>>
g_theoreticalThroughputTables{
  // expected_tpt_LEGACY
  {{ColumnMode::LEGACY, BW_20, GuardInterval::LGI, Aggregation::NO_AGG}, {7, 13, 35, 58, 40, 57, 72, 98, 121, 154, 177, 186, 0, 0, 0}},

  // expected_tpt_SISO_20MHz
  {{ColumnMode::SISO, BW_20, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 42, 0, 76, 102, 124, 159, 183, 193, 202, 216, 0}},
  {{ColumnMode::SISO, BW_20, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 46, 0, 82, 110, 132, 168, 192, 202, 210, 225, 0}},
  {{ColumnMode::SISO, BW_20, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 49, 0, 97, 145, 192, 285, 375, 420, 464, 551, 0}},
  {{ColumnMode::SISO, BW_20, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 54, 0, 108, 160, 213, 315, 415, 465, 513, 608, 0}},

  // expected_tpt_SISO_40MHz
  {{ColumnMode::SISO, BW_40, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 77, 0, 127, 160, 184, 220, 242, 250, 257, 269, 275}},
  {{ColumnMode::SISO, BW_40, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 83, 0, 135, 169, 193, 229, 250, 257, 264, 275, 280}},
  {{ColumnMode::SISO, BW_40, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 101, 0, 199, 295, 389, 570, 744, 828, 911, 1070, 1173}},
  {{ColumnMode::SISO, BW_40, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 112, 0, 220, 326, 429, 629, 819, 912, 1000, 1173, 1284}},

  // expected_tpt_SISO_80MHz
  {{ColumnMode::SISO, BW_80, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 130, 0, 191, 223, 244, 273, 288, 294, 298, 305, 308}},
  {{ColumnMode::SISO, BW_80, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 138, 0, 200, 231, 251, 279, 293, 298, 302, 308, 312}},
  {{ColumnMode::SISO, BW_80, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 217, 0, 429, 634, 834, 1220, 1585, 1760, 1931, 2258, 2466}},
  {{ColumnMode::SISO, BW_80, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 241, 0, 475, 701, 921, 1343, 1741, 1931, 2117, 2468, 2691}},

  // expected_tpt_SISO_160MHz
  {{ColumnMode::SISO, BW_160, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 191, 0, 244, 288, 298, 308, 313, 318, 323, 328, 330}},
  {{ColumnMode::SISO, BW_160, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 200, 0, 251, 293, 302, 312, 317, 322, 327, 332, 334}},
  {{ColumnMode::SISO, BW_160, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 439, 0, 875, 1307, 1736, 2584, 3419, 3831, 4240, 5049, 5581}},
  {{ColumnMode::SISO, BW_160, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 488, 0, 972, 1451, 1925, 2864, 3785, 4240, 4691, 5581, 6165}},

  // expected_tpt_MIMO2_20MHz
  {{ColumnMode::MIMO, BW_20, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 74, 0, 123, 155, 179, 213, 235, 243, 250, 261, 0}},
  {{ColumnMode::MIMO, BW_20, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 81, 0, 131, 164, 187, 221, 242, 250, 256, 267, 0}},
  {{ColumnMode::MIMO, BW_20, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 98, 0, 193, 286, 375, 550, 718, 799, 878, 1032, 0}},
  {{ColumnMode::MIMO, BW_20, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 109, 0, 214, 316, 414, 607, 790, 879, 965, 1132, 0}},

  // expected_tpt_MIMO2_40MHz
  {{ColumnMode::MIMO, BW_40, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 123, 0, 182, 214, 235, 264, 279, 285, 289, 296, 300}},
  {{ColumnMode::MIMO, BW_40, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 131, 0, 191, 222, 242, 270, 284, 289, 293, 300, 303}},
  {{ColumnMode::MIMO, BW_40, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 200, 0, 390, 571, 741, 1067, 1365, 1505, 1640, 1894, 2053}},
  {{ColumnMode::MIMO, BW_40, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 221, 0, 430, 630, 816, 1169, 1490, 1641, 1784, 2053, 2221}},

  // expected_tpt_MIMO2_80MHz
  {{ColumnMode::MIMO, BW_80, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 182, 0, 240, 264, 278, 299, 308, 311, 313, 317, 319}},
  {{ColumnMode::MIMO, BW_80, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 190, 0, 247, 269, 282, 302, 310, 313, 315, 319, 320}},
  {{ColumnMode::MIMO, BW_80, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 428, 0, 833, 1215, 1577, 2254, 2863, 3147, 3418, 3913, 4219}},
  {{ColumnMode::MIMO, BW_80, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 474, 0, 920, 1338, 1732, 2464, 3116, 3418, 3705, 4225, 4545}},

  // expected_tpt_MIMO2_160MHz
  {{ColumnMode::MIMO, BW_160, GuardInterval::LGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 240, 0, 278, 308, 313, 319, 322, 324, 328, 330, 334}},
  {{ColumnMode::MIMO, BW_160, GuardInterval::SGI, Aggregation::NO_AGG}, {0, 0, 0, 0, 247, 0, 282, 310, 315, 320, 323, 325, 329, 332, 338}},
  {{ColumnMode::MIMO, BW_160, GuardInterval::LGI, Aggregation::AGG},    {0, 0, 0, 0, 875, 0, 1735, 2582, 3414, 5043, 6619, 7389, 8147, 9629, 10592}},
  {{ColumnMode::MIMO, BW_160, GuardInterval::SGI, Aggregation::AGG},    {0, 0, 0, 0, 971, 0, 1925, 2861, 3779, 5574, 7304, 8147, 8976, 10592, 11640}},
};

class Column {
public:
  Column () = delete;

  /// Constructor.
  Column (ColumnMode mode, std::set<IntelWifiAntenna> antennas, GuardInterval gi);

  void SetNextColumns (std::vector<Column> columns);

  std::vector<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> &GetNextColumns ();

private:
  ColumnMode m_mode;
  std::set<IntelWifiAntenna> m_antennas;
  GuardInterval m_gi;

  std::vector<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> m_nextColumns;

  /// Represents the column in a format suitable for storage in m_nextColumns.
  std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval> GetColumn ();
};

Column::Column (ColumnMode mode, std::set<IntelWifiAntenna> antennas, GuardInterval gi)
  : m_mode{mode},
    m_antennas{antennas},
    m_gi{gi}
{
}

std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>
Column::GetColumn ()
{
  return {m_mode, m_antennas, m_gi};
}

void
Column::SetNextColumns (std::vector<Column> columns)
{
  for (Column col: columns)
    {
      m_nextColumns.push_back (col.GetColumn ());
    }
}

std::vector<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> &
Column::GetNextColumns () {
  return m_nextColumns;
}

static
std::map<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>, Column>
BuildColumns() {
  Column LEGACY_ANT_A{ColumnMode::LEGACY, {A}, GuardInterval::LGI};
  Column LEGACY_ANT_B{ColumnMode::LEGACY, {B}, GuardInterval::LGI};
  Column SISO_ANT_A{ColumnMode::SISO, {A}, GuardInterval::LGI};
  Column SISO_ANT_B{ColumnMode::SISO, {B}, GuardInterval::LGI};
  Column SISO_ANT_A_SGI{ColumnMode::SISO, {A}, GuardInterval::SGI};
  Column SISO_ANT_B_SGI{ColumnMode::SISO, {B}, GuardInterval::SGI};
  Column MIMO2{ColumnMode::MIMO, {A, B}, GuardInterval::LGI};
  Column MIMO2_SGI{ColumnMode::MIMO, {A, B}, GuardInterval::SGI};
  LEGACY_ANT_A.SetNextColumns({LEGACY_ANT_B, SISO_ANT_A, MIMO2});
  LEGACY_ANT_B.SetNextColumns({LEGACY_ANT_A, SISO_ANT_B, MIMO2});
  SISO_ANT_A.SetNextColumns({SISO_ANT_B, MIMO2, SISO_ANT_A_SGI, LEGACY_ANT_A, LEGACY_ANT_B});
  SISO_ANT_B.SetNextColumns({SISO_ANT_A, MIMO2, SISO_ANT_B_SGI, LEGACY_ANT_A, LEGACY_ANT_B});
  SISO_ANT_A_SGI.SetNextColumns({SISO_ANT_B_SGI, MIMO2_SGI, SISO_ANT_A, LEGACY_ANT_A, LEGACY_ANT_B});
  SISO_ANT_B_SGI.SetNextColumns({SISO_ANT_A_SGI, MIMO2_SGI, SISO_ANT_B, LEGACY_ANT_A, LEGACY_ANT_B});
  MIMO2.SetNextColumns({SISO_ANT_A, MIMO2_SGI, LEGACY_ANT_A, LEGACY_ANT_B});
  MIMO2_SGI.SetNextColumns({SISO_ANT_A_SGI, MIMO2, LEGACY_ANT_A, LEGACY_ANT_B});

  return {
    {{ColumnMode::LEGACY, {A}, GuardInterval::LGI}, LEGACY_ANT_A},
    {{ColumnMode::LEGACY, {B}, GuardInterval::LGI}, LEGACY_ANT_B},
    {{ColumnMode::SISO, {A}, GuardInterval::LGI}, SISO_ANT_A},
    {{ColumnMode::SISO, {B}, GuardInterval::LGI}, SISO_ANT_B},
    {{ColumnMode::SISO, {A}, GuardInterval::SGI}, SISO_ANT_A_SGI},
    {{ColumnMode::SISO, {B}, GuardInterval::SGI}, SISO_ANT_B_SGI},
    {{ColumnMode::MIMO, {A, B}, GuardInterval::LGI}, MIMO2},
    {{ColumnMode::MIMO, {A, B}, GuardInterval::SGI}, MIMO2_SGI}
  };
}

static auto g_columns = BuildColumns ();

class History {
  /// History of transmissions, true for success, false for failure.
  std::vector<bool> m_data;

  /// Maximum throughput.
  int m_maxThroughput = INVALID_THROUGHPUT;

public:
  /// Constructor.
  History (int maxThroughput);

  /// Default constructor.
  History ();

  /// Returns maximum throughput.
  int GetMaxThroughput ();

  /// Sets maximum throughput.
  void SetMaxThroughput (int maxThroughput);

  /// Resets history.
  void Reset ();

  /// Returns history size.
  int Counter ();

  /// Returns average throughput.
  int AverageThroughput ();

  /// Returns the number of successful transmissions.
  int SuccessCounter ();

  /// Returns the number of failed transmissions.
  int FailCounter ();

  /// Record transmission result.
  void Tx (bool success);

  /// Returns success ratio.
  int SuccessRatio ();
};


History::History(int maxThroughput)
  : m_maxThroughput {maxThroughput}
{
}

History::History ()
{
}

int
History::GetMaxThroughput ()
{
  return m_maxThroughput;
}

void
History::SetMaxThroughput (int maxThroughput)
{
  m_maxThroughput = maxThroughput;
}

void
History::Reset ()
{
  m_data.clear ();
}

int
History::Counter ()
{
  return m_data.size ();
}

int
History::AverageThroughput ()
{
  if (FailCounter () >= IWL_MVM_RS_RATE_MIN_FAILURE_TH ||
      SuccessCounter () >= IWL_MVM_RS_RATE_MIN_SUCCESS_TH)
    {
      return (SuccessRatio () * GetMaxThroughput () + 64) / 128;
    }
  else
    {
      return INVALID_THROUGHPUT;
    }
}

int
History::SuccessCounter ()
{
  if (m_data.empty ())
    {
      return INVALID_THROUGHPUT;
    }
  return std::count (m_data.begin(), m_data.end(), true);
}

int
History::FailCounter ()
{
  if (m_data.empty ())
    {
      return INVALID_THROUGHPUT;
    }
  return std::count (m_data.begin (), m_data.end (), false);
}

void
History::Tx (bool success)
{
  m_data.emplace (m_data.begin (), success);
  if (m_data.size () > 62)
    {
      m_data.resize (62);
    }
}

int
History::SuccessRatio ()
{
  if (Counter () > 0)
    {
      return 128 * 100 * SuccessCounter () / Counter ();
    }
  else
    {
      return -1;
    }
}

class State {
public:
  bool m_columnScaling = false;
  int m_lastThroughput = 0;
  int m_index = 0;
  ColumnMode m_mode = ColumnMode::LEGACY;
  RateType m_type = LEGACY_G;
  Bandwidth m_bandwidth = BW_20;
  Bandwidth m_maxWidth = BW_20;
  GuardInterval m_guardInterval = GuardInterval::LGI;
  Aggregation m_agg = Aggregation::NO_AGG;
  std::set<IntelWifiAntenna> m_antennas = {A};
  RsState m_s = RsState::SEARCH_CYCLE_STARTED;

  int m_totalFailed = 0;
  int m_totalSuccess = 0;
  int m_tableCount = 0;

  /// Timer in nanoseconds.
  int64_t m_flushTimer = 0;

  /// Columns already visted during the search cycle. Should never be
  /// empty, as the current column is being visited.
  std::set<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> m_visitedColumns;

  /// When trying a new column, this field stores the parameters of
  /// the old column so that in case the new column is not so good, we
  /// can go back to the old column.
  std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval, int, Bandwidth> m_oldColumnParameters;

  /// History for each rate for each parameters. In the original
  /// driver, only the history of the current column and the search
  /// column are saved, we emulate this behaviour by emptying these
  /// histories but it's easier to maintain a map than switching
  /// between two tables all the time.
  std::map<std::tuple<ColumnMode, Bandwidth, GuardInterval, Aggregation, int>, History> m_histories;

  /// Constructor.
  ///
  /// \param maxWidth Maximum width supported by the STA.
  State (int maxWidth);

  History &GetHistory ();

  History &GetHistory (int index);

  void ClearHistories ();

  Column &GetColumn();

  int GetMaxSuccessLimit () const;

  int GetMaxFailureLimit () const;

  int GetTableCountLimit() const;

  void SetStayInTable ();

  void StayInTable ();

  std::tuple<int, int> GetAdjacentRatesIndexes ();

  /// Returns a pair of a boolean and the result, if it is found.
  ///
  /// Boolean if false if no next column is found.
  std::tuple<bool, std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> GetNextColumn (std::set<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> visited_columns);

  int GetNextIndex (std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval> newColumnParameters);

  std::tuple<ns3::WifiMode, int, int, int, int, int, bool> GetTxVector (bool ht, bool vht);

  void Tx (int success, int failed, bool ampdu);

private:
  RsAction McsScaling (std::tuple<int, int> adjacentIndexes, std::tuple<int, int> adjacentRates);

  void RateScaling ();
};

State::State (int maxWidth)
{
  for (auto const& theoreticalThroughputTable: g_theoreticalThroughputTables)
    {
      for (uint i = 0; i < theoreticalThroughputTable.second.size (); ++i)
        {
          m_histories[std::tuple_cat (theoreticalThroughputTable.first, std::make_tuple (i))] = History (theoreticalThroughputTable.second[i]);
        }
    }
  m_visitedColumns.insert({ColumnMode::LEGACY, {A}, GuardInterval::LGI});
  if (maxWidth == 20)
    {
      m_maxWidth = BW_20;
    }
  else if (maxWidth == 40)
    {
      m_maxWidth = BW_40;
    }
  else if (maxWidth == 80)
    {
      m_maxWidth = BW_80;
    }
  else
    {
      m_maxWidth = BW_160;
    }
}

History &
State::GetHistory ()
{
  return m_histories[{m_mode, m_bandwidth, m_guardInterval, m_agg, m_index}];
}

History &
State::GetHistory (int index)
{
  return m_histories[{m_mode, m_bandwidth, m_guardInterval, m_agg, index}];
}

void
State::ClearHistories ()
{
  for (int i = 0; i < 15; ++i)
    {
      GetHistory (i).Reset ();
    }
}

Column &
State::GetColumn ()
{
  return g_columns.at ({m_mode, m_antennas, m_guardInterval});
}

int
State::GetMaxSuccessLimit () const
{
  if (m_mode == ColumnMode::LEGACY)
    {
      return IWL_MVM_RS_LEGACY_SUCCESS_LIMIT;
    }
  else
    {
      return IWL_MVM_RS_NON_LEGACY_SUCCESS_LIMIT;
    }
}

int
State::GetMaxFailureLimit () const
{
  if (m_mode == ColumnMode::LEGACY)
    {
      return IWL_MVM_RS_LEGACY_FAILURE_LIMIT;
    }
  else
    {
      return IWL_MVM_RS_NON_LEGACY_FAILURE_LIMIT;
    }
}

int
State::GetTableCountLimit () const
{
  if (m_mode == ColumnMode::LEGACY)
    {
      return IWL_MVM_RS_LEGACY_TABLE_COUNT;
    }
  else
    {
      return IWL_MVM_RS_NON_LEGACY_TABLE_COUNT;
    }
}

void
State::SetStayInTable ()
{
  NS_LOG_DEBUG ("Moving to RsState::STAY_IN_COLUMN");
  m_s = RsState::STAY_IN_COLUMN;
  m_totalFailed = 0;
  m_totalSuccess = 0;
  m_tableCount = 0;
  m_flushTimer = Simulator::Now().GetNanoSeconds();
  m_visitedColumns = {std::make_tuple (m_mode, m_antennas, m_guardInterval)};
}

void
State::StayInTable ()
{
  if (m_s == RsState::STAY_IN_COLUMN)
    {
      bool flush_interval_passed = false;
      if (m_flushTimer)
        {
          flush_interval_passed = ((Simulator::Now ().GetNanoSeconds() - m_flushTimer) >= (5000000000*IWL_MVM_RS_STAY_IN_COLUMN_TIMEOUT));
        }

      if (m_totalFailed > GetMaxFailureLimit () ||
          m_totalSuccess > GetMaxSuccessLimit () ||
          (!m_columnScaling && flush_interval_passed))
        {
          NS_LOG_DEBUG ("LQ: stay is expired " << (m_totalFailed > GetMaxFailureLimit ()) << " " <<
                        (m_totalSuccess > GetMaxSuccessLimit()) << " " << !m_columnScaling << " " << flush_interval_passed);
          m_s = RsState::SEARCH_CYCLE_STARTED;
          m_totalFailed = 0;
          m_totalSuccess = 0;
          m_tableCount = 0;
          m_flushTimer = 0;
          m_visitedColumns = {std::make_tuple (m_mode,
                                               m_antennas,
                                               m_guardInterval)};
        }
      else
        {
          m_tableCount++;
          if (m_tableCount > GetTableCountLimit ())
            {
              m_tableCount = 0;
              NS_LOG_DEBUG ("LQ: stay in table. Clear the histories.");
              ClearHistories ();
            }
        }
    }
}

std::tuple<int, int>
State::GetAdjacentRatesIndexes ()
{
  int maxIndex = 14;
  if (m_bandwidth == 20 && m_mode != ColumnMode::LEGACY)
    {
      maxIndex = 13;
    }

  std::tuple<int, int> indexes = {INVALID_INDEX, INVALID_INDEX};
  if (m_type != LEGACY_G)
    {
      for (int i = m_index - 1; i > INVALID_INDEX; --i)
        {
          if (GetHistory (i).GetMaxThroughput () != 0)
            {
              std::get<0> (indexes) = i;
              break;
            }
        }
      for (int i = m_index + 1; i <= maxIndex; ++i)
        {
          if (GetHistory (i).GetMaxThroughput () != 0)
            {
              std::get<1> (indexes) = i;
              break;
            }
        }
    }
  else
    {
      std::vector<std::tuple<int, int>> legacy_g_mapping =
        {
          {-1, 1},
          {0, 2},
          {1, 3},
          {5, 6},
          {2, 3},
          {4, 3},
          {3, 7},
          {6, 8},
          {7, 9},
          {8, 10},
          {9, 11},
          {10, -1}
        };
      NS_LOG_DEBUG ("m_index=" << m_index);
      return legacy_g_mapping.at (m_index);
    }
  return indexes;
}

RsAction
State::McsScaling (std::tuple<int, int> adjacentIndexes, std::tuple<int, int> adjacentRates)
{
  RsAction action = RsAction::STAY;

  if (GetHistory ().SuccessRatio () <= RS_PERCENT (IWL_MVM_RS_SR_FORCE_DECREASE) ||
      GetHistory ().AverageThroughput () == 0)
    {
      NS_LOG_DEBUG ("Decrease rate because of low SR");
      return RsAction::DOWNSCALE;
    }

  if (std::get<0> (adjacentRates) == INVALID_THROUGHPUT &&
      std::get<1> (adjacentRates) == INVALID_THROUGHPUT &&
      std::get<1> (adjacentIndexes) != INVALID_INDEX)
    {
      NS_LOG_DEBUG ("No data about high/low rates. Increase rate");
      return RsAction::UPSCALE;
    }

  if (std::get<1> (adjacentRates) == INVALID_THROUGHPUT &&
      std::get<1> (adjacentIndexes) != INVALID_INDEX &&
      std::get<0> (adjacentRates) != INVALID_THROUGHPUT &&
      std::get<0> (adjacentRates) < GetHistory ().AverageThroughput ())
    {
      NS_LOG_DEBUG ("No data about high rate and low rate is worse. Increase rate");
      return RsAction::UPSCALE;
    }

  if (std::get<1> (adjacentRates) != INVALID_THROUGHPUT &&
      std::get<1> (adjacentRates) > GetHistory ().AverageThroughput ())
    {
      NS_LOG_DEBUG ("Higher rate is better. Increate rate");
      return RsAction::UPSCALE;
    }

  if (std::get<0> (adjacentRates) != INVALID_THROUGHPUT &&
      std::get<1> (adjacentRates) != INVALID_THROUGHPUT &&
      std::get<0> (adjacentRates) < GetHistory ().AverageThroughput () &&
      std::get<1> (adjacentRates) < GetHistory ().AverageThroughput ())
    {
      NS_LOG_DEBUG ("Both high and low are worse. Maintain rate");
      return RsAction::STAY;
    }

  if (std::get<0> (adjacentRates) != INVALID_THROUGHPUT &&
      std::get<0> (adjacentRates) > GetHistory ().AverageThroughput ())
    {
      NS_LOG_DEBUG ("Lower rate is better");
      action = RsAction::DOWNSCALE;
    }
  else if (std::get<0> (adjacentRates) == INVALID_THROUGHPUT &&
           std::get<0> (adjacentIndexes) != INVALID_INDEX)
    {
      NS_LOG_DEBUG ("No data about lower rate");
      action = RsAction::DOWNSCALE;
    }
  else
    {
      NS_LOG_DEBUG ("Maintain rate");
    }

  if (action == RsAction::DOWNSCALE && std::get<0> (adjacentIndexes) != INVALID_INDEX)
    {
      if (GetHistory ().SuccessRatio () >= RS_PERCENT(IWL_MVM_RS_SR_NO_DECREASE))
        {
          NS_LOG_DEBUG ("SR is above NO DECREASE. Avoid downscale");
          action = RsAction::STAY;
        }
      else if (GetHistory ().AverageThroughput () > 100 * GetHistory (std::get<0> (adjacentIndexes)).GetMaxThroughput ())
        {
          NS_LOG_DEBUG ("Current TPT is higher than max expected in low rate. Avoid downscale");
          action = RsAction::STAY;
        }
      else
        {
          NS_LOG_DEBUG ("Decrease rate");
        }
    }
  return action;
}

std::tuple<bool, std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>>
State::GetNextColumn (std::set<std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval>> visited_columns)
{
  NS_LOG_DEBUG ("Visited columns: " << visited_columns.size ());
  for (auto key: GetColumn ().GetNextColumns ())
    {
      if (visited_columns.find (key) == visited_columns.end ())
        {
          Bandwidth bandwidth = m_bandwidth;
          if (std::get<0> (key) == ColumnMode::LEGACY)
            {
              bandwidth = BW_20;
            }
          // Check Throughput can be beaten
          const auto &throughputs = g_theoreticalThroughputTables.at ({std::get<0> (key), bandwidth, std::get<2> (key), m_agg});
          int max_expected_tpt = *std::max_element (throughputs.begin (), throughputs.end ());

          if (100 * max_expected_tpt <= GetHistory ().AverageThroughput ())
            {
              NS_LOG_DEBUG ("Skip column: can't beat current TPT. Max expected " << (max_expected_tpt*100) << " current " << GetHistory ().AverageThroughput ());
              continue;
            }

          return {true, key};
        }
    }
  return {false, {ColumnMode::LEGACY, {A}, GuardInterval::LGI}};
}

int
State::GetNextIndex (std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval> newColumnParameters)
{
  int throughputThreshold = INVALID_THROUGHPUT;
  int newIndex = INVALID_INDEX;
  if (GetHistory().SuccessRatio () >= RS_PERCENT(IWL_MVM_RS_SR_NO_DECREASE))
    {
      throughputThreshold = GetHistory().GetMaxThroughput()*100;
      NS_LOG_DEBUG ("SR " << GetHistory().SuccessRatio () << " high. Find rate exceeding EXPECTED_CURRENT " << throughputThreshold);
    }
  else
    {
      throughputThreshold = GetHistory ().AverageThroughput();
      NS_LOG_DEBUG ("SR " << GetHistory().SuccessRatio () << " low. Find rate exceeding ACTUAL_TPT " << throughputThreshold);
    }
  const auto &newThroughputs = g_theoreticalThroughputTables.at ({std::get<0> (newColumnParameters), m_bandwidth, std::get<2> (newColumnParameters), m_agg});
  for (uint i = 0; i < newThroughputs.size (); i++)
    {
      if (newThroughputs.at (i) != 0)
        {
          newIndex = i;
        }
      if (newThroughputs.at (i) * 100 > throughputThreshold)
        {
          NS_LOG_DEBUG ("Found " << i << " " << newThroughputs.at (i) << "  > " << throughputThreshold);
          break;
        }
    }
  if (newIndex == INVALID_INDEX)
    {
      NS_LOG_DEBUG ("WARNING: INVALID INDEX");
    }
  return newIndex;
}

void
State::RateScaling ()
{
  bool update_lq = false, done_search = false;
  int index = m_index;

  // If we don't have enough data, keep gathering statistics.
  if (GetHistory ().AverageThroughput () == INVALID_THROUGHPUT)
    {
      NS_LOG_DEBUG ("Test Window " << m_index << " : succ " << GetHistory ().SuccessCounter () << " total " << GetHistory ().Counter ());
      StayInTable ();
      return;
    }

  if (m_columnScaling)
    {
      // If we are searching for a better column, check if it's working.
      if (GetHistory ().AverageThroughput () > m_lastThroughput)
        {
          NS_LOG_DEBUG ("SWITCHING TO NEW TABLE SR :" << GetHistory ().SuccessRatio () << " cur-tpt " << GetHistory ().AverageThroughput () << " old-tpt " << m_lastThroughput);
        }
      else
        {
          NS_LOG_DEBUG ("GOING BACK TO THE OLD TABLE: SR: " << GetHistory ().SuccessRatio () << " cur-tpt " << GetHistory ().AverageThroughput () << " old-tpt " << m_lastThroughput);
          m_mode = std::get<0> (m_oldColumnParameters);
          m_antennas = std::get<1> (m_oldColumnParameters);
          m_guardInterval = std::get<2> (m_oldColumnParameters);
          m_bandwidth = std::get<4> (m_oldColumnParameters);
          NS_LOG_DEBUG ("Old index " << index);
          index = std::get<3> (m_oldColumnParameters);

          if (m_mode == ColumnMode::LEGACY)
            {
              m_type = LEGACY_G;
            }
          else
            {
              m_type = NONE;
            }

          update_lq = true;
        }
      m_columnScaling = false;
      done_search = true;
    }
  else
    {
      /* Else, we do MCS Scaling */
      std::tuple<int, int> adjacentIndexes = this->GetAdjacentRatesIndexes();
      std::tuple<int, int> adjacentRates = {INVALID_THROUGHPUT, INVALID_THROUGHPUT};
      if (std::get<0> (adjacentIndexes) != INVALID_INDEX)
        {
          std::get<0> (adjacentRates) = GetHistory (std::get<0> (adjacentIndexes)).AverageThroughput ();
        }
      if (std::get<1> (adjacentIndexes) != INVALID_INDEX)
        {
          std::get<1> (adjacentRates) = GetHistory (std::get<1> (adjacentIndexes)).AverageThroughput ();
        }
      NS_LOG_DEBUG ("cur_tpt " << GetHistory ().AverageThroughput () << " SR " << GetHistory ().SuccessRatio () << " low " << std::get<0> (adjacentIndexes) << " high " << std::get<1> (adjacentIndexes) << " low_tpt " << std::get<0> (adjacentRates) << " high_tpt " << std::get<1> (adjacentRates));
      RsAction scale_action = this->McsScaling (adjacentIndexes, adjacentRates);
      switch (scale_action)
        {
        case RsAction::DOWNSCALE:
          if (std::get<0> (adjacentIndexes) != INVALID_INDEX)
            {
              update_lq = true;
              index = std::get<0> (adjacentIndexes);
            }
          else
            {
              NS_LOG_DEBUG ("At the bottom rate. Can't decrease");
            }
          break;
        case RsAction::UPSCALE:
          if (std::get<1> (adjacentIndexes) != INVALID_INDEX)
            {
              update_lq = true;
              index = std::get<1> (adjacentIndexes);
            }
          else
            {
              NS_LOG_DEBUG ("At the top rate. Can't increase");
            }
          break;
        case RsAction::STAY:
          if (m_s == RsState::STAY_IN_COLUMN)
            {
              /* Here, in the original Intel code, transmission power adaptation is done (rs_tpt_perform).
               * This wasn't done in this code, so patches welcome !
               */
            }
          break;
        default:
          break;
        }
    }

  if (update_lq)
    {
      m_index = index;
    }

  StayInTable ();
  if (!update_lq && !done_search && m_s == RsState::SEARCH_CYCLE_STARTED && GetHistory ().Counter ())
    {
      NS_LOG_DEBUG ("Saving last tpt");
      m_lastThroughput = GetHistory ().AverageThroughput ();
      NS_LOG_DEBUG ("Start Search: update_lq " << update_lq << " done_search " << done_search << " rs_state " << m_s << " win->counter " << GetHistory ().Counter ());
      auto findColumn = GetNextColumn (m_visitedColumns);
      if (std::get<0> (findColumn))
        {
          NS_LOG_DEBUG ("Switch to column.");

          std::tuple<ColumnMode, std::set<IntelWifiAntenna>, GuardInterval> newColumnParameters = std::get<1> (findColumn);

          m_oldColumnParameters = std::make_tuple (m_mode, m_antennas, m_guardInterval, m_index, m_bandwidth);

          int nextIndex = GetNextIndex (newColumnParameters);

          m_columnScaling = true;
          m_mode = std::get<0> (newColumnParameters);
          if (m_mode == ColumnMode::LEGACY)
            {
              m_type = LEGACY_G;
            }
          else
            {
              m_type = NONE;
              m_bandwidth = m_maxWidth;
            }

          m_antennas = std::get<1> (newColumnParameters);
          m_guardInterval = std::get<2> (newColumnParameters);
          m_index = nextIndex;

          m_visitedColumns.insert ({m_mode, m_antennas, m_guardInterval});
          /* We start in a new column with a clean history */
          ClearHistories ();
        }
      else
        {
          NS_LOG_DEBUG ("No more columns to explore in search cycle. Go to RsState::SEARCH_CYCLE_ENDED");
          m_s = RsState::SEARCH_CYCLE_ENDED;
          // TODO: FIXME
          done_search = 1;
        }
    }

  if (done_search && m_s == RsState::SEARCH_CYCLE_ENDED)
    {
      SetStayInTable ();
    }
}

std::tuple<ns3::WifiMode, int, int, int, int, int, bool>
State::GetTxVector(bool ht, bool vht)
{
  std::string rate;
  int index = m_index;
  int nss = 1;
  if (m_mode == ColumnMode::SISO)
    {
      if (index == 4)
        {
          index = 0;
        }
      else
        {
          index -= 5;
        }
      if (!vht)
        {
          rate = "HtMcs" + std::to_string (index);
        }
      else
        {
          rate = "VhtMcs" + std::to_string (index);
        }
    }
  else if (m_mode == ColumnMode::MIMO)
    {
      nss = 2;
      if (!vht)
        {
          if (index == 4)
            {
              index = 8;
            }
          else
            {
              index += 3;
            }
          rate = "HtMcs" + std::to_string (index);
        }
      else
        {
          if (index == 4)
            {
              index = 0;
            }
          else
            {
              index -= 5;
            }
          rate = "VhtMcs" + std::to_string (index);
        }
    }
  else
    {
      if (index <= 4)
        {
          index = 4;
        }
      switch(index) {
      case 0:
        rate = "DsssRate1Mbps";
        break;
      case 1:
        rate = "DsssRate2Mbps";
        break;
      case 2:
        rate = "DsssRate5_5Mbps";
        break;
      case 3:
        rate = "DsssRate11Mbps";
        break;
      case 4:
        rate = "OfdmRate6Mbps";
        break;
      case 5:
        rate = "OfdmRate9Mbps";
        break;
      case 6:
        rate = "OfdmRate12Mbps";
        break;
      case 7:
        rate = "OfdmRate18Mbps";
        break;
      case 8:
        rate = "OfdmRate24Mbps";
        break;
      case 9:
        rate = "OfdmRate36Mbps";
        break;
      case 10:
        rate = "OfdmRate48Mbps";
        break;
      case 11:
        rate = "OfdmRate54Mbps";
        break;
      default:
        NS_LOG_DEBUG ("Warning default case legacy rate");
        break;
      }
    }

  ns3::WifiMode mode{rate};
  return {mode, m_guardInterval == GuardInterval::LGI ? 800 : 400, m_antennas.size (), nss, 0, m_bandwidth, m_agg == Aggregation::AGG};
}

void
State::Tx (int success, int failed, bool ampdu)
{
  // TODO: Check if Tx idle for too long
  if (ampdu && success == 0)
    {
      // We missed the block ack
      failed = 1;
    }

  for (int i = 0; i < success; ++i)
    {
      GetHistory ().Tx (true);
    }

  for (int i = 0; i<failed; ++i)
    {
      GetHistory ().Tx (false);
    }

  if (m_s == RsState::STAY_IN_COLUMN)
    {
      m_totalSuccess += success;
      m_totalFailed += failed;
    }

  RateScaling ();
}


struct IntelWifiRemoteStation : public WifiRemoteStation
{
  State m_state;

  IntelWifiRemoteStation (uint16_t width);
};

IntelWifiRemoteStation::IntelWifiRemoteStation (uint16_t width)
  : m_state{width}
{
}

TypeId
IntelWifiManager::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::IntelWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<IntelWifiManager> ()
    .AddAttribute ("ControlMode", "The transmission mode to use for every RTS packet transmission.",
        StringValue ("OfdmRate6Mbps"),
        MakeWifiModeAccessor (&IntelWifiManager::m_ctlMode),
        MakeWifiModeChecker ())
    .AddTraceSource ("Rate",
                     "Traced value for rate changes (b/s)",
                     MakeTraceSourceAccessor (&IntelWifiManager::m_currentRate),
                     "ns3::TracedValueCallback::Uint64")
    ;
  return tid;
}


WifiModeList
IntelWifiManager::GetVhtDeviceMcsList () const
{
  WifiModeList vhtMcsList;
  Ptr<WifiPhy> phy = GetPhy ();
  for (const auto &mode : phy->GetMcsList ())
    {
      if (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
        {
          vhtMcsList.push_back (mode);
        }
    }
  return vhtMcsList;
}

WifiModeList
IntelWifiManager::GetHtDeviceMcsList () const
{
  WifiModeList htMcsList;
  Ptr<WifiPhy> phy = GetPhy ();
  for (const auto &mode : phy->GetMcsList ())
    {
      if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
        {
          htMcsList.push_back (mode);
        }
    }
  return htMcsList;
}

bool
IntelWifiManager::IsValidMcs (Ptr<WifiPhy> phy, uint8_t streams, uint16_t chWidth, WifiMode mode)
{
  NS_LOG_FUNCTION (this << phy << +streams << chWidth << mode);
  WifiTxVector txvector;
  txvector.SetNss (streams);
  txvector.SetChannelWidth (chWidth);
  txvector.SetMode (mode);
  return txvector.IsValid ();
}

void
IntelWifiManager::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  if (GetHtSupported ())
    {
      WifiModeList htMcsList = GetHtDeviceMcsList ();
    }

  if (GetVhtSupported ())
    {
      WifiModeList vhtMcsList = GetVhtDeviceMcsList ();
    }

  if (!(GetVhtSupported() || GetHtSupported()))
    {
      NS_LOG_DEBUG ("Device does not support HT or VHT!");
    }
}

IntelWifiManager::IntelWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

IntelWifiManager::~IntelWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

WifiRemoteStation *
IntelWifiManager::DoCreateStation (void) const {
  NS_LOG_FUNCTION (this);
  IntelWifiRemoteStation *station = new IntelWifiRemoteStation (GetPhy ()->GetChannelWidth ());
  return station;
}

void
IntelWifiManager::DoReportRxOk (WifiRemoteStation *station, double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
IntelWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
IntelWifiManager::DoReportDataFailed (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  auto station = static_cast<IntelWifiRemoteStation *> (st);
  station->m_state.Tx (0, 1, false);
}

void
IntelWifiManager::DoReportRtsOk (WifiRemoteStation *st, double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode << rtsSnr);
}

void
IntelWifiManager::DoReportDataOk (WifiRemoteStation *st, double ackSnr, WifiMode ackMode, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << +dataNss);
  auto station = static_cast<IntelWifiRemoteStation *> (st);
  station->m_state.Tx (1, 0, false);
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr);
}

void
IntelWifiManager::DoReportAmpduTxStatus (WifiRemoteStation *st, uint16_t nSuccessfulMpdus, uint16_t nFailedMpdus, double rxSnr, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  auto station = static_cast<IntelWifiRemoteStation *> (st);
  station->m_state.Tx(nSuccessfulMpdus, nFailedMpdus, true);
}

void
IntelWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
IntelWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

WifiTxVector
IntelWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  auto station = static_cast<IntelWifiRemoteStation *> (st);

  WifiMode mode;
  uint16_t guardInterval;
  uint8_t nTx;
  uint8_t nss;
  uint8_t ness;
  uint16_t channelWidth;
  bool aggregation;

  std::tie(mode, guardInterval, nTx, nss, ness, channelWidth, aggregation) =
    station->m_state.GetTxVector (GetHtSupported (), GetVhtSupported ());

  return WifiTxVector (mode,
                       GetDefaultTxPowerLevel (),
                       GetPreambleForTransmission (mode.GetModulationClass (),
                                                   guardInterval == 400),
                       guardInterval,
                       nTx,
                       nss,
                       ness,
                       channelWidth,
                       aggregation);
}

WifiTxVector
IntelWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  NS_LOG_DEBUG ("Warning: RTS/CTS not yet fully supported.");
  return WifiTxVector (m_ctlMode,
                       GetDefaultTxPowerLevel (),
                       GetPreambleForTransmission (m_ctlMode.GetModulationClass (), GetShortPreambleEnabled ()),
                       ConvertGuardIntervalToNanoSeconds (m_ctlMode,
                                                          GetShortGuardIntervalSupported (st),
                                                          NanoSeconds (GetGuardInterval (st))),
                       1,
                       1,
                       0,
                       GetChannelWidthForTransmission (m_ctlMode, GetChannelWidth (st)), GetAggregation (st),
                       false);
}

}
