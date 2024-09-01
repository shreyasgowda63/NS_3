/*
 * Copyright (c) 2013 Budiarto Herman
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
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#ifndef NO_OP_HANDOVER_ALGORITHM_H
#define NO_OP_HANDOVER_ALGORITHM_H

#include "lte-handover-algorithm.h"
#include "lte-handover-management-sap.h"
#include "lte-rrc-sap.h"

namespace ns3
{

/**
 * \brief Handover algorithm implementation which simply does nothing.
 *
 * Selecting this handover algorithm is equivalent to disabling automatic
 * triggering of handover. This is the default choice.
 *
 * To enable automatic handover, please select another handover algorithm, i.e.,
 * another child class of LteHandoverAlgorithm.
 */
class NoOpHandoverAlgorithm : public LteHandoverAlgorithm
{
  public:
    /// Creates a No-op handover algorithm instance.
    NoOpHandoverAlgorithm();

    ~NoOpHandoverAlgorithm() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from LteHandoverAlgorithm
    void SetLteHandoverManagementSapUser(LteHandoverManagementSapUser* s) override;
    LteHandoverManagementSapProvider* GetLteHandoverManagementSapProvider() override;

    /// let the forwarder class access the protected and private members
    friend class MemberLteHandoverManagementSapProvider<NoOpHandoverAlgorithm>;

  protected:
    // inherited from Object
    void DoInitialize() override;
    void DoDispose() override;

    // inherited from LteHandoverAlgorithm as a Handover Management SAP implementation
    void DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults) override;

  private:
    /// Interface to the eNodeB RRC instance.
    LteHandoverManagementSapUser* m_handoverManagementSapUser;
    /// Receive API calls from the eNodeB RRC instance.
    LteHandoverManagementSapProvider* m_handoverManagementSapProvider;
};

} // namespace ns3

#endif /* NO_OP_HANDOVER_ALGORITHM_H */
