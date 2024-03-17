/*
 * Copyright (c) 2016
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

#include "wifi-mac-helper.h"

#include "ns3/boolean.h"
#include "ns3/eht-configuration.h"
#include "ns3/emlsr-manager.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/multi-user-scheduler.h"
#include "ns3/wifi-ack-manager.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection-manager.h"

namespace ns3
{

WifiMacHelper::WifiMacHelper()
{
    // By default, we create an AdHoc MAC layer (without QoS).
    SetType("ns3::AdhocWifiMac");

    m_assocManager.SetTypeId("ns3::WifiDefaultAssocManager");
    m_queueScheduler.SetTypeId("ns3::FcfsWifiQueueScheduler");
    m_protectionManager.SetTypeId("ns3::WifiDefaultProtectionManager");
    m_ackManager.SetTypeId("ns3::WifiDefaultAckManager");
    m_emlsrManager.SetTypeId("ns3::DefaultEmlsrManager");
}

WifiMacHelper::~WifiMacHelper()
{
}

Ptr<wifi::WifiMac>
WifiMacHelper::Create(Ptr<wifi::WifiNetDevice> device, wifi::WifiStandard standard) const
{
    NS_ABORT_MSG_IF(standard == wifi::WIFI_STANDARD_UNSPECIFIED, "No standard specified!");

    // this is a const method, but we need to force the correct QoS setting
    ObjectFactory macObjectFactory = m_mac;
    if (standard >= wifi::WIFI_STANDARD_80211n)
    {
        macObjectFactory.Set("QosSupported", BooleanValue(true));
    }

    auto mac = macObjectFactory.Create<wifi::WifiMac>();
    mac->SetDevice(device);
    mac->SetAddress(Mac48Address::Allocate());
    device->SetMac(mac);
    mac->ConfigureStandard(standard);

    auto queueScheduler = m_queueScheduler.Create<wifi::WifiMacQueueScheduler>();
    mac->SetMacQueueScheduler(queueScheduler);

    // WaveNetDevice (through ns-3.38) stores PHY entities in a different member than WifiNetDevice,
    // hence GetNPhys() would return 0. We have to attach a protection manager and an ack manager to
    // the unique instance of frame exchange manager anyway
    for (uint8_t linkId = 0; linkId < std::max<uint8_t>(device->GetNPhys(), 1); ++linkId)
    {
        auto fem = mac->GetFrameExchangeManager(linkId);

        auto protectionManager = m_protectionManager.Create<wifi::WifiProtectionManager>();
        protectionManager->SetWifiMac(mac);
        protectionManager->SetLinkId(linkId);
        fem->SetProtectionManager(protectionManager);

        auto ackManager = m_ackManager.Create<wifi::WifiAckManager>();
        ackManager->SetWifiMac(mac);
        ackManager->SetLinkId(linkId);
        fem->SetAckManager(ackManager);

        // 11be MLDs require a MAC address to be assigned to each STA. Note that
        // FrameExchangeManager objects are created by WifiMac::SetupFrameExchangeManager
        // (which is invoked by WifiMac::ConfigureStandard, which is called above),
        // which sets the FrameExchangeManager's address to the address held by WifiMac.
        // Hence, in case the number of PHY objects is 1, the FrameExchangeManager's
        // address equals the WifiMac's address.
        if (device->GetNPhys() > 1)
        {
            fem->SetAddress(Mac48Address::Allocate());
        }
    }

    // create and install the Multi User Scheduler if this is an HE AP
    Ptr<wifi::ApWifiMac> apMac;
    if (standard >= wifi::WIFI_STANDARD_80211ax && m_muScheduler.IsTypeIdSet() &&
        (apMac = DynamicCast<wifi::ApWifiMac>(mac)))
    {
        auto muScheduler = m_muScheduler.Create<wifi::MultiUserScheduler>();
        apMac->AggregateObject(muScheduler);
    }

    // create and install the Association Manager if this is a STA
    auto staMac = DynamicCast<wifi::StaWifiMac>(mac);
    if (staMac)
    {
        auto assocManager = m_assocManager.Create<wifi::WifiAssocManager>();
        staMac->SetAssocManager(assocManager);
    }

    // create and install the EMLSR Manager if this is an EHT non-AP MLD with EMLSR activated
    if (BooleanValue emlsrActivated;
        standard >= wifi::WIFI_STANDARD_80211be && staMac && staMac->GetNLinks() > 1 &&
        device->GetEhtConfiguration()->GetAttributeFailSafe("EmlsrActivated", emlsrActivated) &&
        emlsrActivated.Get())
    {
        auto emlsrManager = m_emlsrManager.Create<wifi::EmlsrManager>();
        staMac->SetEmlsrManager(emlsrManager);
    }

    return mac;
}

} // namespace ns3
