/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef ADVANCED_EMLSR_MANAGER_H
#define ADVANCED_EMLSR_MANAGER_H

#include "default-emlsr-manager.h"

namespace ns3
{

/**
 * \ingroup wifi
 *
 * AdvancedEmlsrManager is an advanced EMLSR manager.
 */
class AdvancedEmlsrManager : public DefaultEmlsrManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    AdvancedEmlsrManager();
    ~AdvancedEmlsrManager() override;

    Time GetDelayUntilAccessRequest(uint8_t linkId) override;

  protected:
    void DoDispose() override;
    void DoSetWifiMac(Ptr<StaWifiMac> mac) override;

    /**
     * Possibly take actions when notified of the MAC header of the MPDU being received by the
     * given PHY.
     *
     * \param phy the given PHY
     * \param macHdr the MAC header of the MPDU being received
     * \param txVector the TXVECTOR used to transmit the PSDU
     * \param psduDuration the remaining duration of the PSDU
     */
    void ReceivedMacHdr(Ptr<WifiPhy> phy,
                        const WifiMacHeader& macHdr,
                        const WifiTxVector& txVector,
                        Time psduDuration);

  private:
    bool m_useNotifiedMacHdr; //!< whether to use the information about the MAC header of
                              //!< the MPDU being received (if notified by the PHY)
    bool m_allowUlTxopInRx;   //!< whether a (main or aux) PHY is allowed to start an UL
                              //!< TXOP if another PHY is receiving a PPDU
};

} // namespace ns3

#endif /* ADVANCED_EMLSR_MANAGER_H */
