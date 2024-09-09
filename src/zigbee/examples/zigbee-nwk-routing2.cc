/*
 * Copyright (c) 2024 Tokushima University, Japan
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

#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/mobility-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/zigbee-module.h>

#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;
using namespace ns3::zigbee;

// NS_LOG_COMPONENT_DEFINE("ZigbeeNwkRouting2");

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
        std::cout << "    Network discovery confirm Received. Networks found "
                  << "(" << params.m_netDescList.size() << ")\n";

        for (const auto& netDescriptor : params.m_netDescList)
        {
            std::cout << "      ExtPanID: 0x" << std::hex << netDescriptor.m_extPanId << std::dec
                      << "\n"
                      << "      CH:  " << static_cast<uint32_t>(netDescriptor.m_logCh) << std::hex
                      << "\n"
                      << "      Pan Id: 0x" << netDescriptor.m_panId << std::hex << "\n"
                      << "      stackprofile: " << std::dec
                      << static_cast<uint32_t>(netDescriptor.m_stackProfile) << "\n"
                      << "      ----------------\n ";
        }

        NlmeJoinRequestParams joinParams;

        zigbee::CapabilityInformation capaInfo;
        capaInfo.SetDeviceType(ROUTER);
        capaInfo.SetAllocateAddrOn(true);

        joinParams.m_rejoinNetwork = zigbee::JoiningMethod::ASSOCIATION;
        joinParams.m_capabilityInfo = capaInfo.GetCapability();
        joinParams.m_extendedPanId = params.m_netDescList[0].m_extPanId;

        Simulator::ScheduleNow(&ZigbeeNwk::NlmeJoinRequest, stack->GetNwk(), joinParams);
    }
    else
    {
        std::cout << " WARNING: Unable to discover networks | status: "
                  << static_cast<uint32_t>(params.m_status) << "\n";
    }
}

static void
NwkJoinConfirm(Ptr<ZigbeeStack> stack, NlmeJoinConfirmParams params)
{
    if (params.m_status == ZigbeeNwkStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S)
                  << " The device joined the network SUCCESSFULLY with short address [" << std::hex
                  << params.m_networkAddress << "] on the Extended PAN Id: " << std::hex
                  << params.m_extendedPanId << "\n"
                  << std::dec;

        // 3 - After dev  is associated, it should be started as a router
        //     (i.e. it becomes able to accept request from other devices to join the network)
        NlmeStartRouterRequestParams startRouterParams;
        Simulator::ScheduleNow(&ZigbeeNwk::NlmeStartRouterRequest,
                               stack->GetNwk(),
                               startRouterParams);
    }
    else
    {
        std::cout << Simulator::Now().As(Time::S)
                  << " The device FAILED to join the network with status "
                  << static_cast<uint32_t>(params.m_status) << "\n";
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

    // RngSeedManager::SetSeed(3);
    // RngSeedManager::SetRun(4);

    NodeContainer nodes;
    nodes.Create(10);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(30.0),
                                  "DeltaY",
                                  DoubleValue(30.0),
                                  "GridWidth",
                                  UintegerValue(20),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.Install(nodes);

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    LrWpanHelper lrWpanHelper;
    lrWpanHelper.SetChannel(channel);

    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

    // Set the extended address to all devices (EUI-64)
    lrWpanHelper.SetExtendedAddresses(lrwpanDevices);

    ZigbeeHelper zigbeeHelper;
    ZigbeeStackContainer zigbeeStacks = zigbeeHelper.Install(lrwpanDevices);

    for (auto i = zigbeeStacks.Begin(); i != zigbeeStacks.End(); i++)
    {
        Ptr<ZigbeeStack> zstack = *i;
        // NLME-NETWORK-FORMATION.Confirm
        zstack->GetNwk()->SetNlmeNetworkFormationConfirmCallback(
            MakeBoundCallback(&NwkNetworkFormationConfirm, zstack));
        // NLDE-DATA.Indication
        zstack->GetNwk()->SetNldeDataIndicationCallback(
            MakeBoundCallback(&NwkDataIndication, zstack));
        // NLDE-NETWORK-DISCOVERY.Confirm
        zstack->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
            MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack));
        // NLME-JOIN.Confirm
        zstack->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack));
        // NLME-ROUTE-DISCOVERY.Confirm
        zstack->GetNwk()->SetNlmeRouteDiscoveryConfirmCallback(
            MakeBoundCallback(&NwkRouteDiscoveryConfirm, zstack));
    }

    for (auto i = zigbeeStacks.Begin(); i != zigbeeStacks.End(); i++)
    {
        int index = std::distance(zigbeeStacks.Begin(), i);
        Ptr<ZigbeeStack> zstack = *i;
        if (index == 0)
        {
            // 1 - Initiate the Zigbee coordinator, start the network
            NlmeNetworkFormationRequestParams netFormParams;
            netFormParams.m_scanChannelList.channelPageCount = 1;
            netFormParams.m_scanChannelList.channelsField[0] = 0x07FFF800; // 0x00000800;
            netFormParams.m_scanDuration = 0;
            netFormParams.m_superFrameOrder = 15;
            netFormParams.m_beaconOrder = 15;

            Simulator::ScheduleWithContext(zstack->GetNode()->GetId(),
                                           Seconds(index * 0.5),
                                           &ZigbeeNwk::NlmeNetworkFormationRequest,
                                           zstack->GetNwk(),
                                           netFormParams);
        }
        else
        {
            // 2- Let devices discovery the coordinator or routers and join the network, after
            //    this, it will become router itself(call to NLME-START-ROUTER.request). We
            //    continue doing the same with the rest of the devices which will discover the
            //    previously added routers and join the network
            NlmeNetworkDiscoveryRequestParams netDiscParams;
            netDiscParams.m_scanChannelList.channelPageCount = 1;
            netDiscParams.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
            netDiscParams.m_scanDuration = 2;

            Simulator::ScheduleWithContext(zstack->GetNode()->GetId(),
                                           Seconds(2 + index * 10), // 2+index * 0.5
                                           &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                           zstack->GetNwk(),
                                           netDiscParams);
        }
    }

    /*



        // 5- Find a route to the given device short address
        NlmeRouteDiscoveryRequestParams routeDiscParams;
        routeDiscParams.m_dstAddr = Mac16Address("0d:10");
        Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                       Seconds(8),
                                       &ZigbeeNwk::NlmeRouteDiscoveryRequest,
                                       zstack0->GetNwk(),
                                       routeDiscParams);

        // Print routing tables of coordinator (originator of route request) at
        // the end of the simulation

        Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
        Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                       Seconds(11),
                                       &ZigbeeNwk::PrintNeighborTable,
                                       zstack0->GetNwk(),
                                       stream);

        Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                       Seconds(11),
                                       &ZigbeeNwk::PrintRoutingTable,
                                       zstack0->GetNwk(),
                                       stream);

        Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                       Seconds(11),
                                       &ZigbeeNwk::PrintRouteDiscoveryTable,
                                       zstack0->GetNwk(),
                                       stream);*/

    Simulator::Stop(Seconds(1500));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
