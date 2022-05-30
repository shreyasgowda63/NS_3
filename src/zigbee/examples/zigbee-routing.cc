/*
 * Copyright (c) 2023 Tokushima University, Japan
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
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/**
 * This example shows the NWK procedure to perform a route request.
 * Prior the route request, an association-based join is performed.
 * The procedure requires a sequence of primitive calls on a specific order in the indicated
 * devices.
 *
 *
 *  Network Extended PAN id: 0X000000000000CA:FE (based on the PAN coordinator address)
 *
 *
 *  [Coordinator] ZC  (dev0): [00:00:00:00:00:00:CA:FE]  [00:00]
 *  [Router 1]    ZR1 (dev1): [00:00:00:00:00:00:00:01]  [short addr assigned by ZC]
 *  [Router 2]    ZR2 (dev2): [00:00:00:00:00:00:00:02]  [short addr assigned by ZR1]
 *  [Router 3]    ZR3 (dev3): [00:00:00:00:00:00:00:03]  [short addr assigned by ZR2]
 *  [Router 4]    ZR4 (dev4): [00:00:00:00:00:00:00:04]  [short addr assigned by ZR4]
 *
 *  Topology:
 *
 *  ZC--------ZR1------------ZR2----------ZR3
 *              |
 *              |
 *             ZR4
 *
 *
 *
 *
 *
 */

#include <ns3/constant-position-mobility-model.h>
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/zigbee-module.h>

#include <iostream>

using namespace ns3;
using namespace zigbee;

NS_LOG_COMPONENT_DEFINE("ZigbeeExample");

static void
NwkDataIndication(Ptr<ZigbeeStack> stack, NldeDataIndicationParams params, Ptr<Packet> p)
{
    std::cout << "Received packet of size " << p->GetSize() << "\n";
}

static void
NwkNetworkFormationConfirm(Ptr<ZigbeeStack> stack, NlmeNetworkFormationConfirmParams params)
{
    std::cout << "NlmeNetworkFormationConfirmStatus = " << static_cast<uint32_t>(params.m_status)
              << "\n";
}

static void
NwkNetworkDiscoveryConfirm(Ptr<ZigbeeStack> stack, NlmeNetworkDiscoveryConfirmParams params)
{
    // See Zigbee Specification r22.1.0, 3.6.1.4.1
    // This method implements a simplistic version of the method implemented
    // in a zigbee APL layer. In this layer a candidate Extended PAN Id must
    // be selected and a NLME-JOIN.request must be issued.

    if (params.m_status == ZigbeeNwkStatus::SUCCESS)
    {
        std::cout << " Network discovery confirm Received. Networks found:\n";

        for (const auto& netDescriptor : params.m_netDescList)
        {
            std::cout << " ExtPanID: 0x" << std::hex << netDescriptor.m_extPanId << std::dec
                      << " CH:  " << static_cast<uint32_t>(netDescriptor.m_logCh) << std::hex
                      << " Pan Id: 0x" << netDescriptor.m_panId << " stackprofile " << std::dec
                      << static_cast<uint32_t>(netDescriptor.m_stackProfile) << "\n";
        }

        NlmeJoinRequestParams joinParams;

        zigbee::CapabilityInformation capaInfo;
        capaInfo.SetDeviceType(ROUTER);
        capaInfo.SetAllocateAddrOn(true);

        joinParams.m_rejoinNetwork = ASSOCIATION;
        joinParams.m_capabilityInfo = capaInfo.GetCapability();
        joinParams.m_extendedPanId = params.m_netDescList[0].m_extPanId;

        Simulator::ScheduleNow(&zigbee::ZigbeeNwk::NlmeJoinRequest, stack->GetNwk(), joinParams);
    }
    else
    {
        NS_ABORT_MSG(
            "Unable to discover networks | status: " << static_cast<uint32_t>(params.m_status));
    }
}

static void
NwkJoinConfirm(Ptr<ZigbeeStack> stack, NlmeJoinConfirmParams params)
{
    if (params.m_status == ZigbeeNwkStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S)
                  << " The device joined the network SUCCESSFULLY with short address " << std::hex
                  << params.m_networkAddress << " on the Extended PAN Id: " << std::hex
                  << params.m_extendedPanId << "\n"
                  << std::dec;

        // 3 - After dev 1 is associated, it should be started as a router
        //     (i.e. it becomes able to accept request from other devices to join the network)
        zigbee::NlmeStartRouterRequestParams startRouterParams;
        Simulator::ScheduleNow(&zigbee::ZigbeeNwk::NlmeStartRouterRequest,
                               stack->GetNwk(),
                               startRouterParams);
    }
    else
    {
        std::cout << " The device FAILED to join the network with status " << params.m_status
                  << "\n";
    }
}

