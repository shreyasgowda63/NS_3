/*
 * Copyright (c) 2019 Orange Labs
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
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#include "wifi-ppdu.h"

#include "wifi-phy-operating-channel.h"
#include "wifi-psdu.h"

#include "ns3/log.h"

namespace
{
/**
 * Get the center frequency (in MHz) of each segment covered by the provided channel width (in
 * MHz). If the specified channel width is contained in a single frequency segment, a single
 * center frequency is returned. If the specified channel width is spread over multiple
 * frequency segments (e.g. 160 MHz if operating channel is 80+80MHz), multiple center
 * frequencies are returned.
 *
 * @param channel the operating channel of the PHY
 * @param channelWidth the channel width in MHz
 * @return the center frequency (in MHz) of each segment covered by the given width
 */
std::vector<uint16_t>
GetChannelCenterFrequenciesPerSegment(const ns3::WifiPhyOperatingChannel& channel,
                                      ns3::ChannelWidthMhz channelWidth)
{
    if (!channel.IsSet())
    {
        return {};
    }
    std::vector<uint16_t> freqs{};
    const auto width = std::min(channelWidth, channel.GetWidth(0));
    const auto primarySegmentIndex = channel.GetPrimarySegmentIndex(width);
    const auto secondarySegmentIndex = channel.GetSecondarySegmentIndex(width);
    const auto primaryIndex = channel.GetPrimaryChannelIndex(channelWidth);
    const auto segmentIndices =
        ((channel.GetNSegments() < 2) || (channelWidth <= channel.GetWidth(primarySegmentIndex)))
            ? std::vector<uint8_t>{primarySegmentIndex}
            : std::vector<uint8_t>{primarySegmentIndex, secondarySegmentIndex};
    for (auto segmentIndex : segmentIndices)
    {
        const auto segmentFrequency = channel.GetFrequency(segmentIndex);
        const auto segmentWidth = channel.GetWidth(segmentIndex);
        const auto segmentOffset = (primarySegmentIndex * (segmentWidth / channelWidth));
        const auto freq =
            segmentFrequency - (segmentWidth / 2.) + (primaryIndex - segmentOffset + 0.5) * width;
        freqs.push_back(freq);
    }
    return freqs;
}
} // namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPpdu");

WifiPpdu::WifiPpdu(Ptr<const WifiPsdu> psdu,
                   const WifiTxVector& txVector,
                   const WifiPhyOperatingChannel& channel,
                   uint64_t uid /* = UINT64_MAX */)
    : m_preamble(txVector.GetPreambleType()),
      m_modulation(txVector.IsValid() ? txVector.GetModulationClass() : WIFI_MOD_CLASS_UNKNOWN),
      m_txCenterFreqs(GetChannelCenterFrequenciesPerSegment(channel, txVector.GetChannelWidth())),
      m_uid(uid),
      m_txVector(txVector),
      m_operatingChannel(channel),
      m_truncatedTx(false),
      m_txPowerLevel(txVector.GetTxPowerLevel()),
      m_txAntennas(txVector.GetNTx()),
      m_txChannelWidth(txVector.GetChannelWidth())
{
    NS_LOG_FUNCTION(this << *psdu << txVector << channel << uid);
    m_psdus.insert(std::make_pair(SU_STA_ID, psdu));
}

WifiPpdu::WifiPpdu(const WifiConstPsduMap& psdus,
                   const WifiTxVector& txVector,
                   const WifiPhyOperatingChannel& channel,
                   uint64_t uid)
    : m_preamble(txVector.GetPreambleType()),
      m_modulation(txVector.IsValid() ? txVector.GetMode(psdus.begin()->first).GetModulationClass()
                                      : WIFI_MOD_CLASS_UNKNOWN),
      m_txCenterFreqs(GetChannelCenterFrequenciesPerSegment(channel, txVector.GetChannelWidth())),
      m_uid(uid),
      m_txVector(txVector),
      m_operatingChannel(channel),
      m_truncatedTx(false),
      m_txPowerLevel(txVector.GetTxPowerLevel()),
      m_txAntennas(txVector.GetNTx()),
      m_txChannelWidth(txVector.GetChannelWidth())
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << uid);
    m_psdus = psdus;
}

WifiPpdu::~WifiPpdu()
{
    for (auto& psdu : m_psdus)
    {
        psdu.second = nullptr;
    }
    m_psdus.clear();
}

const WifiTxVector&
WifiPpdu::GetTxVector() const
{
    if (!m_txVector.has_value())
    {
        m_txVector = DoGetTxVector();
        m_txVector->SetTxPowerLevel(m_txPowerLevel);
        m_txVector->SetNTx(m_txAntennas);
        m_txVector->SetChannelWidth(m_txChannelWidth);
    }
    return m_txVector.value();
}

