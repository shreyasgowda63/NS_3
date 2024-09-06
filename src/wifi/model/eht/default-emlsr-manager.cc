/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "default-emlsr-manager.h"

#include "eht-frame-exchange-manager.h"

#include "ns3/boolean.h"
#include "ns3/channel-access-manager.h"
#include "ns3/log.h"
#include "ns3/qos-txop.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultEmlsrManager);

TypeId
DefaultEmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DefaultEmlsrManager")
            .SetParent<EmlsrManager>()
            .SetGroupName("Wifi")
            .AddConstructor<DefaultEmlsrManager>()
            .AddAttribute("SwitchAuxPhy",
                          "Whether Aux PHY should switch channel to operate on the link on which "
                          "the Main PHY was operating before moving to the link of the Aux PHY. "
                          "Note that, if the Aux PHY does not switch channel, the main PHY will "
                          "switch back to its previous link once the TXOP terminates (otherwise, "
                          "no PHY will be listening on that EMLSR link).",
                          BooleanValue(true),
                          MakeBooleanAccessor(&DefaultEmlsrManager::m_switchAuxPhy),
                          MakeBooleanChecker())
            .AddAttribute("PutAuxPhyToSleep",
                          "Whether Aux PHY should be put into sleep mode while the Main PHY "
                          "is operating on the same link as the Aux PHY (this only matters "
                          "when the Aux PHY does not switch channel).",
                          BooleanValue(true),
                          MakeBooleanAccessor(&DefaultEmlsrManager::m_auxPhyToSleep),
                          MakeBooleanChecker());
    return tid;
}

DefaultEmlsrManager::DefaultEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultEmlsrManager::~DefaultEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
DefaultEmlsrManager::DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);
}

uint8_t
DefaultEmlsrManager::GetLinkToSendEmlOmn()
{
    NS_LOG_FUNCTION(this);
    auto linkId = GetStaMac()->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT_MSG(linkId, "Link on which the main PHY is operating not found");
    return *linkId;
}

std::optional<uint8_t>
DefaultEmlsrManager::ResendNotification(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this);
    auto linkId = GetStaMac()->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT_MSG(linkId, "Link on which the main PHY is operating not found");
    return *linkId;
}

void
DefaultEmlsrManager::NotifyEmlsrModeChanged()
{
    NS_LOG_FUNCTION(this);
}

void
DefaultEmlsrManager::NotifyMainPhySwitch(uint8_t currLinkId, uint8_t nextLinkId, Time duration)
{
    NS_LOG_FUNCTION(this << currLinkId << nextLinkId << duration.As(Time::US));

    if (m_switchAuxPhy)
    {
        // switch channel on Aux PHY so that it operates on the link on which the main PHY was
        // operating
        SwitchAuxPhy(nextLinkId, currLinkId);
        return;
    }

    if (currLinkId != GetMainPhyId())
    {
        // the main PHY is leaving a non-primary link, hence an aux PHY needs to be reconnected
        NS_ASSERT_MSG(
            m_auxPhyToReconnect,
            "There should be an aux PHY to reconnect when the main PHY leaves a non-primary link");

        // the Aux PHY is not actually switching (hence no switching delay)
        GetStaMac()->NotifySwitchingEmlsrLink(m_auxPhyToReconnect, currLinkId, Seconds(0));
        // resume aux PHY from sleep (once reconnected to its original link)
        m_auxPhyToReconnect->ResumeFromSleep();
        SetCcaEdThresholdOnLinkSwitch(m_auxPhyToReconnect, currLinkId);
        m_auxPhyToReconnect = nullptr;
    }

    if (nextLinkId != GetMainPhyId())
    {
        // the main PHY is moving to a non-primary link and the aux PHY does not switch link
        m_auxPhyToReconnect = GetStaMac()->GetWifiPhy(nextLinkId);

        if (m_auxPhyToSleep)
        {
            // aux PHY can be put into sleep mode when the main PHY completes the channel switch
            m_auxPhyToSleepEvent =
                Simulator::Schedule(duration, &WifiPhy::SetSleepMode, m_auxPhyToReconnect);
        }
    }
}

Time
DefaultEmlsrManager::GetDelayUntilAccessRequest(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    return Time{0}; // start the TXOP
}