static void
NwkRouteDiscoveryConfirm(Ptr<ZigbeeStack> stack, NlmeRouteDiscoveryConfirmParams params)
{
    std::cout << "NlmeRouteDiscoveryConfirmStatus = " << static_cast<uint32_t>(params.m_status)
              << "\n";
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    LogComponentEnable("ZigbeeNwk", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanCsmaCa", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_DEBUG);

    RngSeedManager::SetSeed(3);
    RngSeedManager::SetRun(4);

    NodeContainer nodes;
    nodes.Create(5);

    //// Configure MAC

    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    Ptr<LrWpanNetDevice> dev0 = lrwpanDevices.Get(0)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = lrwpanDevices.Get(1)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = lrwpanDevices.Get(2)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev3 = lrwpanDevices.Get(3)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev4 = lrwpanDevices.Get(4)->GetObject<LrWpanNetDevice>();

    dev0->GetMac()->SetExtendedAddress("00:00:00:00:00:00:CA:FE");
    dev1->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:01");
    dev2->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:02");
    dev3->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:03");
    dev4->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:04");

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);
    dev2->SetChannel(channel);
    dev3->SetChannel(channel);
    dev4->SetChannel(channel);

    //// Configure NWK

    ZigbeeHelper zigbee;
    ZigbeeStackContainer zigbeeStackContainer = zigbee.Install(lrwpanDevices);

    Ptr<ZigbeeStack> zstack0 = zigbeeStackContainer.Get(0)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack1 = zigbeeStackContainer.Get(1)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack2 = zigbeeStackContainer.Get(2)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack3 = zigbeeStackContainer.Get(3)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack4 = zigbeeStackContainer.Get(4)->GetObject<ZigbeeStack>();

    //// Configure Nodes Mobility

    Ptr<ConstantPositionMobilityModel> dev0Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(dev0Mobility);

    Ptr<ConstantPositionMobilityModel> dev1Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev1Mobility->SetPosition(Vector(90, 0, 0)); // 110 ,0,0
    dev1->GetPhy()->SetMobility(dev1Mobility);

    Ptr<ConstantPositionMobilityModel> dev2Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev2Mobility->SetPosition(Vector(170, 0, 0)); // 190,0,0
    dev2->GetPhy()->SetMobility(dev2Mobility);

    Ptr<ConstantPositionMobilityModel> dev3Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev3Mobility->SetPosition(Vector(250, 0, 0)); // 250,0,0
    dev3->GetPhy()->SetMobility(dev3Mobility);

    Ptr<ConstantPositionMobilityModel> dev4Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev4Mobility->SetPosition(Vector(90, 50, 0)); // 110,50,0
    dev4->GetPhy()->SetMobility(dev4Mobility);

    // NWK callbacks hooks, these hooks are usually directly connected to the APS layer
    // where all these calls inform the result of a request originated the APS layer.

    zstack0->GetNwk()->SetNlmeNetworkFormationConfirmCallback(
        MakeBoundCallback(&NwkNetworkFormationConfirm, zstack0));
    zstack0->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack0));
    zstack0->GetNwk()->SetNlmeRouteDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkRouteDiscoveryConfirm, zstack0));

    zstack1->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack1));
    zstack2->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack2));
    zstack3->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack3));
    zstack4->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack4));

    zstack1->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack1));
    zstack2->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack2));
    zstack3->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack3));
    zstack4->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack4));

    zstack1->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack1));
    zstack2->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack2));
    zstack3->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack3));
    zstack4->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack4));

    // 1 - Initiate the Zigbee coordinator, start the network
    NlmeNetworkFormationRequestParams netFormParams;
    netFormParams.m_scanChannelList.channelPageCount = 1;
    netFormParams.m_scanChannelList.channelsField[0] = 0x07FFF800; // 0x00000800;
    netFormParams.m_scanDuration = 0;
    netFormParams.m_superFrameOrder = 15;
    netFormParams.m_beaconOrder = 15;

    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(1.0),
                                   &zigbee::ZigbeeNwk::NlmeNetworkFormationRequest,
                                   zstack0->GetNwk(),
                                   netFormParams);

    // 2- Let the dev1 (Router) discovery the coordinator and join the network, after
    //    this, it will become router itself(call to NLME-START-ROUTER.request). We
    //    continue doing the same with the rest of the devices which will discover the
    //    previously added routers and join the network
    NlmeNetworkDiscoveryRequestParams netDiscParams;
    netDiscParams.m_scanChannelList.channelPageCount = 1;
    netDiscParams.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
    netDiscParams.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack1->GetNode()->GetId(),
                                   Seconds(3.0),
                                   &zigbee::ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack1->GetNwk(),
                                   netDiscParams);

    NlmeNetworkDiscoveryRequestParams netDiscParams2;
    netDiscParams2.m_scanChannelList.channelPageCount = 1;
    netDiscParams2.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
    netDiscParams2.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack2->GetNode()->GetId(),
                                   Seconds(4.0),
                                   &zigbee::ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack2->GetNwk(),
                                   netDiscParams2);

    NlmeNetworkDiscoveryRequestParams netDiscParams3;
    netDiscParams2.m_scanChannelList.channelPageCount = 1;
    netDiscParams2.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
    netDiscParams2.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack3->GetNode()->GetId(),
                                   Seconds(5.0),
                                   &zigbee::ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack3->GetNwk(),
                                   netDiscParams3);

    NlmeNetworkDiscoveryRequestParams netDiscParams4;
    netDiscParams4.m_scanChannelList.channelPageCount = 1;
    netDiscParams4.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
    netDiscParams4.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack4->GetNode()->GetId(),
                                   Seconds(6.0),
                                   &zigbee::ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack4->GetNwk(),
                                   netDiscParams4);

    // 5- Find a route to the given device short address
    NlmeRouteDiscoveryRequestParams routeDiscParams;
    routeDiscParams.m_dstAddr = Mac16Address("0d:10");
    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(8),
                                   &zigbee::ZigbeeNwk::NlmeRouteDiscoveryRequest,
                                   zstack0->GetNwk(),
                                   routeDiscParams);

    // Print routing tables of coordinator (originator of route request) at
    // the end of the simulation

    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(11),
                                   &zigbee::ZigbeeNwk::PrintNeighborTable,
                                   zstack0->GetNwk(),
                                   stream);

    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(11),
                                   &zigbee::ZigbeeNwk::PrintRoutingTable,
                                   zstack0->GetNwk(),
                                   stream);

    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(11),
                                   &zigbee::ZigbeeNwk::PrintRouteDiscoveryTable,
                                   zstack0->GetNwk(),
                                   stream);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
