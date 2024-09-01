/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
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

#ifndef DEFAULT_AP_EMLSR_MANAGER_H
#define DEFAULT_AP_EMLSR_MANAGER_H

#include "ap-emlsr-manager.h"

namespace ns3
{

/**
 * \ingroup wifi
 *
 * DefaultApEmlsrManager is the default AP EMLSR manager.
 */
class DefaultApEmlsrManager : public ApEmlsrManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultApEmlsrManager();
    ~DefaultApEmlsrManager() override;

    Time GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                     const WifiTxVector& txVector,
                                     WifiPhyBand band) override;
    bool UpdateCwAfterFailedIcf() override;
};

} // namespace ns3

#endif /* DEFAULT_AP_EMLSR_MANAGER_H */