WifiTxVector
WifiPpdu::DoGetTxVector() const
{
    NS_FATAL_ERROR("This method should not be called for the base WifiPpdu class. Use the "
                   "overloaded version in the amendment-specific PPDU subclasses instead!");
    return WifiTxVector(); // should be overloaded
}

void
WifiPpdu::ResetTxVector() const
{
    NS_LOG_FUNCTION(this);
    m_txVector.reset();
}

void
WifiPpdu::UpdateTxVector(const WifiTxVector& updatedTxVector) const
{
    NS_LOG_FUNCTION(this << updatedTxVector);
    ResetTxVector();
    m_txVector = updatedTxVector;
}

Ptr<const WifiPsdu>
WifiPpdu::GetPsdu() const
{
    return m_psdus.begin()->second;
}

bool
WifiPpdu::IsTruncatedTx() const
{
    return m_truncatedTx;
}

void
WifiPpdu::SetTruncatedTx()
{
    NS_LOG_FUNCTION(this);
    m_truncatedTx = true;
}

WifiModulationClass
WifiPpdu::GetModulation() const
{
    return m_modulation;
}

ChannelWidthMhz
WifiPpdu::GetTxChannelWidth() const
{
    return m_txChannelWidth;
}

std::vector<uint16_t>
WifiPpdu::GetTxCenterFreqs() const
{
    return m_txCenterFreqs;
}

bool
WifiPpdu::DoesOverlapChannel(uint16_t minFreq, uint16_t maxFreq) const
{
    NS_LOG_FUNCTION(this << minFreq << maxFreq);
    // all segments have the same width
    const auto txChannelWidth = (m_txChannelWidth / m_txCenterFreqs.size());
    for (auto txCenterFreq : m_txCenterFreqs)
    {
        const auto minTxFreq = txCenterFreq - txChannelWidth / 2;
        const auto maxTxFreq = txCenterFreq + txChannelWidth / 2;
        /**
         * The PPDU does not overlap the channel in two cases.
         *
         * First non-overlapping case:
         *
         *                                        ┌─────────┐
         *                                PPDU    │ Nominal │
         *                                        │  Band   │
         *                                        └─────────┘
         *                                   minTxFreq   maxTxFreq
         *
         *       minFreq                       maxFreq
         *         ┌──────────────────────────────┐
         *         │           Channel            │
         *         └──────────────────────────────┘
         *
         * Second non-overlapping case:
         *
         *         ┌─────────┐
         * PPDU    │ Nominal │
         *         │  Band   │
         *         └─────────┘
         *    minTxFreq   maxTxFreq
         *
         *                 minFreq                       maxFreq
         *                   ┌──────────────────────────────┐
         *                   │           Channel            │
         *                   └──────────────────────────────┘
         */
        if ((minTxFreq < maxFreq) && (maxTxFreq > minFreq))
        {
            return true;
        }
    }
    return false;
}

uint64_t
WifiPpdu::GetUid() const
{
    return m_uid;
}

WifiPreamble
WifiPpdu::GetPreamble() const
{
    return m_preamble;
}

WifiPpduType
WifiPpdu::GetType() const
{
    return WIFI_PPDU_TYPE_SU;
}

uint16_t
WifiPpdu::GetStaId() const
{
    return SU_STA_ID;
}

Time
WifiPpdu::GetTxDuration() const
{
    NS_FATAL_ERROR("This method should not be called for the base WifiPpdu class. Use the "
                   "overloaded version in the amendment-specific PPDU subclasses instead!");
    return MicroSeconds(0); // should be overloaded
}

void
WifiPpdu::Print(std::ostream& os) const
{
    os << "[ preamble=" << m_preamble << ", modulation=" << m_modulation
       << ", truncatedTx=" << (m_truncatedTx ? "Y" : "N") << ", UID=" << m_uid << ", "
       << PrintPayload() << "]";
}

std::string
WifiPpdu::PrintPayload() const
{
    std::ostringstream ss;
    ss << "PSDU=" << GetPsdu() << " ";
    return ss.str();
}

Ptr<WifiPpdu>
WifiPpdu::Copy() const
{
    NS_FATAL_ERROR("This method should not be called for the base WifiPpdu class. Use the "
                   "overloaded version in the amendment-specific PPDU subclasses instead!");
    return Ptr<WifiPpdu>(new WifiPpdu(*this), false);
}

std::ostream&
operator<<(std::ostream& os, const Ptr<const WifiPpdu>& ppdu)
{
    ppdu->Print(os);
    return os;
}

std::ostream&
operator<<(std::ostream& os, const WifiConstPsduMap& psdus)
{
    for (const auto& psdu : psdus)
    {
        os << "PSDU for STA_ID=" << psdu.first << " (" << *psdu.second << ") ";
    }
    return os;
}

} // namespace ns3
