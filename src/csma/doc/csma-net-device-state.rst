CsmaNetDeviceState
------------------

This documentation desribes some notable behavior of CsmaNetDevice When
the device changes operational states.

OperationalState of CsmaNetDevice is set to IF_OPER_DOWN in the beginning
when CsmaNetDeviceState is aggregated to CsmaNetDevice. Usually,
it is the CsmaHelper class that creates a CsmaNetDevice and attaches it to
a CsmaChannel. So in scenarios where CsmaHelper is used, OperationalState 
of CsmaNetDevice is set to IF_OPER_UP without the involvement of user. This
happens before start of the simulation. During the simulation, user can change
the administrative state of a CsmaNetDevice as well as Attach, Detach, Reattach
and Remove a CsmaNetDevice from a CsmaChannel. These operations will change the
OperationalState of CsmaNetDevice as follows:

* When CsmaNetDevice is administratively brought DOWN using 
  :cpp:class:'CsmaHelper::SetDeviceDown () function, the
  OperationalState changes to IF_OPER_DOWN.

* When CsmaNetDevice is administratively brought UP using
  :cpp:class:'CsmaHelper::SetDeviceUp () function, the
  OperationalState changes to IF_OPER_UP if
  CsmaNetDevice is connected to a CsmaChannel.

* When the CsmaNetDevice is attached to a CsmaChannel using
  :cpp:class:'CsmaChannel::Attach () function, the
  OperationalState changes to IF_OPER_UP.

* When the CsmaNetDevice is reattached to the CsmaChannel
  it was previously attached to by using
  :cpp:class:'CsmaHelper::ReattachChannel () function, the
  OperationalState changes to IF_OPER_UP.

* When the CsmaNetDevice is detached from a CsmaChannel using
  :cpp:class:'CsmaHelper::ReattachChannel () function, the
  OperationalState changes to IF_OPER_DOWN.

* When the CsmaNetDevice is removed from a CsmaChannel completely
  using :cpp:class:'CsmaChannel::Remove () function, the
  OperationalState changes to IF_OPER_DOWN.

Note: When using any of these functions, it should be noted that
if the device is currently transmitting, the packet completes
transmission (the packet transmitting behavior is not altered).
Currently a packet transmission is not aborted mid-way.