void
DefaultEmlsrManager::DoNotifyIcfReceived(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
DefaultEmlsrManager::DoNotifyUlTxopStart(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
DefaultEmlsrManager::DoNotifyTxopEnd(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // switch main PHY to the previous link, if needed
    if (!m_switchAuxPhy && m_auxPhyToReconnect)
    {
        auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

        // the main PHY may be switching at the end of a TXOP when, e.g., the main PHY starts
        // switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
        // is not received and the UL TXOP ends before the main PHY channel switch is completed.
        // In such cases, wait until the main PHY channel switch is completed before requesting
        // a new channel switch and cancel the event to put the aux PHY to sleep.
        // Backoff shall not be reset on the link left by the main PHY because a TXOP ended and
        // a new backoff value must be generated.
        // a new channel switch and cancel the event to put the aux PHY to sleep.
        if (!mainPhy->IsStateSwitching())
        {
            SwitchMainPhy(GetMainPhyId(), false, DONT_RESET_BACKOFF, REQUEST_ACCESS);
        }
        else
        {
            m_auxPhyToSleepEvent.Cancel();
            Simulator::Schedule(mainPhy->GetDelayUntilIdle(), [=, this]() {
                // request the main PHY to switch back to the primary link only if in the meantime
                // no TXOP started on another link (which will require the main PHY to switch link)
                if (!GetEhtFem(linkId)->UsingOtherEmlsrLink())
                {
                    SwitchMainPhy(GetMainPhyId(), false, DONT_RESET_BACKOFF, REQUEST_ACCESS);
                }
            });
        }
        return;
    }
}

bool
DefaultEmlsrManager::SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    NS_ASSERT_MSG(!m_auxPhyTxCapable,
                  "This function should only be called if aux PHY is not TX capable");

    // the aux PHY is not TX capable; check if main PHY has to switch to the aux PHY's link
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    // if the main PHY is idle, check whether the remaining backoff counter on at least an AC with
    // queued packets is greater than the main PHY channel switch delay
    auto backoffGreaterThanSwitchDelay = false;

    if (mainPhy->IsStateIdle())
    {
        auto mainPhyLinkId = GetStaMac()->GetLinkForPhy(mainPhy);
        NS_ASSERT(mainPhyLinkId.has_value());

        // update backoff on main PHY link for all ACs
        GetStaMac()
            ->GetChannelAccessManager(*mainPhyLinkId)
            ->NeedBackoffUponAccess(GetStaMac()->GetQosTxop(AC_BE),
                                    Txop::HAD_FRAMES_TO_TRANSMIT,
                                    Txop::CHECK_MEDIUM_BUSY);

        for (const auto& [aci, ac] : wifiAcList)
        {
            if (auto edca = GetStaMac()->GetQosTxop(aci); edca->HasFramesToTransmit(linkId))
            {
                auto backoffEnd =
                    GetStaMac()->GetChannelAccessManager(*mainPhyLinkId)->GetBackoffEndFor(edca);
                NS_LOG_DEBUG("Backoff end for " << aci
                                                << " on primary link: " << backoffEnd.As(Time::US));

                if (backoffEnd > Simulator::Now() + mainPhy->GetChannelSwitchDelay() +
                                     GetStaMac()->GetWifiPhy(linkId)->GetPifs())
                {
                    backoffGreaterThanSwitchDelay = true;
                    break;
                }
            }
        }
    }

    if ((mainPhy->IsStateCcaBusy() && !mainPhy->IsReceivingPhyHeader()) ||
        (mainPhy->IsStateIdle() && backoffGreaterThanSwitchDelay))
    {
        // switch main PHY
        SwitchMainPhy(linkId, false, RESET_BACKOFF, REQUEST_ACCESS);

        return true;
    }

    // Determine if and when we need to request channel access again for the aux PHY based on
    // the main PHY state.
    // Note that, if we have requested the main PHY to switch (above), the function has returned
    // and the EHT FEM will start a TXOP if the medium is idle for a PIFS interval following
    // the end of the main PHY channel switch.
    // If the state is switching, but we have not requested the main PHY to switch, then we
    // request channel access again for the aux PHY a PIFS after that the main PHY state is back
    // to IDLE (to avoid stealing the main PHY from the non-primary link which the main PHY is
    // switching to), and then we will determine if the main PHY has to switch link.
    // If the state is CCA_BUSY, the medium is busy but the main PHY is not receiving a PPDU.
    // In this case, we request channel access again for the aux PHY a PIFS after that the main
    // PHY state is back to IDLE, and then we will determine if the main PHY has to switch link.
    // If the state is TX or RX, it means that the main PHY is involved in a TXOP. In this
    // case, do nothing because the channel access will be requested when unblocking links
    // at the end of the TXOP.
    // If the state is IDLE, then either no AC has traffic to send or the backoff on the link
    // of the main PHY is shorter than the channel switch delay. In the former case, do
    // nothing because channel access will be triggered when new packets arrive; in the latter
    // case, do nothing because the main PHY will start a TXOP and at the end of such TXOP
    // links will be unblocked and the channel access requested on all links

    if (!mainPhy->IsStateSwitching() && !mainPhy->IsStateCcaBusy())
    {
        NS_LOG_DEBUG("Main PHY state is " << mainPhy->GetState()->GetState() << ". Do nothing");
        return false;
    }

    auto delay = mainPhy->GetDelayUntilIdle();
    NS_ASSERT(delay.IsStrictlyPositive());
    delay += mainPhy->GetSifs() + mainPhy->GetSlot();

    NS_LOG_DEBUG("Main PHY state is " << mainPhy->GetState()->GetState()
                                      << ". Schedule channel access request on link " << +linkId
                                      << " at time " << (Simulator::Now() + delay).As(Time::NS));
    Simulator::Schedule(delay, [=, this]() {
        for (const auto& [aci, ac] : wifiAcList)
        {
            auto edca = GetStaMac()->GetQosTxop(aci);
            if (edca->GetAccessStatus(linkId) != Txop::REQUESTED &&
                edca->HasFramesToTransmit(linkId))
            {
                NS_LOG_DEBUG("Request channel access on link " << +linkId << " for " << aci);
                GetStaMac()->GetChannelAccessManager(linkId)->RequestAccess(edca);
            }
        }
    });

    return false;
}

} // namespace ns3
