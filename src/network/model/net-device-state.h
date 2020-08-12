/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Ananthakrishnan S
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
 * Author: Ananthakrishnan S <ananthakrishnan190@gmail.com>
 */

#ifndef NET_DEVICE_STATE_H
#define NET_DEVICE_STATE_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"
#include "ns3/ptr.h"

namespace ns3 {

 /**
  * This enum is the implementation of RFC 2863
  * operational states. More details can be found here:
  * https://tools.ietf.org/html/rfc2863
  * https://www.kernel.org/doc/Documentation/networking/operstates.txt
  */
  enum class OperationalState {

  /** 
   * Used for devices where RFC 2863 operational states are not
   * implemented in their device drivers in Linux kernel. In ns-3, devices
   * that does not use RFC 2863 operational states do not aggregate 
   * NetDeviceState object with them. This state is therefore not needed.
   * 
   * IF_OPER_UNKNOWN,
   */

  /** 
   * Can be used to denote removed netdevices. Not used
   * in linux kernel. (Removed devices disappear.)
   * 
   * IF_OPER_NOTPRESENT,
   */
  
  /**
   * Carrier is down on a non-stacked device.
   */
  IF_OPER_DOWN,

  /**
   * Useful only in stacked interfaces. An interface stacked on
   * another interface that is in IF_OPER_DOWN show this state.
   * (eg. VLAN)
   */
	IF_OPER_LOWERLAYERDOWN,

  /** 
   * Unused in Linux kernel. Testing mode; not
   * relevant in ns-3.
   * 
   * IF_OPER_TESTING,
   */

  /**
   * Interface is L1 up, but waiting for an external event, for eg. for a
   * protocol to establish such as 802.1X.
   */
	IF_OPER_DORMANT,

  /**
   * Carrier is detected and the device can be used.
   */
  IF_OPER_UP,
};

/**
 * \ingroup netdevice
 * \brief Administrative and Operational state of NetDevice.
 *
 * This class holds the implementation of administrative state and
 * operational state of a NetDevice. Operational state is  based on
 * the states mentioned in RFC 2863: The Interfaces Group MIB.
 * This class can be subclassed to provide implementations specific to
 * NetDevices. However, anyone wanting to use this architecture should 
 * use public APIs in the base class itself. This implementation is not
 * a necessary part of NetDevice. In other words, this is an optional feature. 
 * 
 * Upper layers such as IP that are interested in keeping track of states
 * of NetDevice can connect to callbacks in this class.
 * 
 */
class NetDeviceState : public Object
{
public:

  friend class NetDevice;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  NetDeviceState ();

  /**
   * \brief Register for receiving notifications on changes
   *  in adminstrative state of a NetDevice.
   * 
   *  This callback registration requires no context.
   * \param callback the callback to be added
   */
  void RegisterAdministrativeStateNotifierWithoutContext (Callback<void, bool> callback);

  /**
   * \brief Unregister from receiving notifictions on changes
   *  in adminstrative state of a NetDevice. This function
   *  removes the callback that was added without providing
   *  context.
   * 
   * \param callback the callback to be removed
   */
  void UnRegisterAdministrativeStateNotifierWithoutContext (Callback<void, bool> callback);

  /**
   * \brief Register for notifications on changes
   *  in adminstrative state of a NetDevice. This
   *  function appends callback to the chain with
   *  a context.
   * 
   * \param callback the callback to be added
   * \param contextPath Context path to provide when invoking the Callback.
   */
  void RegisterAdministrativeStateNotifierWithContext (Callback<void, bool> callback,
                                                  std::string contextPath);

  /**
   * \brief UnRegister from receiving notifications
   *  on changes in adminstrative state of a NetDevice.
   *  This function removes the given callback from the
   *  chain which was connected with a context.
   * 
   * \param callback the callback to be added
   * \param contextPath Context path which was used to connect the Callback.
   */
  void UnRegisterAdministrativeStateNotifierWithContext (Callback<void, bool> callback, 
                                                    std::string contextPath);

  /**
   * \brief Register for receiving notifications on changes
   *  in RFC 2863 operational state of a NetDevice.
   * 
   *  This callback registration requires no context.
   * \param callback the callback to be added
   */
  void RegisterOperationalStateNotifierWithoutContext (Callback<void, OperationalState> callback);

  /**
   * \brief Unregister from receiving notifictions on changes
   *  in RFC 2863 operational state of a NetDevice. This function
   *  removes the callback that was added without providing
   *  context.
   * 
   * \param callback the callback to be removed
   */
  void UnRegisterOperationalStateNotifierWithoutContext (Callback<void, OperationalState> callback);

  /**
   * \brief Register for notifications on changes
   *  in adminstrative state of a NetDevice. This
   *  function appends callback to the chain with
   *  a context.
   * 
   * \param callback the callback to be added
   * \param contextPath Context path to provide when invoking the Callback.
   */
  void RegisterOperationalStateNotifierWithContext (Callback<void, OperationalState> callback,
                                                  std::string contextPath);

   /**
   * \brief UnRegister from receiving notifications
   *  on changes in RFC 2863 operational state
   *  of a NetDevice. This function removes the given
   *  callback from the chain which was connected with
   *  a context.
   * 
   * \param callback the callback to be added
   * \param contextPath Context path which was used to connect the Callback.
   */
  void UnRegisterOperationalStateNotifierWithContext (Callback<void, OperationalState> callback, 
                                                    std::string contextPath); 

  /**
   * \return RFC 2863 operational state of 
   * the NetDevice.
   */
  OperationalState GetOperationalState (void) const;

  /**
   * \return the administrative state of the NetDevice.
   */
  bool IsUp () const;

  /**
   * Bring up a NetDevice; Device is now
   * administratively up. 
   */
  void SetUp (void);

  /**
   * Bring down a NetDevice. Device is now 
   * administratively down. 
   */
  void SetDown (void);

private:

  /**
   * \brief This method is used to take care of device specific
   *  actions needed to bring up a NetDevice similar to calling
   *  ndo_open () from dev_open () in Linux.
   */
  virtual void DoSetUp (void) = 0;

  /**
   * \brief This method is used to take care of device specific
   *  actions needed to bring down a NetDevice similar to calling
   *  ndo_stop () from inside dev_close () in Linux.
   */
  virtual void DoSetDown (void) = 0;

protected:

  /**
   * \brief Set RFC 2863 operational state of the device. 
   *   
   * /param opState the state to be set.
   */
  void SetOperationalState (OperationalState opState);

  /**
   * Represents IFF_UP in net_device_flags enum
   * in Linux. Used to store the administrative
   * state of the NetDevice.
   */
  bool m_administrativeState;

  /**
   * RFC 2863 operational state of the device.
   */
  OperationalState m_operationalState;

  /**
   * List of callbacks to fire when adminstrative state
   * of a NetDevice changes.
   */
  TracedCallback<bool> m_administrativeStateChangeCallback;

  /**
   * List of callbacks to fire when operational state
   * of a NetDevice changes.
   */
  TracedCallback<OperationalState> m_operationalStateChangeCallback;

  /**
   * Traced callback for tracing device states.
   */
  TracedCallback< bool, OperationalState> m_stateChangeTrace;

};

} // namespace ns3
#endif /* NET_DEVICE_STATE_H */
