/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 INRIA, 2012 University of Washington
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
 */

#ifndef L3_EMU_FD_NET_DEVICE_HELPER_H
#define L3_EMU_FD_NET_DEVICE_HELPER_H

#include "ns3/fd-net-device.h"
#include "ns3/emu-fd-net-device-helper.h"

namespace ns3 {

/**
 * \ingroup fd-net-device
 * \brief build a set of L3EmuFdNetDevice objects attached to a physical
 * network interface
 */
class L3EmuFdNetDeviceHelper : public EmuFdNetDeviceHelper
{
public:
  /**
   * Construct a L3EmuFdNetDeviceHelper.
   */
  L3EmuFdNetDeviceHelper ();
  virtual ~L3EmuFdNetDeviceHelper ()
  {}

protected:

  /**
   * Sets a file descriptor on the FileDescriptorNetDevice.
   */
  virtual void SetFileDescriptor (Ptr<FdNetDevice> device) const;

  /**
   * Call out to a separate process running as suid root in order to get a raw
   * socket.  We do this to avoid having the entire simulation running as root.
   * \return the rawSocket number
   */
  virtual int CreateFileDescriptor (void) const;
};

} // namespace ns3

#endif /* L3_EMU_FD_NET_DEVICE_HELPER_H */
