/*
 * Copyright (c) 2020 University of Washington
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
 * Authors: Rohan Patidar <rpatidar@uw.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Sian Jin <sianjin@uw.edu>
 */

// This file contains table data for the TableBasedErrorRateModel.  For more
// information on the source of this data, see wifi module documentation.

#ifndef ERROR_RATE_TABLES_H
#define ERROR_RATE_TABLES_H

namespace ns3
{
namespace wifi
{

const uint16_t ERROR_TABLE_BCC_SMALL_FRAME_SIZE =
    32; //!< reference size (bytes) of small frames for BCC
const uint16_t ERROR_TABLE_BCC_LARGE_FRAME_SIZE =
    1458; //!< reference size (bytes) of large frames for BCC
const uint16_t ERROR_TABLE_LDPC_FRAME_SIZE = 1458; //!< reference size (bytes) for LDPC
const uint8_t ERROR_TABLE_BCC_MAX_NUM_MCS = 10;    //!< maximum number of MCSs for BCC
const uint8_t ERROR_TABLE_LDPC_MAX_NUM_MCS = 12;   //!< maximum number of MCSs for LDPC

/// Table of SNR (dB) and PER pairs
typedef std::vector<std::pair<double /* SNR (dB) */, double /* PER */>> SnrPerTable;

/// AWGN error table for BCC with reference size of 32 bytes
static const SnrPerTable AwgnErrorTableBcc32[ERROR_TABLE_BCC_MAX_NUM_MCS] = {
    // MCS-0
    {
        {-3.50000, 1.00000},
        {-3.00000, 0.99500},
        {-2.50000, 0.94080},
        {-2.00000, 0.82590},
        {-1.50000, 0.58950},
        {-1.00000, 0.30830},
        {-0.50000, 0.12540},
        {0.00000, 0.03440},
        {0.50000, 0.00850},
        {1.00000, 0.00150},
        {1.50000, 0.00024},
        {2.00000, 0.00009},
        {2.50000, 0.00000},
    },
    // MCS-1
    {
        {-0.50000, 1.00000},
        {0.00000, 0.99210},
        {0.50000, 0.96710},
        {1.00000, 0.83490},
        {1.50000, 0.58740},
        {2.00000, 0.31690},
        {2.50000, 0.11820},
        {3.00000, 0.03640},
        {3.50000, 0.00850},
        {4.00000, 0.00160},
        {4.50000, 0.00026},
        {5.00000, 0.00003},
        {5.50000, 0.00000},
    },
    // MCS-2
    {
        {2.00000, 1.00000},
        {2.50000, 0.99400},
        {3.00000, 0.95880},
        {3.50000, 0.85120},
        {4.00000, 0.59900},
        {4.50000, 0.31250},
        {5.00000, 0.11710},
        {5.50000, 0.03390},
        {6.00000, 0.00780},
        {6.50000, 0.00160},
        {7.00000, 0.00028},
        {7.50000, 0.00008},
        {8.00000, 0.00000},
    },
    // MCS-3
    {
        {4.00000, 1.00000},
        {4.50000, 0.99900},
        {5.00000, 0.99800},
        {5.50000, 0.96900},
        {6.00000, 0.88430},
        {6.50000, 0.74090},
        {7.00000, 0.46510},
        {7.50000, 0.25710},
        {8.00000, 0.10800},
        {8.50000, 0.03810},
        {9.00000, 0.01190},
        {9.50000, 0.00320},
        {10.00000, 0.00076},
        {10.50000, 0.00017},
        {11.00000, 0.00003},
        {11.50000, 0.00000},
    },
    // MCS-4
    {
        {8.00000, 1.00000},
        {8.50000, 0.99900},
        {9.00000, 0.94790},
        {9.50000, 0.79890},
        {10.00000, 0.59230},
        {10.50000, 0.33700},
        {11.00000, 0.15720},
        {11.50000, 0.05420},
        {12.00000, 0.01580},
        {12.50000, 0.00400},
        {13.00000, 0.00110},
        {13.50000, 0.00027},
        {14.00000, 0.00009},
        {14.50000, 0.00000},
    },
    // MCS-5
    {
        {11.50000, 1.00000},
        {12.00000, 0.99800},
        {12.50000, 0.96530},
        {13.00000, 0.89700},
        {13.50000, 0.73010},
        {14.00000, 0.52570},
        {14.50000, 0.30580},
        {15.00000, 0.15750},
        {15.50000, 0.06460},
        {16.00000, 0.02410},
        {16.50000, 0.00790},
        {17.00000, 0.00230},
        {17.50000, 0.00069},
        {18.00000, 0.00018},
        {18.50000, 0.00004},
        {19.00000, 0.00002},
        {19.50000, 0.00000},
    },
    // MCS-6
    {
        {13.00000, 1.00000},
        {13.50000, 0.99010},
        {14.00000, 0.96250},
        {14.50000, 0.83980},
        {15.00000, 0.68660},
        {15.50000, 0.46340},
        {16.00000, 0.25200},
        {16.50000, 0.11450},
        {17.00000, 0.04610},
        {17.50000, 0.01580},
        {18.00000, 0.00490},
        {18.50000, 0.00160},
        {19.00000, 0.00039},
        {19.50000, 0.00011},
        {20.00000, 0.00002},
        {20.50000, 0.00000},
    },
    // MCS-7
    {
        {14.00000, 1.00000},
        {14.50000, 0.99700},
        {15.00000, 0.98330},
        {15.50000, 0.94260},
        {16.00000, 0.84830},
        {16.50000, 0.63470},
        {17.00000, 0.43770},
        {17.50000, 0.22260},
        {18.00000, 0.11110},
        {18.50000, 0.04400},
        {19.00000, 0.01730},
        {19.50000, 0.00530},
        {20.00000, 0.00190},
        {20.50000, 0.00052},
        {21.00000, 0.00016},
        {21.50000, 0.00004},
        {22.00000, 0.00000},
    },
    // MCS-8
    {
        {17.50000, 1.00000},
        {18.00000, 0.99600},
        {18.50000, 0.98040},
        {19.00000, 0.92860},
        {19.50000, 0.82250},
        {20.00000, 0.67500},
        {20.50000, 0.44140},
        {21.00000, 0.26660},
        {21.50000, 0.13380},
        {22.00000, 0.05950},
        {22.50000, 0.02450},
        {23.00000, 0.00910},
        {23.50000, 0.00320},
        {24.00000, 0.00098},
        {24.50000, 0.00033},
        {25.00000, 0.00014},
        {25.50000, 0.00004},
        {26.00000, 0.00000},
    },
    // MCS-9
    {
        {19.00000, 1.00000},
        {19.50000, 0.99800},
        {20.00000, 0.98910},
        {20.50000, 0.95970},
        {21.00000, 0.87500},
        {21.50000, 0.73170},
        {22.00000, 0.51540},
        {22.50000, 0.30380},
        {23.00000, 0.15240},
        {23.50000, 0.06340},
        {24.00000, 0.02470},
        {24.50000, 0.00850},
        {25.00000, 0.00290},
        {25.50000, 0.00100},
        {26.00000, 0.00017},
        {26.50000, 0.00014},
        {27.00000, 0.00002},
        {27.50000, 0.00000},
    },
};

/// AWGN error table for BCC with reference size of 1458 bytes
static const SnrPerTable AwgnErrorTableBcc1458[ERROR_TABLE_BCC_MAX_NUM_MCS] = {
    // MCS-0
    {
        {-1.00000, 1.00000},
        {-0.50000, 0.99400},
        {0.00000, 0.81850},
        {0.50000, 0.29080},
        {1.00000, 0.06630},
        {1.50000, 0.01120},
        {2.00000, 0.00150},
        {2.50000, 0.00015},
        {3.00000, 0.00001},
        {3.50000, 0.00000},
    },
    // MCS-1
    {
        {2.00000, 1.00000},
        {2.50000, 0.99700},
        {3.00000, 0.79440},
        {3.50000, 0.30080},
        {4.00000, 0.07280},
        {4.50000, 0.01200},
        {5.00000, 0.00150},
        {5.50000, 0.00023},
        {6.00000, 0.00000},
    },
    // MCS-2
    {
        {4.50000, 1.00000},
        {5.00000, 0.99800},
        {5.50000, 0.75780},
        {6.00000, 0.30100},
        {6.50000, 0.06760},
        {7.00000, 0.01220},
        {7.50000, 0.00230},
        {8.00000, 0.00035},
        {8.50000, 0.00004},
        {9.00000, 0.00000},
    },
    // MCS-3
    {
        {7.50000, 1.00000},
        {8.00000, 0.99400},
        {8.50000, 0.84050},
        {9.00000, 0.43410},
        {9.50000, 0.14190},
        {10.00000, 0.03740},
        {10.50000, 0.00860},
        {11.00000, 0.00190},
        {11.50000, 0.00036},
        {12.00000, 0.00005},
        {12.50000, 0.00000},
    },
    // MCS-4
    {
        {11.00000, 1.00000},
        {11.50000, 0.92690},
        {12.00000, 0.51390},
        {12.50000, 0.18260},
        {13.00000, 0.04650},
        {13.50000, 0.01100},
        {14.00000, 0.00260},
        {14.50000, 0.00041},
        {15.00000, 0.00010},
        {15.50000, 0.00000},
    },
    // MCS-5
    {
        {14.50000, 1.00000},
        {15.00000, 0.99900},
        {15.50000, 0.94790},
        {16.00000, 0.66250},
        {16.50000, 0.29780},
        {17.00000, 0.10580},
        {17.50000, 0.03340},
        {18.00000, 0.00910},
        {18.50000, 0.00230},
        {19.00000, 0.00064},
        {19.50000, 0.00017},
        {20.00000, 0.00002},
        {20.50000, 0.00000},
    },
    // MCS-6
    {
        {16.00000, 1.00000},
        {16.50000, 0.99500},
        {17.00000, 0.87960},
        {17.50000, 0.51390},
        {18.00000, 0.20910},
        {18.50000, 0.06390},
        {19.00000, 0.01860},
        {19.50000, 0.00460},
        {20.00000, 0.00130},
        {20.50000, 0.00023},
        {21.00000, 0.00002},
        {21.50000, 0.00000},
    },
    // MCS-7
    {
        {17.50000, 1.00000},
        {18.00000, 0.97850},
        {18.50000, 0.73930},
        {19.00000, 0.33750},
        {19.50000, 0.12340},
        {20.00000, 0.03550},
        {20.50000, 0.01000},
        {21.00000, 0.00270},
        {21.50000, 0.00050},
        {22.00000, 0.00009},
        {22.50000, 0.00001},
        {23.00000, 0.00000},
    },
    // MCS-8
    {
        {21.00000, 1.00000},
        {21.50000, 0.99800},
        {22.00000, 0.93990},
        {22.50000, 0.67090},
        {23.00000, 0.35250},
        {23.50000, 0.13760},
        {24.00000, 0.04750},
        {24.50000, 0.01540},
        {25.00000, 0.00520},
        {25.50000, 0.00150},
        {26.00000, 0.00036},
        {26.50000, 0.00007},
        {27.00000, 0.00000},
    },
    // MCS-9
    {
        {22.50000, 1.00000},
        {23.00000, 0.99900},
        {23.50000, 0.95060},
        {24.00000, 0.68470},
        {24.50000, 0.32610},
        {25.00000, 0.12480},
        {25.50000, 0.04090},
        {26.00000, 0.01300},
        {26.50000, 0.00360},
        {27.00000, 0.00082},
        {27.50000, 0.00010},
        {28.00000, 0.00002},
        {28.50000, 0.00001},
        {29.00000, 0.00000},
    },
};

/// AWGN error table for LDPC with reference size of 1458 bytes
static const SnrPerTable AwgnErrorTableLdpc1458[ERROR_TABLE_LDPC_MAX_NUM_MCS] = {
    // MCS-0
    {
        {-1.50000, 1.00000},
        {-1.25000, 0.97950},
        {-1.00000, 0.60480},
        {-0.75000, 0.17050},
        {-0.50000, 0.03320},
        {-0.25000, 0.00530},
        {0.00000, 0.00085},
        {0.25000, 0.00022},
        {0.50000, 0.00004},
        {0.75000, 0.00000},
    },
    // MCS-1
    {
        {1.50000, 1.00000},
        {1.75000, 0.97470},
        {2.00000, 0.62330},
        {2.25000, 0.18590},
        {2.50000, 0.03400},
        {2.75000, 0.00550},
        {3.00000, 0.00083},
        {3.25000, 0.00015},
        {3.50000, 0.00003},
        {3.75000, 0.00000},
    },
    // MCS-2
    {
        {4.00000, 1.00000},
        {4.25000, 0.98720},
        {4.50000, 0.62560},
        {4.75000, 0.15800},
        {5.00000, 0.02090},
        {5.25000, 0.00250},
        {5.50000, 0.00034},
        {5.75000, 0.00003},
        {6.00000, 0.00000},
    },
    // MCS-3
    {
        {6.75000, 1.00000},
        {7.00000, 0.99800},
        {7.25000, 0.94340},
        {7.50000, 0.57890},
        {7.75000, 0.20640},
        {8.00000, 0.04840},
        {8.25000, 0.00930},
        {8.50000, 0.00180},
        {8.75000, 0.00040},
        {9.00000, 0.00011},
        {9.25000, 0.00002},
        {9.50000, 0.00000},
    },
    // MCS-4
    {{10.00000, 1.00000},
     {10.25000, 0.99310},
     {10.50000, 0.70890},
     {10.75000, 0.24720},
     {11.00000, 0.04700},
     {11.25000, 0.00590},
     {11.50000, 0.00091},
     {11.75000, 0.00016},
     {12.00000, 0.00003},
     {12.25000, 0.00000}},
    // MCS-5
    {
        {14.00000, 1.00000},
        {14.25000, 0.99700},
        {14.50000, 0.91830},
        {14.75000, 0.53790},
        {15.00000, 0.16610},
        {15.25000, 0.03690},
        {15.50000, 0.00650},
        {15.75000, 0.00100},
        {16.00000, 0.00031},
        {16.25000, 0.00005},
        {16.50000, 0.00000},
    },
    // MCS-6
    {
        {15.50000, 1.00000},
        {15.75000, 0.98140},
        {16.00000, 0.73930},
        {16.25000, 0.33110},
        {16.50000, 0.08150},
        {16.75000, 0.01620},
        {17.00000, 0.00270},
        {17.25000, 0.00052},
        {17.50000, 0.00005},
        {17.75000, 0.00003},
        {18.00000, 0.00000},
    },
    // MCS-7
    {
        {17.00000, 1.00000},
        {17.25000, 0.97750},
        {17.50000, 0.73980},
        {17.75000, 0.33190},
        {18.00000, 0.09640},
        {18.25000, 0.02180},
        {18.50000, 0.00470},
        {18.75000, 0.00087},
        {19.00000, 0.00018},
        {19.25000, 0.00003},
        {19.50000, 0.00000},
    },
    // MCS-8
    {
        {20.50000, 1.00000},
        {20.75000, 0.99500},
        {21.00000, 0.89700},
        {21.25000, 0.56270},
        {21.50000, 0.20920},
        {21.75000, 0.05600},
        {22.00000, 0.01170},
        {22.25000, 0.00250},
        {22.50000, 0.00038},
        {22.75000, 0.00013},
        {23.00000, 0.00004},
        {23.25000, 0.00001},
        {23.50000, 0.00000},
    },
    // MCS-9
    {
        {22.25000, 1.00000},
        {22.50000, 0.99900},
        {22.75000, 0.94080},
        {23.00000, 0.63600},
        {23.25000, 0.27190},
        {23.50000, 0.08700},
        {23.75000, 0.02210},
        {24.00000, 0.00500},
        {24.25000, 0.00110},
        {24.50000, 0.00032},
        {24.75000, 0.00004},
        {25.00000, 0.00000},
    },
    // MCS-10
    {
        {25.75000, 1.00000},
        {26.00000, 0.94970},
        {26.25000, 0.68660},
        {26.50000, 0.32940},
        {26.75000, 0.11620},
        {27.00000, 0.03440},
        {27.25000, 0.00880},
        {27.50000, 0.00210},
        {27.75000, 0.00054},
        {28.00000, 0.00009},
        {28.25000, 0.00002},
        {28.50000, 0.00000},
    },
    // MCS-11
    {
        {27.75000, 1.00000},
        {28.00000, 0.94880},
        {28.25000, 0.75260},
        {28.50000, 0.40230},
        {28.75000, 0.16210},
        {29.00000, 0.05150},
        {29.25000, 0.01310},
        {29.50000, 0.00360},
        {29.75000, 0.00100},
        {30.00000, 0.00022},
        {30.25000, 0.00006},
        {30.50000, 0.00000},
    },
};

} // namespace wifi
} // namespace ns3

#endif /* ERROR_RATE_TABLES_H */
