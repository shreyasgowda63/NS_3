/*
 * Copyright (c) 2018 University of Washington
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "obss-pd-algorithm.h"

#include "ns3/double.h"
#include "ns3/eht-phy.h"
#include "ns3/log.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ObssPdAlgorithm");
NS_OBJECT_ENSURE_REGISTERED(ObssPdAlgorithm);

TypeId
ObssPdAlgorithm::GetTypeId()
{
    static ns3::TypeId tid =
        ns3::TypeId("ns3::ObssPdAlgorithm")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute(
                "ObssPdLevel",
                "The current OBSS PD level.",
                dBmValue(-82.0_dBm),
                MakedBmAccessor(&ObssPdAlgorithm::SetObssPdLevel, &ObssPdAlgorithm::GetObssPdLevel),
                MakedBmChecker(-101_dBm, -62_dBm))
            .AddAttribute("ObssPdLevelMin",
                          "Minimum value of OBSS PD level.",
                          dBmValue(-82.0_dBm),
                          MakedBmAccessor(&ObssPdAlgorithm::m_obssPdLevelMin),
                          MakedBmChecker(-101_dBm, -62_dBm))
            .AddAttribute("ObssPdLevelMax",
                          "Maximum value of OBSS PD level.",
                          dBmValue(-62.0_dBm),
                          MakedBmAccessor(&ObssPdAlgorithm::m_obssPdLevelMax),
                          MakedBmChecker(-101_dBm, -62_dBm))
            .AddAttribute("TxPowerRefSiso",
                          "The SISO reference TX power level.",
                          dBmValue(21_dBm),
                          MakedBmAccessor(&ObssPdAlgorithm::m_txPowerRefSiso),
                          MakedBmChecker())
            .AddAttribute("TxPowerRefMimo",
                          "The MIMO reference TX power level.",
                          dBmValue(25_dBm),
                          MakedBmAccessor(&ObssPdAlgorithm::m_txPowerRefMimo),
                          MakedBmChecker())
            .AddTraceSource("Reset",
                            "Trace CCA Reset event",
                            MakeTraceSourceAccessor(&ObssPdAlgorithm::m_resetEvent),
                            "ns3::ObssPdAlgorithm::ResetTracedCallback");
    return tid;
}

void
ObssPdAlgorithm::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_device = nullptr;
}

void
ObssPdAlgorithm::ConnectWifiNetDevice(const Ptr<WifiNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_device = device;
    auto phy = device->GetPhy();
    if (phy->GetStandard() >= WIFI_STANDARD_80211be)
    {
        auto ehtPhy = DynamicCast<EhtPhy>(device->GetPhy()->GetPhyEntity(WIFI_MOD_CLASS_EHT));
        NS_ASSERT(ehtPhy);
        ehtPhy->SetObssPdAlgorithm(this);
    }
    auto hePhy = DynamicCast<HePhy>(device->GetPhy()->GetPhyEntity(WIFI_MOD_CLASS_HE));
    NS_ASSERT(hePhy);
    hePhy->SetObssPdAlgorithm(this);
}

void
ObssPdAlgorithm::ResetPhy(HeSigAParameters params)
{
    dBm txPowerMaxSiso{0};
    dBm txPowerMaxMimo{0};
    bool powerRestricted = false;
    // Fetch my BSS color
    Ptr<HeConfiguration> heConfiguration = m_device->GetHeConfiguration();
    NS_ASSERT(heConfiguration);
    uint8_t bssColor = heConfiguration->GetBssColor();
    NS_LOG_DEBUG("My BSS color " << (uint16_t)bssColor << " received frame "
                                 << (uint16_t)params.bssColor);

    Ptr<WifiPhy> phy = m_device->GetPhy();
    if ((m_obssPdLevel > m_obssPdLevelMin) && (m_obssPdLevel <= m_obssPdLevelMax))
    {
        txPowerMaxSiso =
            m_txPowerRefSiso.in_dBm() - (m_obssPdLevel.in_dBm() - m_obssPdLevelMin.in_dBm());
        txPowerMaxMimo =
            m_txPowerRefMimo.in_dBm() - (m_obssPdLevel.in_dBm() - m_obssPdLevelMin.in_dBm());
        powerRestricted = true;
    }
    m_resetEvent(bssColor, params.rssi, powerRestricted, txPowerMaxSiso, txPowerMaxMimo);
    phy->ResetCca(powerRestricted, txPowerMaxSiso, txPowerMaxMimo);
}

void
ObssPdAlgorithm::SetObssPdLevel(dBm level)
{
    NS_LOG_FUNCTION(this << level);
    m_obssPdLevel = level;
}

dBm
ObssPdAlgorithm::GetObssPdLevel() const
{
    return m_obssPdLevel;
}

} // namespace ns3
