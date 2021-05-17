.. include replace.txt
.. _wifi_clipping:

++++++++++++++++++++
YansWifiChannelSpatialIndex
++++++++++++++++++++

The helper is utilized to return the appropriate class if clipping is enabled.

:code: 
      `
      ...
      NodeContainer nodes;
      nodes.Create(NODECOUNT);
      // mobility required for position aware
      MobilityHelper mobility;
      mobility.install(nodes);
      // position aware required for spatial indexing
      PositionAwareHelper pos_aware;
      pos_aware.Install(nodes);
      YansWifiChannelHelper wifiChannel;
      if (clipping_enabled) // assumes clipping_enabled is determined before hand, perhaps by command line
      {
        wifiChannel.EnableClipping ();
      }
      Ptr<YansWifiChannel> channel = wifiChannel.Create (); // if clipping is enabled, returns YansWifiChannelSpatialIndex vs YansWifiChannel
      if (clipping_enabled)
      {
        channel->SetAttribute ("ReceiveClipRange", DoubleValue (clip_range));
      }
      ...
      `