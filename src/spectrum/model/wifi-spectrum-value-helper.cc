/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 * Copyright (c) 2017 Orange Labs
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Giuseppe Piro  <g.piro@poliba.it>
 *          Rediet <getachew.redieteab@orange.com>
 */

#include <map>
#include <cmath>
#include "wifi-spectrum-value-helper.h"
#include "ns3/log.h"
#include "ns3/fatal-error.h"
#include "ns3/assert.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiSpectrumValueHelper");

///< Wifi Spectrum Model structure
struct WifiSpectrumModelId
{
  /**
   * Constructor
   * \param centerFrequency the center frequency (in MHz)
   * \param channelWidth the channel width (in MHz)
   * \param granularity the granularity of each band (in Hz)
   * \param guardBandwidth the guard band width (in MHz)
   */
  WifiSpectrumModelId (uint32_t centerFrequency, uint16_t channelWidth,
                       uint32_t granularity, uint16_t guardBandwidth);
  uint32_t m_centerFrequency;  ///< center frequency (in MHz)
  uint16_t m_channelWidth;     ///< channel width (in MHz)
  uint32_t m_granularity;      ///< granularity of each band (in Hz)
  uint16_t m_guardBandwidth;   ///< guard band width (in MHz)
};

WifiSpectrumModelId::WifiSpectrumModelId (uint32_t centerFrequency, uint16_t channelWidth,
                                          uint32_t granularity, uint16_t guardBandwidth)
  : m_centerFrequency (centerFrequency),
    m_channelWidth (channelWidth),
    m_granularity (granularity),
    m_guardBandwidth (guardBandwidth)
{
  NS_LOG_FUNCTION (this << centerFrequency << channelWidth << granularity << guardBandwidth);
}

/**
 * Less than operator
 * \param a the first wifi spectrum to compare
 * \param b the second wifi spectrum to compare
 * \returns true if the first spectrum is less than the second spectrum
 */
bool
operator < (const WifiSpectrumModelId& a, const WifiSpectrumModelId& b)
{
  return ( (a.m_centerFrequency < b.m_centerFrequency)
           || ((a.m_centerFrequency == b.m_centerFrequency)
               && (a.m_channelWidth < b.m_channelWidth))
           || ((a.m_centerFrequency == b.m_centerFrequency)
               && (a.m_channelWidth == b.m_channelWidth)
               && (a.m_granularity < b.m_granularity)) // to cover coexistence of 11ax with legacy case
           || ((a.m_centerFrequency == b.m_centerFrequency)
               && (a.m_channelWidth == b.m_channelWidth)
               && (a.m_granularity == b.m_granularity)
               && (a.m_guardBandwidth < b.m_guardBandwidth))); // to cover 2.4 GHz case, where DSSS coexists with OFDM
}

static std::map<WifiSpectrumModelId, Ptr<SpectrumModel> > g_wifiSpectrumModelMap; ///< static initializer for the class

Ptr<SpectrumModel>
WifiSpectrumValueHelper::GetSpectrumModel (uint32_t centerFrequency, uint16_t channelWidth, uint32_t granularity,
                                           bool includeAdjacentChannelPower)
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << granularity << includeAdjacentChannelPower);
  bool extraDcSubBand = true;
  if (granularity == GetGranularityForChannelSpacing (centerFrequency)) //channel spacing is to be used
    {
      //Adapt parameters to retrieve a model with channel spacing granularity
      channelWidth = (channelWidth == 22) ? 20 : channelWidth; //use 20 MHz bandwidth for DSSS
      extraDcSubBand = false; //no extra sub-band will be added for DC
    }
  uint32_t guardBandwidth = includeAdjacentChannelPower ? channelWidth : 0;

  Ptr<SpectrumModel> ret;
  WifiSpectrumModelId key (centerFrequency, channelWidth, granularity, guardBandwidth);
  std::map<WifiSpectrumModelId, Ptr<SpectrumModel> >::iterator it = g_wifiSpectrumModelMap.find (key);
  if (it != g_wifiSpectrumModelMap.end ())
    {
      ret = it->second;
    }
  else
    {
      Bands bands;
      double centerFrequencyHz = centerFrequency * 1e6;
      double bandwidth = (channelWidth + (2.0 * guardBandwidth)) * 1e6;
      double startingFrequencyHz = centerFrequencyHz;
      // For OFDM, the center subcarrier is null (at center frequency)
      uint32_t numBands = static_cast<uint32_t> ((bandwidth / granularity) + 0.5);
      NS_ASSERT (numBands > 0);
      if (extraDcSubBand)
        {
          if (numBands % 2 == 0)
            {
              // round up to the nearest odd number of subbands so that bands
              // are symmetric around center frequency
              numBands += 1;
            }
          NS_ASSERT_MSG (numBands % 2 == 1, "Number of bands should be odd");
          // lay down numBands/2 bands symmetrically around center frequency
          // and place an additional band at center frequency
          startingFrequencyHz -= (numBands / 2 * granularity) + granularity / 2;
        }
      else
        {
          startingFrequencyHz -= bandwidth / 2;
        }
      NS_LOG_DEBUG ("Num bands " << numBands << ", granularity: " << granularity << " Hz");

      for (size_t i = 0; i < numBands; i++)
        {
          BandInfo info;
          double f = startingFrequencyHz + (i * granularity);
          info.fl = f;
          f += granularity / 2;
          info.fc = f;
          f += granularity / 2;
          info.fh = f;
          NS_LOG_DEBUG ("creating band " << i << " (" << info.fl << ":" << info.fc << ":" << info.fh << ")");
          bands.push_back (info);
        }
      ret = Create<SpectrumModel> (bands);
      g_wifiSpectrumModelMap.insert (std::pair<WifiSpectrumModelId, Ptr<SpectrumModel> > (key, ret));
    }
  NS_LOG_LOGIC ("returning SpectrumModel::GetUid () == " << ret->GetUid ());
  return ret;
}

