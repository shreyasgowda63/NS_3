/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef VHT_FRAME_EXCHANGE_MANAGER_H
#define VHT_FRAME_EXCHANGE_MANAGER_H

#include "ns3/ht-frame-exchange-manager.h"

namespace ns3
{
namespace wifi
{

/**
 * \ingroup wifi
 *
 * VhtFrameExchangeManager handles the frame exchange sequences
 * for VHT stations.
 */
class VhtFrameExchangeManager : public HtFrameExchangeManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    VhtFrameExchangeManager();
    ~VhtFrameExchangeManager() override;

  protected:
    Ptr<WifiPsdu> GetWifiPsdu(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector) const override;
    uint32_t GetPsduSize(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector) const override;
};

} // namespace wifi
} // namespace ns3

#endif /* VHT_FRAME_EXCHANGE_MANAGER_H */
