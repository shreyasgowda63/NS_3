.. include replace.txt
.. _wifi_clipping:

++++++++++++++++++++
YansWifiChannelSpatialIndex
++++++++++++++++++++

Clipping is dependant on :ref:`position-aware<position-aware>`. This module should be installed on a nodes prior to
adding them to the channel.

:ref:`Clipped<clipping>` adds the attributes "ReceiveClipRange" and  "EnableSpatialIndexing" both multi and single model spectrum
channels. To utilize clipping, enable clipping via the EnableSpatialIndexing attribute and set the clip range to be non-zero. 
The interaction between this clip range and the delta-position that is used by position-aware is described in 
the :reg:`clipping documentation<clipping>` 

The helper is utilized to set up clipping.

:code: 
      `
      ...
      YansWifiChannelHelper wifiChannel;
      wifiChannel.EnableClipping ();
      Ptr<YansWifiChannel> channel = wifiChannel.Create (); // if clipping is enabled, returns YansWifiChannelSpatialIndex vs YansWifiChannel
      channel->SetAttribute ("ReceiveClipRange", DoubleValue (clip_range));
      ...
      `