// Power allocated to 71 center subbands out of 135 total subbands in the band
Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateDsssTxPowerSpectralDensity (uint32_t centerFrequency, uint32_t granularity,
                                                           double txPowerW, bool includeAdjacentChannelPower)
{
  NS_LOG_FUNCTION (centerFrequency << granularity << txPowerW << includeAdjacentChannelPower);
  uint16_t channelWidth = 22;  // DSSS channels are 22 MHz wide
  if (granularity == GetGranularityForChannelSpacing (centerFrequency)) //channel spacing is to be used
    {
      return CreateTxPowerSpectralDensityForChannelSpacingGranularity (centerFrequency, 20 /*use 20 MHz channel for DSSS*/, granularity, txPowerW);
    }
  uint32_t guardBandwidth = includeAdjacentChannelPower ? channelWidth : 0;
  Ptr<SpectrumValue> c = Create<SpectrumValue> (GetSpectrumModel (centerFrequency, channelWidth, granularity, includeAdjacentChannelPower));
  Values::iterator vit = c->ValuesBegin ();
  Bands::const_iterator bit = c->ConstBandsBegin ();
  uint32_t nGuardBands = static_cast<uint32_t> (((2 * guardBandwidth * 1e6) / granularity) + 0.5);
  uint32_t nAllocatedBands = static_cast<uint32_t> (((channelWidth * 1e6) / granularity) + 0.5);
  NS_ASSERT (c->GetSpectrumModel ()->GetNumBands () == (nAllocatedBands + nGuardBands + 1));
  // Evenly spread power across 22 MHz
  double txPowerPerBand = txPowerW / nAllocatedBands;
  for (size_t i = 0; i < c->GetSpectrumModel ()->GetNumBands (); i++, vit++, bit++)
    {
      if ((i >= (nGuardBands / 2)) && (i <= ((nGuardBands / 2) + nAllocatedBands - 1)))
        {
          *vit = txPowerPerBand / (bit->fh - bit->fl);
        }
    }
  return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity (uint32_t centerFrequency, uint16_t channelWidth,
                                                           uint32_t granularity, double txPowerW, bool includeAdjacentChannelPower,
                                                           double minInnerBandDbr, double minOuterBandDbr, double lowestPointDbr)
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << granularity << txPowerW << includeAdjacentChannelPower <<
                   minInnerBandDbr << minOuterBandDbr << lowestPointDbr << includeAdjacentChannelPower);
  if (granularity == GetGranularityForChannelSpacing (centerFrequency)) //channel spacing is to be used
    {
      return CreateTxPowerSpectralDensityForChannelSpacingGranularity (centerFrequency, channelWidth, granularity, txPowerW);
    }
  uint32_t guardBandwidth = includeAdjacentChannelPower ? channelWidth : 0;
  uint32_t innerSlopeWidth = 0;
  switch (channelWidth)
    {
    case 20:
      innerSlopeWidth = static_cast<uint32_t> ((2e6 / granularity) + 0.5); // [-11;-9] & [9;11]
      break;
    case 10:
      innerSlopeWidth = static_cast<uint32_t> ((1e6 / granularity) + 0.5); // [-5.5;-4.5] & [4.5;5.5]
      break;
    case 5:
      innerSlopeWidth = static_cast<uint32_t> ((5e5 / granularity) + 0.5); // [-2.75;-2.5] & [2.5;2.75]
      break;
    default:
      NS_FATAL_ERROR ("Channel width " << channelWidth << " should be correctly set.");
      return 0;
    }

  Ptr<SpectrumValue> c = Create<SpectrumValue> (GetSpectrumModel (centerFrequency, channelWidth, granularity, includeAdjacentChannelPower));
  uint32_t nGuardBands = static_cast<uint32_t> (((2 * guardBandwidth * 1e6) / granularity) + 0.5);
  uint32_t nAllocatedBands = static_cast<uint32_t> (((channelWidth * 1e6) / granularity) + 0.5);
  NS_ASSERT_MSG (c->GetSpectrumModel ()->GetNumBands () == (nAllocatedBands + nGuardBands + 1), "Unexpected number of bands " << c->GetSpectrumModel ()->GetNumBands ());
  // 52 subcarriers (48 data + 4 pilot)
  // skip guard band and 6 subbands, then place power in 26 subbands, then
  // skip the center subband, then place power in 26 subbands, then skip
  // the final 6 subbands and the guard band.
  double txPowerPerBandW = txPowerW / 52;
  NS_LOG_DEBUG ("Power per band " << txPowerPerBandW << "W");
  uint32_t start1 = (nGuardBands / 2) + 6;
  uint32_t stop1 = start1 + 26 - 1;
  uint32_t start2 = stop1 + 2;
  uint32_t stop2 = start2 + 26 - 1;

  //Build transmit spectrum mask
  std::vector <WifiSpectrumBand> subBands;
  subBands.push_back (std::make_pair (start1, stop1));
  subBands.push_back (std::make_pair (start2, stop2));
  WifiSpectrumBand maskBand (0, nAllocatedBands + nGuardBands);
  CreateSpectrumMaskForOfdm (c, subBands, maskBand,
                             txPowerPerBandW, nGuardBands,
                             innerSlopeWidth, minInnerBandDbr,
                             minOuterBandDbr, lowestPointDbr);
  NormalizeSpectrumMask (c, txPowerW);
  NS_ASSERT_MSG (std::abs (txPowerW - Integral (*c)) < 1e-6, "Power allocation failed");
  return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity (uint32_t centerFrequency, uint16_t channelWidth,
                                                             uint32_t granularity, double txPowerW, bool includeAdjacentChannelPower,
                                                             double minInnerBandDbr, double minOuterBandDbr, double lowestPointDbr)
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << granularity << txPowerW << includeAdjacentChannelPower <<
                   minInnerBandDbr << minOuterBandDbr << lowestPointDbr << includeAdjacentChannelPower);
  if (granularity == GetGranularityForChannelSpacing (centerFrequency)) //channel spacing is to be used
    {
      return CreateTxPowerSpectralDensityForChannelSpacingGranularity (centerFrequency, channelWidth, granularity, txPowerW);
    }
  uint32_t guardBandwidth = includeAdjacentChannelPower ? channelWidth : 0;
  Ptr<SpectrumValue> c = Create<SpectrumValue> (GetSpectrumModel (centerFrequency, channelWidth, granularity, includeAdjacentChannelPower));
  uint32_t nGuardBands = static_cast<uint32_t> (((2 * guardBandwidth * 1e6) / granularity) + 0.5);
  uint32_t nAllocatedBands = static_cast<uint32_t> (((channelWidth * 1e6) / granularity) + 0.5);
  NS_ASSERT_MSG (c->GetSpectrumModel ()->GetNumBands () == (nAllocatedBands + nGuardBands + 1), "Unexpected number of bands " << c->GetSpectrumModel ()->GetNumBands ());
  double txPowerPerBandW = 0.0;
  // skip the guard band and 4 subbands, then place power in 28 subbands, then
  // skip the center subband, then place power in 28 subbands, then skip
  // the final 4 subbands and the guard band.
  // Repeat for each 20 MHz band.
  uint32_t start1 = (nGuardBands / 2) + 4;
  uint32_t stop1 = start1 + 28 - 1;
  uint32_t start2 = stop1 + 2;
  uint32_t stop2 = start2 + 28 - 1;
  uint32_t start3 = stop2 + (2 * 4);
  uint32_t stop3 = start3 + 28 - 1;
  uint32_t start4 = stop3 + 2;
  uint32_t stop4 = start4 + 28 - 1;
  uint32_t start5 = stop4 + (2 * 4);
  uint32_t stop5 = start5 + 28 - 1;
  uint32_t start6 = stop5 + 2;
  uint32_t stop6 = start6 + 28 - 1;
  uint32_t start7 = stop6 + (2 * 4);
  uint32_t stop7 = start7 + 28 - 1;
  uint32_t start8 = stop7 + 2;
  uint32_t stop8 = start8 + 28 - 1;
  uint32_t start9 = stop8 + (2 * 4);
  uint32_t stop9 = start9 + 28 - 1;
  uint32_t start10 = stop9 + 2;
  uint32_t stop10 = start10 + 28 - 1;
  uint32_t start11 = stop10 + (2 * 4);
  uint32_t stop11 = start11 + 28 - 1;
  uint32_t start12 = stop11 + 2;
  uint32_t stop12 = start12 + 28 - 1;
  uint32_t start13 = stop12 + (2 * 4);
  uint32_t stop13 = start13 + 28 - 1;
  uint32_t start14 = stop13 + 2;
  uint32_t stop14 = start14 + 28 - 1;
  uint32_t start15 = stop14 + (2 * 4);
  uint32_t stop15 = start15 + 28 - 1;
  uint32_t start16 = stop15 + 2;
  uint32_t stop16 = start16 + 28 - 1;
  //Prepare spectrum mask specific variables
  uint32_t innerSlopeWidth = static_cast<uint32_t> ((2e6 / granularity) + 0.5); //size in number of subcarriers of the inner band (2MHz for HT/VHT)
  std::vector <WifiSpectrumBand> subBands; //list of data/pilot-containing subBands (sent at 0dBr)
  WifiSpectrumBand maskBand (0, nAllocatedBands + nGuardBands);
  switch (channelWidth)
    {
    case 20:
      // 56 subcarriers (52 data + 4 pilot)
      txPowerPerBandW = txPowerW / 56;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      break;
    case 40:
      // 112 subcarriers (104 data + 8 pilot)
      // possible alternative:  114 subcarriers (108 data + 6 pilot)
      txPowerPerBandW = txPowerW / 112;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      subBands.push_back (std::make_pair (start3, stop3));
      subBands.push_back (std::make_pair (start4, stop4));
      break;
    case 80:
      // 224 subcarriers (208 data + 16 pilot)
      // possible alternative:  242 subcarriers (234 data + 8 pilot)
      txPowerPerBandW = txPowerW / 224;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      subBands.push_back (std::make_pair (start3, stop3));
      subBands.push_back (std::make_pair (start4, stop4));
      subBands.push_back (std::make_pair (start5, stop5));
      subBands.push_back (std::make_pair (start6, stop6));
      subBands.push_back (std::make_pair (start7, stop7));
      subBands.push_back (std::make_pair (start8, stop8));
      break;
    case 160:
      // 448 subcarriers (416 data + 32 pilot)
      // possible alternative:  484 subcarriers (468 data + 16 pilot)
      txPowerPerBandW = txPowerW / 448;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      subBands.push_back (std::make_pair (start3, stop3));
      subBands.push_back (std::make_pair (start4, stop4));
      subBands.push_back (std::make_pair (start5, stop5));
      subBands.push_back (std::make_pair (start6, stop6));
      subBands.push_back (std::make_pair (start7, stop7));
      subBands.push_back (std::make_pair (start8, stop8));
      subBands.push_back (std::make_pair (start9, stop9));
      subBands.push_back (std::make_pair (start10, stop10));
      subBands.push_back (std::make_pair (start11, stop11));
      subBands.push_back (std::make_pair (start12, stop12));
      subBands.push_back (std::make_pair (start13, stop13));
      subBands.push_back (std::make_pair (start14, stop14));
      subBands.push_back (std::make_pair (start15, stop15));
      subBands.push_back (std::make_pair (start16, stop16));
      break;
    }

  //Build transmit spectrum mask
  CreateSpectrumMaskForOfdm (c, subBands, maskBand,
                             txPowerPerBandW, nGuardBands,
                             innerSlopeWidth, minInnerBandDbr,
                             minOuterBandDbr, lowestPointDbr);
  NormalizeSpectrumMask (c, txPowerW);
  NS_ASSERT_MSG (std::abs (txPowerW - Integral (*c)) < 1e-6, "Power allocation failed");
  return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (uint32_t centerFrequency, uint16_t channelWidth,
                                                             uint32_t granularity, double txPowerW, bool includeAdjacentChannelPower,
                                                             double minInnerBandDbr, double minOuterBandDbr, double lowestPointDbr)
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << granularity << txPowerW << includeAdjacentChannelPower <<
                   minInnerBandDbr << minOuterBandDbr << lowestPointDbr);
  if (granularity == GetGranularityForChannelSpacing (centerFrequency)) //channel spacing is to be used
    {
      return CreateTxPowerSpectralDensityForChannelSpacingGranularity (centerFrequency, channelWidth, granularity, txPowerW);
    }
  uint32_t guardBandwidth = includeAdjacentChannelPower ? channelWidth : 0;
  Ptr<SpectrumValue> c = Create<SpectrumValue> (GetSpectrumModel (centerFrequency, channelWidth, granularity, includeAdjacentChannelPower));
  uint32_t nGuardBands = static_cast<uint32_t> (((2 * guardBandwidth * 1e6) / granularity) + 0.5);
  uint32_t nAllocatedBands = static_cast<uint32_t> (((channelWidth * 1e6) / granularity) + 0.5);
  NS_ASSERT_MSG (c->GetSpectrumModel ()->GetNumBands () == (nAllocatedBands + nGuardBands + 1), "Unexpected number of bands " << c->GetSpectrumModel ()->GetNumBands ());
  double txPowerPerBandW = 0.0;
  uint32_t start1;
  uint32_t stop1;
  uint32_t start2;
  uint32_t stop2;
  uint32_t start3;
  uint32_t stop3;
  uint32_t start4;
  uint32_t stop4;
  //Prepare spectrum mask specific variables
  uint32_t innerSlopeWidth = static_cast<uint32_t> ((1e6 / granularity) + 0.5); //size in number of subcarriers of the inner band
  std::vector <WifiSpectrumBand> subBands; //list of data/pilot-containing subBands (sent at 0dBr)
  WifiSpectrumBand maskBand (0, nAllocatedBands + nGuardBands);
  switch (channelWidth)
    {
    case 20:
      // 242 subcarriers (234 data + 8 pilot)
      txPowerPerBandW = txPowerW / 242;
      innerSlopeWidth = static_cast<uint32_t> ((5e5 / granularity) + 0.5); // [-10.25;-9.75] & [9.75;10.25]
      // skip the guard band and 6 subbands, then place power in 121 subbands, then
      // skip 3 DC, then place power in 121 subbands, then skip
      // the final 5 subbands and the guard band.
      start1 = (nGuardBands / 2) + 6;
      stop1 = start1 + 121 - 1;
      start2 = stop1 + 4;
      stop2 = start2 + 121 - 1;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      break;
    case 40:
      // 484 subcarriers (468 data + 16 pilot)
      txPowerPerBandW = txPowerW / 484;
      // skip the guard band and 12 subbands, then place power in 242 subbands, then
      // skip 5 DC, then place power in 242 subbands, then skip
      // the final 11 subbands and the guard band.
      start1 = (nGuardBands / 2) + 12;
      stop1 = start1 + 242 - 1;
      start2 = stop1 + 6;
      stop2 = start2 + 242 - 1;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      break;
    case 80:
      // 996 subcarriers (980 data + 16 pilot)
      txPowerPerBandW = txPowerW / 996;
      // skip the guard band and 12 subbands, then place power in 498 subbands, then
      // skip 5 DC, then place power in 498 subbands, then skip
      // the final 11 subbands and the guard band.
      start1 = (nGuardBands / 2) + 12;
      stop1 = start1 + 498 - 1;
      start2 = stop1 + 6;
      stop2 = start2 + 498 - 1;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      break;
    case 160:
      // 2 x 996 subcarriers (2 x 80 MHZ bands)
      txPowerPerBandW = txPowerW / (2 * 996);
      start1 = (nGuardBands / 2) + 12;
      stop1 = start1 + 498 - 1;
      start2 = stop1 + 6;
      stop2 = start2 + 498 - 1;
      start3 = stop2 + (2 * 12);
      stop3 = start3 + 498 - 1;
      start4 = stop3 + 6;
      stop4 = start4 + 498 - 1;
      subBands.push_back (std::make_pair (start1, stop1));
      subBands.push_back (std::make_pair (start2, stop2));
      subBands.push_back (std::make_pair (start3, stop3));
      subBands.push_back (std::make_pair (start4, stop4));
      break;
    default:
      NS_FATAL_ERROR ("ChannelWidth " << channelWidth << " unsupported");
      break;
    }

  //Build transmit spectrum mask
  CreateSpectrumMaskForOfdm (c, subBands, maskBand,
                             txPowerPerBandW, nGuardBands,
                             innerSlopeWidth, minInnerBandDbr,
                             minOuterBandDbr, lowestPointDbr);
  NormalizeSpectrumMask (c, txPowerW);
  NS_ASSERT_MSG (std::abs (txPowerW - Integral (*c)) < 1e-6, "Power allocation failed");
  return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity (uint32_t centerFrequency, uint16_t channelWidth, uint32_t granularity,
                                                               double txPowerW, bool includeAdjacentChannelPower,
                                                               WifiSpectrumBand ru)
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << granularity << txPowerW << includeAdjacentChannelPower << ru.first << ru.second);
  if (granularity == GetGranularityForChannelSpacing (centerFrequency)) //channel spacing is to be used
    {
      NS_FATAL_ERROR ("OFDMA is not supported with channel-spacing-based granularity");
    }
  Ptr<SpectrumValue> c = Create<SpectrumValue> (GetSpectrumModel (centerFrequency, channelWidth, granularity, includeAdjacentChannelPower));

  //Build spectrum mask
  Values::iterator vit = c->ValuesBegin ();
  Bands::const_iterator bit = c->ConstBandsBegin ();
  double txPowerPerBandW = (txPowerW / (ru.second - ru.first + 1)); //FIXME: null subcarriers
  uint32_t numBands = c->GetSpectrumModel ()->GetNumBands ();
  for (size_t i = 0; i < numBands; i++, vit++, bit++)
    {
      if (i < ru.first || i > ru.second) //outside the spectrum mask
        {
          *vit = 0.0;
        }
      else
        {
          *vit = (txPowerPerBandW / (bit->fh - bit->fl));
        }
    }
  
  return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateNoisePowerSpectralDensity (uint32_t centerFrequency, uint16_t channelWidth, uint32_t granularity,
                                                          double noiseFigure, bool includeAdjacentChannelPower)
{
  Ptr<SpectrumModel> model = GetSpectrumModel (centerFrequency, channelWidth, granularity, includeAdjacentChannelPower);
  return CreateNoisePowerSpectralDensity (noiseFigure, model);
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateNoisePowerSpectralDensity (double noiseFigureDb, Ptr<SpectrumModel> spectrumModel)
{
  NS_LOG_FUNCTION (noiseFigureDb << spectrumModel);

  // see "LTE - From theory to practice"
  // Section 22.4.4.2 Thermal Noise and Receiver Noise Figure
  const double kT_dBm_Hz = -174.0;  // dBm/Hz
  double kT_W_Hz = DbmToW (kT_dBm_Hz);
  double noiseFigureLinear = std::pow (10.0, noiseFigureDb / 10.0);
  double noisePowerSpectralDensity =  kT_W_Hz * noiseFigureLinear;

  Ptr<SpectrumValue> noisePsd = Create <SpectrumValue> (spectrumModel);
  (*noisePsd) = noisePowerSpectralDensity;
  NS_LOG_INFO ("NoisePowerSpectralDensity has integrated power of " << Integral (*noisePsd));
  return noisePsd;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateRfFilter (uint32_t centerFrequency, uint16_t totalChannelWidth,
                                         uint32_t granularity, bool includeAdjacentChannelPower, WifiSpectrumBand band)
{
  uint32_t startIndex = band.first;
  uint32_t stopIndex = band.second;
  NS_LOG_FUNCTION (centerFrequency << totalChannelWidth << granularity << includeAdjacentChannelPower << startIndex << stopIndex);
  Ptr<SpectrumValue> c = Create <SpectrumValue> (GetSpectrumModel (centerFrequency, totalChannelWidth, granularity, includeAdjacentChannelPower));
  Bands::const_iterator bit = c->ConstBandsBegin ();
  Values::iterator vit = c->ValuesBegin ();
  vit += startIndex;
  bit += startIndex;
  for (size_t i = startIndex; i <= stopIndex; i++, vit++, bit++)
    {
      *vit = 1;
    }
  NS_LOG_LOGIC ("Added subbands " << startIndex << " to " << stopIndex << " to filter");
  return c;
}

void
WifiSpectrumValueHelper::CreateSpectrumMaskForOfdm (Ptr<SpectrumValue> c, std::vector <WifiSpectrumBand> allocatedSubBands, WifiSpectrumBand maskBand,
                                                    double txPowerPerBandW, uint32_t nGuardBands, uint32_t innerSlopeWidth,
                                                    double minInnerBandDbr, double minOuterBandDbr, double lowestPointDbr)
{
  NS_LOG_FUNCTION (c << allocatedSubBands.front ().first << allocatedSubBands.back ().second << maskBand.first << maskBand.second <<
                   txPowerPerBandW << nGuardBands << innerSlopeWidth << minInnerBandDbr << minOuterBandDbr << lowestPointDbr);
  uint32_t numSubBands = allocatedSubBands.size ();
  uint32_t numBands = c->GetSpectrumModel ()->GetNumBands ();
  uint32_t numMaskBands = maskBand.second - maskBand.first + 1;
  NS_ASSERT (numSubBands && numBands && numMaskBands);
  NS_LOG_LOGIC ("Power per band " << txPowerPerBandW << "W");

  //Different power levels
  double txPowerRefDbm = (10.0 * std::log10 (txPowerPerBandW * 1000.0));
  double txPowerInnerBandMinDbm = txPowerRefDbm + minInnerBandDbr;
  double txPowerMiddleBandMinDbm = txPowerRefDbm + minOuterBandDbr;
  double txPowerOuterBandMinDbm = txPowerRefDbm + lowestPointDbr; //TODO also take into account dBm/MHz constraints

  //Different widths (in number of bands)
  const bool isGuardBand = nGuardBands > 0;
  uint32_t outerSlopeWidth = nGuardBands / 4; // nGuardBands is the total left+right guard band. The left/right outer part is half of the left/right guard band.
  uint32_t middleSlopeWidth = isGuardBand ? (outerSlopeWidth - (innerSlopeWidth / 2)) : 0;
  if (!isGuardBand)
    {
      //the inner slope normally overflows on the guard bands so it needs to be adjusted if there aren't any
      innerSlopeWidth = allocatedSubBands.front ().first - maskBand.first;
      NS_ASSERT (innerSlopeWidth == maskBand.second - allocatedSubBands.back ().second);
    }
  WifiSpectrumBand outerBandLeft (maskBand.first, //to handle cases where allocated channel is under WifiPhy configured channel width.
                                  isGuardBand ? (maskBand.first + outerSlopeWidth - 1) : maskBand.first);
  WifiSpectrumBand middleBandLeft (isGuardBand ? (outerBandLeft.second + 1) : outerBandLeft.second,
                                   outerBandLeft.second + middleSlopeWidth);
  WifiSpectrumBand innerBandLeft (allocatedSubBands.front ().first - innerSlopeWidth,
                                  allocatedSubBands.front ().first - 1); //better to place slope based on allocated subcarriers
  WifiSpectrumBand flatJunctionLeft (isGuardBand ? (middleBandLeft.second + 1) : middleBandLeft.second,
                                     isGuardBand ? (innerBandLeft.first - 1) : innerBandLeft.first); //in order to handle shift due to guard subcarriers
  WifiSpectrumBand outerBandRight (isGuardBand ? (maskBand.second - outerSlopeWidth + 1) : maskBand.second,
                                   maskBand.second); //start from outer edge to be able to compute flat junction width
  WifiSpectrumBand middleBandRight (outerBandRight.first - middleSlopeWidth,
                                    isGuardBand ? (outerBandRight.first - 1) : outerBandRight.first);
  WifiSpectrumBand innerBandRight (allocatedSubBands.back ().second + 1,
                                   allocatedSubBands.back ().second + innerSlopeWidth);
  WifiSpectrumBand flatJunctionRight (isGuardBand ? (innerBandRight.second + 1) : innerBandRight.second,
                                      isGuardBand ? (middleBandRight.first - 1) : middleBandRight.first);
  NS_LOG_DEBUG ("outerBandLeft=[" << outerBandLeft.first << ";" << outerBandLeft.second << "] " <<
                "middleBandLeft=[" << middleBandLeft.first << ";" << middleBandLeft.second << "] " <<
                "flatJunctionLeft=[" << flatJunctionLeft.first << ";" << flatJunctionLeft.second << "] " <<
                "innerBandLeft=[" << innerBandLeft.first << ";" << innerBandLeft.second << "] " <<
                "subBands=[" << allocatedSubBands.front ().first << ";" << allocatedSubBands.back ().second << "] " <<
                "innerBandRight=[" << innerBandRight.first << ";" << innerBandRight.second << "] " <<
                "flatJunctionRight=[" << flatJunctionRight.first << ";" << flatJunctionRight.second << "] " <<
                "middleBandRight=[" << middleBandRight.first << ";" << middleBandRight.second << "] " <<
                "outerBandRight=[" << outerBandRight.first << ";" << outerBandRight.second << "] ");
  NS_ASSERT (numMaskBands == ((allocatedSubBands.back ().second - allocatedSubBands.front ().first + 1)  //equivalent to allocatedBand (includes notches and DC)
                              + 2 * (innerSlopeWidth + middleSlopeWidth + outerSlopeWidth)
                              + (isGuardBand ? (flatJunctionLeft.second - flatJunctionLeft.first + 1) : 0) //flat junctions
                              + (isGuardBand ? (flatJunctionRight.second - flatJunctionRight.first + 1) : 0)));

  //Different slopes
  double innerSlope = (-1 * minInnerBandDbr) / innerSlopeWidth;
  double middleSlope = (-1 * (minOuterBandDbr - minInnerBandDbr)) / middleSlopeWidth;
  double outerSlope = (txPowerMiddleBandMinDbm - txPowerOuterBandMinDbm) / outerSlopeWidth;

  //Build spectrum mask
  Values::iterator vit = c->ValuesBegin ();
  Bands::const_iterator bit = c->ConstBandsBegin ();
  double txPowerW = 0.0;
  for (size_t i = 0; i < numBands; i++, vit++, bit++)
    {
      if (i < maskBand.first || i > maskBand.second) //outside the spectrum mask
        {
          txPowerW = 0.0;
        }
      else if (isGuardBand && i <= outerBandLeft.second && i >= outerBandLeft.first) //better to put greater first (less computation)
        {
          txPowerW = DbmToW (txPowerOuterBandMinDbm + ((i - outerBandLeft.first) * outerSlope));
        }
      else if (isGuardBand && i <= middleBandLeft.second && i >= middleBandLeft.first)
        {
          txPowerW = DbmToW (txPowerMiddleBandMinDbm + ((i - middleBandLeft.first) * middleSlope));
        }
      else if (isGuardBand && i <= flatJunctionLeft.second && i >= flatJunctionLeft.first)
        {
          txPowerW = DbmToW (txPowerInnerBandMinDbm);
        }
      else if (i <= innerBandLeft.second && i >= innerBandLeft.first)
        {
          txPowerW = DbmToW (txPowerInnerBandMinDbm + ((i - innerBandLeft.first) * innerSlope));
        }
      else if (i <= allocatedSubBands.back ().second && i >= allocatedSubBands.front ().first) //roughly in allocated band
        {
          bool insideSubBand = false;
          for (uint32_t j = 0; !insideSubBand && j < numSubBands; j++) //continue until inside a sub-band
            {
              insideSubBand = (i <= allocatedSubBands[j].second) && (i >= allocatedSubBands[j].first);
            }
          if (insideSubBand)
            {
              txPowerW = txPowerPerBandW;
            }
          else
            {
              txPowerW = DbmToW (txPowerInnerBandMinDbm);
            }
        }
      else if (i <= innerBandRight.second && i >= innerBandRight.first)
        {
          txPowerW = DbmToW (txPowerRefDbm - ((i - innerBandRight.first + 1) * innerSlope)); // +1 so as to be symmetric with left slope
        }
      else if (i <= flatJunctionRight.second && i >= flatJunctionRight.first)
        {
          txPowerW = DbmToW (txPowerInnerBandMinDbm);
        }
      else if (isGuardBand && i <= middleBandRight.second && i >= middleBandRight.first)
        {
          txPowerW = DbmToW (txPowerInnerBandMinDbm - ((i - middleBandRight.first + 1) * middleSlope)); // +1 so as to be symmetric with left slope
        }
      else if (isGuardBand && i <= outerBandRight.second && i >= outerBandRight.first)
        {
          txPowerW = DbmToW (txPowerMiddleBandMinDbm - ((i - outerBandRight.first + 1) * outerSlope)); // +1 so as to be symmetric with left slope
        }
      else
        {
          NS_FATAL_ERROR ("Should have handled all cases");
        }
      double txPowerDbr = 10 * std::log10 (txPowerW / txPowerPerBandW);
      NS_LOG_LOGIC (uint32_t (i) << " -> " << txPowerDbr);
      *vit = txPowerW / (bit->fh - bit->fl);
    }
  NS_LOG_INFO ("Added signal power to subbands " << allocatedSubBands.front ().first << "-" << allocatedSubBands.back ().second);
}

void
WifiSpectrumValueHelper::NormalizeSpectrumMask (Ptr<SpectrumValue> c, double txPowerW)
{
  NS_LOG_FUNCTION (c << txPowerW);
  //Normalize power so that total signal power equals transmit power
  double currentTxPowerW = Integral (*c);
  double normalizationRatio = currentTxPowerW / txPowerW;
  NS_LOG_LOGIC ("Current power: " << currentTxPowerW << "W vs expected power: " << txPowerW << "W" <<
                " -> ratio (C/E) = " << normalizationRatio);
  Values::iterator vit = c->ValuesBegin ();
  for (size_t i = 0; i < c->GetSpectrumModel ()->GetNumBands (); i++, vit++)
    {
      *vit = (*vit) / normalizationRatio;
    }
}

double
WifiSpectrumValueHelper::DbmToW (double dBm)
{
  return std::pow (10.0, 0.1 * (dBm - 30.0));
}

uint32_t
WifiSpectrumValueHelper::GetGranularityForChannelSpacing (uint32_t centerFrequency)
{
  uint32_t granularity = 20; //MHz
  if (centerFrequency <= 2484 //5 MHz spacing for 2.4 GHz channels
      || (centerFrequency >= 5860 && centerFrequency <= 5920)) //802.11p channels are 5 MHz or 10 MHz wide
    {
      granularity = 5;
    }
  return granularity * 1000000; //Hz expected
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateTxPowerSpectralDensityForChannelSpacingGranularity (uint32_t centerFrequency, uint16_t channelWidth, uint32_t granularity,
                                                                                   double txPowerW)
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << granularity << txPowerW);
  NS_ASSERT (channelWidth % 5 == 0);
  Ptr<SpectrumValue> c = Create<SpectrumValue> (GetSpectrumModel (centerFrequency, channelWidth, granularity, false /*no guard bandwidth*/));
  Values::iterator vit = c->ValuesBegin ();
  Bands::const_iterator bit = c->ConstBandsBegin ();
  uint32_t nAllocatedBands = static_cast<uint32_t> (((channelWidth * 1e6) / granularity) + 0.5);
  NS_ASSERT (c->GetSpectrumModel ()->GetNumBands () == nAllocatedBands);
  // Evenly spread power across band
  double txPowerPerBand = txPowerW / nAllocatedBands;
  for (size_t i = 0; i < c->GetSpectrumModel ()->GetNumBands (); i++, vit++, bit++)
    {
      *vit = txPowerPerBand / (bit->fh - bit->fl);
    }
  return c;
}

static Ptr<SpectrumModel> g_WifiSpectrumModel5Mhz; ///< static initializer for the class

WifiSpectrumValueHelper::~WifiSpectrumValueHelper ()
{
}

WifiSpectrumValue5MhzFactory::~WifiSpectrumValue5MhzFactory ()
{
}

/**
 * Static class to initialize the values for the 2.4 GHz Wi-Fi spectrum model
 */
static class WifiSpectrumModel5MhzInitializer
{
public:
  WifiSpectrumModel5MhzInitializer ()
  {
    Bands bands;
    for (int i = -4; i < 13 + 7; i++)
      {
        BandInfo bi;
        bi.fl = 2407.0e6 + i * 5.0e6;
        bi.fh = 2407.0e6 + (i + 1) * 5.0e6;
        bi.fc = (bi.fl +  bi.fh) / 2;
        bands.push_back (bi);
      }
    g_WifiSpectrumModel5Mhz = Create<SpectrumModel> (bands);
  }
} g_WifiSpectrumModel5MhzInitializerInstance; //!< initialization instance for WifiSpectrumModel5Mhz



Ptr<SpectrumValue>
WifiSpectrumValue5MhzFactory::CreateConstant (double v)
{
  Ptr<SpectrumValue> c = Create <SpectrumValue> (g_WifiSpectrumModel5Mhz);
  (*c) = v;
  return c;
}


Ptr<SpectrumValue>
WifiSpectrumValue5MhzFactory::CreateTxPowerSpectralDensity (double txPower, uint8_t channel)
{
  Ptr<SpectrumValue> txPsd = Create <SpectrumValue> (g_WifiSpectrumModel5Mhz);

  // since the spectrum model has a resolution of 5 MHz, we model
  // the transmitted signal with a constant density over a 20MHz
  // bandwidth centered on the center frequency of the channel. The
  // transmission power outside the transmission power density is
  // calculated considering the transmit spectrum mask, see IEEE
  // Std. 802.11-2007, Annex I

  double txPowerDensity = txPower / 20e6;

  NS_ASSERT (channel >= 1);
  NS_ASSERT (channel <= 13);

  (*txPsd)[channel - 1] = txPowerDensity * 1e-4;      // -40dB
  (*txPsd)[channel]     = txPowerDensity * 1e-4;      // -40dB
  (*txPsd)[channel + 1] = txPowerDensity * 0.0015849; // -28dB
  (*txPsd)[channel + 2] = txPowerDensity * 0.0015849; // -28dB
  (*txPsd)[channel + 3] = txPowerDensity;
  (*txPsd)[channel + 4] = txPowerDensity;
  (*txPsd)[channel + 5] = txPowerDensity;
  (*txPsd)[channel + 6] = txPowerDensity;
  (*txPsd)[channel + 7] = txPowerDensity * 0.0015849; // -28dB
  (*txPsd)[channel + 8] = txPowerDensity * 0.0015849; // -28dB
  (*txPsd)[channel + 9] = txPowerDensity * 1e-4;      // -40dB
  (*txPsd)[channel + 10] = txPowerDensity * 1e-4;      // -40dB

  return txPsd;
}

Ptr<SpectrumValue>
WifiSpectrumValue5MhzFactory::CreateRfFilter (uint8_t channel)
{
  Ptr<SpectrumValue> rf = Create <SpectrumValue> (g_WifiSpectrumModel5Mhz);

  NS_ASSERT (channel >= 1);
  NS_ASSERT (channel <= 13);

  (*rf)[channel + 3] = 1;
  (*rf)[channel + 4] = 1;
  (*rf)[channel + 5] = 1;
  (*rf)[channel + 6] = 1;

  return rf;
}

} // namespace ns3
