/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


/*
* Copyright (c) 2020 DLTLT 
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
* Author: Niki Hrovatin <niki.hrovatin@famnit.upr.si>
*/



/*
 * Example of implementation and use of the OnionRouting class 
 *
 * 
 * The given example can be used to construct onion messages of the following features:
 *   0- ONION_NO_CONTENT - onion message including only routing information
 *   1- ONION_ENDCONTENT - onion message including content to be delivered to the last node in the path
 *   2- ONION_LAYERCONTENT - onion message including a content of fixed length (in bytes) in each layer
 *   3- ONION_LAYERCONTENT_ENDCONTENT - onion message including a content of fixed length in each layer and 
 *                                      content of arbitrary length to be delivered to the last node in the path
 *
 *  The listed onion messagess are selected through the cmd argument onionMode. This argument defines the mode of operation of the example code.
 *    (The value preceeding the name should be given to the cmd argument)
 *
 *  !!NOTE!! the given example uses the external library <a href="https://libsodium.gitbook.io/doc/">libsodium</a> for encryption and decryption.
 *
 *
 *   The Network Topology
 *
 *
 *   n0   n1   n2   n3   n4
 *   |    |    |    |    |
 *   =====================
 *        LAN 10.1.1.0
 *
 *
 *  Instructions:
 *        1) Download & setup libsodium library
 *        2) run in terminal: ./waf --run "src/internet-apps/examples/onion-routing-example.cc --onionMode=1"
 *             --note~ the argument onionMode defines the mode of operation of the example -- see the upper list
 *
 * 
 */

//! @cond Doxygen_Suppress


//#include "ns3/core-module.h"
#include "ns3/csma-module.h"
//#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/onion-routing.h"
#include "ns3/applications-module.h"
 

#define ONION_NO_CONTENT 0
#define ONION_ENDCONTENT 1
#define ONION_LAYERCONTENT 2
#define ONION_LAYERCONTENT_ENDCONTENT 3



using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("OnionRoutingDummyEncryptionExample");


//Serialize an Ipv4Address
uint8_t * IpToBuff (Ipv4Address in)
{
  uint8_t * out = new uint8_t[4];
  in.Serialize (&out[0]);
  return out;
}

//Construct Ipv4 address from the serialized form 
Ipv4Address ConstructIpv4 (uint8_t * buf)
{
  uint32_t ip = 0;

  ip += buf[0];
  ip = ip << 8;
  ip += buf[1];
  ip = ip << 8;
  ip += buf[2];
  ip = ip << 8;
  ip += buf[3];

  return Ipv4Address (ip);
}



//Serialize a string
uint8_t * StringToUchar (std::string in)
{
  uint8_t * out = new uint8_t[in.length ()];
  memcpy (&out[0], &in[0], in.length ());
  return out;
}

//Deserialize a string
std::string UcharToString (uint8_t* seq, int len)
{
  std::string strForm (&seq[0], &seq[0] + len);
  return strForm;
}





//Application to be installed on nodes
class MyApp : public Application
{
public:
  MyApp (); //constructor
  MyApp (uint8_t onionMode,uint16_t layerContentLen); //constructor to setup onionMode and layerContent
  virtual ~MyApp (); //destructor

  static TypeId GetTypeId (void);//return the typeid

  //return the pk
  uint8_t * GetEncryptionKey ();
  //return ip address of the node
  Ipv4Address GetAddress ();
  //Setting up encryption & address
  void Setup ();
  void SetRoute (uint16_t routeLen, uint8_t ** ipRoute, uint8_t ** keys, uint8_t ** layerContent, uint8_t layerContentLen);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendOnion ();
  void RecvOnion (Ptr<Socket> socket);

  Ptr<Socket>                     m_socket;
  Address                         m_peer;
  uint16_t                        m_port;
  Ipv4Address                     m_address;
  OnionRoutingDummyEncryption     m_onionManager;
  uint8_t                         m_onionMode;
  uint16_t                        m_routeLen;
  uint8_t **                      m_ipRoute;
  uint8_t **                      m_keys;
  uint8_t **                      m_layerContent;
  uint16_t                        m_layerContentLen;

};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_port (4242),
    m_onionManager (32,Ipv4L3Protocol::PROT_NUMBER),
    m_routeLen (0)
{}


//setup onion mode and length of data to be encrypted in layers
MyApp::MyApp (uint8_t onionMode,uint16_t layerContentLen)
  : m_socket (0),
    m_peer (),
    m_port (4242),
    m_onionManager (32,Ipv4L3Protocol::PROT_NUMBER),
    m_onionMode (onionMode),
    m_routeLen (0),
    m_layerContentLen (layerContentLen)
{}


MyApp::~MyApp ()
{
  m_socket = 0;
}




uint8_t * MyApp::GetEncryptionKey ()
{
  return m_onionManager.GetEncryptionKey ();
}

Ipv4Address MyApp::GetAddress ()
{
  return m_address;
}



void MyApp::Setup ()
{

  //setup encryption
  m_onionManager.GenerateNewKey ();


  //Get node details
  Ptr<Node> PtrNode = this->GetNode ();
  Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0);
  m_address = iaddr.GetLocal ();

}

//called only on the node who will send the onion
//used to set-up the route and content of the onion message
void MyApp::SetRoute (uint16_t routeLen, uint8_t ** ipRoute, uint8_t ** keys, uint8_t ** layerContent, uint8_t layerContentLen)
{
  m_routeLen = routeLen;
  m_ipRoute = ipRoute;
  m_keys = keys;
  m_layerContent = layerContent;
  m_layerContentLen = layerContentLen;
}



/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("OR-dummy-example")
    .AddConstructor<MyApp> ()
  ;
  return tid;
}


//Construct and send the onion made using the class Onion Routing
void
MyApp::SendOnion ()
{

  //set the content of the onion message
  std::string s ("Some content to send anonymously.");
  uint8_t * content = StringToUchar (s);
  uint16_t contentLen = s.length ();

  int cipherLen = 0;
  uint8_t * cipher;

  //Construct the onion based on the selected mode
  if (m_onionMode == ONION_NO_CONTENT)
    {
      cipherLen = m_onionManager.OnionLength (m_routeLen,0,0);
      cipher = new uint8_t[cipherLen];

      m_onionManager.BuildOnion (cipher, m_ipRoute, m_keys, m_routeLen);
    }
  else if (m_onionMode == ONION_ENDCONTENT)
    {
      cipherLen = m_onionManager.OnionLength (m_routeLen,0,contentLen);
      cipher = new uint8_t[cipherLen];

      m_onionManager.BuildOnion (cipher, m_ipRoute, m_keys, m_routeLen,content,contentLen);
    }
  else if (m_onionMode == ONION_LAYERCONTENT)
    {
      cipherLen = m_onionManager.OnionLength (m_routeLen,m_layerContentLen,0);
      cipher = new uint8_t[cipherLen];

      m_onionManager.BuildOnion (cipher, m_ipRoute, m_keys, m_layerContent, m_layerContentLen, m_routeLen);
    }
  else if (m_onionMode == ONION_LAYERCONTENT_ENDCONTENT)
    {
      cipherLen = m_onionManager.OnionLength (m_routeLen,m_layerContentLen,contentLen);
      cipher = new uint8_t[cipherLen];

      m_onionManager.BuildOnion (cipher, m_ipRoute, m_keys, m_layerContent, m_layerContentLen, m_routeLen,content,contentLen);
    }


  //Insert the onion in a packet & send to the first node in the route
  uint8_t const * buff = cipher;
  Ptr<Packet> p = Create<Packet> (buff,cipherLen);
  m_socket->SendTo (p, 0, InetSocketAddress (ConstructIpv4 (m_ipRoute[0]),m_port));
  NS_LOG_INFO ("Onion construction--Onion sent to: " << ConstructIpv4 (m_ipRoute[0]) << " of size: " << p->GetSize () << " bytes" );

}

//Performed when the node receives an onion
void
MyApp::RecvOnion (Ptr<Socket> socket)
{
  Address from;
  Ptr<Packet> p = socket->RecvFrom (from);

  //extract onion from the packet
  uint32_t cipherLen = p->GetSize ();

  if (cipherLen == 0 )//execute if onion mode -- ONION_NO_CONTENT -- was selected
    {
      NS_LOG_INFO ("Onion reveal--Empty onion sent from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " received at: " << m_address );
      return;
    }

  uint8_t cipher[cipherLen];
  p->CopyData (cipher, cipherLen);

  //decrypt onion layer
  orLayer * onionLayer = m_onionManager.PeelOnion (cipher,cipherLen,m_onionManager.GetEncryptionKey (),m_onionManager.GetEncryptionKey ());


  if (ConstructIpv4 (onionLayer->nextHopIP).Get () == 0) //execute if onion mode -- -- was selected
    {//Onion totally decrypted
      NS_LOG_INFO ("Onion reveal--Onion sent from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " received at: " << m_address <<  " of size: " << p->GetSize () << " bytes, containing the end content:" << UcharToString (onionLayer->innerLayer,onionLayer->innerLayerLen));
    }
  else
    {//Onion routing step
      if (m_onionMode == ONION_LAYERCONTENT || m_onionMode == ONION_LAYERCONTENT_ENDCONTENT) //execute if onion mode -- ONION_LAYERCONTENT,ONION_LAYERCONTENT_ENDCONTENT -- was selected
        {
          uint8_t const * buff = &onionLayer->innerLayer[m_layerContentLen];
          Ptr<Packet> np = Create<Packet> (buff,onionLayer->innerLayerLen - m_layerContentLen);
          m_socket->SendTo (np, 0, InetSocketAddress (ConstructIpv4 (onionLayer->nextHopIP),m_port));
          NS_LOG_INFO ("Onion routing--Onion sent from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " received at: " << m_address <<  " of size: " << p->GetSize () << " bytes, containing the layer content: " << UcharToString (onionLayer->innerLayer,m_layerContentLen) << ", sent to: " << ConstructIpv4 (onionLayer->nextHopIP));

        }
      else //execute if onion mode -- ONION_NO_CONTENT, ONION_ENDCONTENT, -- was selected
        {
          uint8_t const * buff = onionLayer->innerLayer;
          Ptr<Packet> np = Create<Packet> (buff,onionLayer->innerLayerLen);
          m_socket->SendTo (np, 0, InetSocketAddress (ConstructIpv4 (onionLayer->nextHopIP),m_port));
          NS_LOG_INFO ("Onion routing--Onion sent from: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " received at: " << m_address <<  " of size: " << p->GetSize () << " bytes, sent to: " << ConstructIpv4 (onionLayer->nextHopIP));
        }
    }


}


void
MyApp::StartApplication (void)
{

  //Create socket
  m_socket = Socket::CreateSocket (this->GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_port));
  m_socket->SetRecvCallback (MakeCallback (&MyApp::RecvOnion,this));

  //Check if the node has a route for the onion
  if (m_routeLen != 0)
    {
      //Schedule an onion routing
      Simulator::Schedule (Seconds (2), &MyApp::SendOnion, this);
    }

}

void
MyApp::StopApplication (void)
{

  m_socket->Close ();

}








int
main (int argc, char *argv[])
{
  bool  verbose = true;
  uint32_t  nCsma = 5;
  uint8_t   onionMode = ONION_ENDCONTENT;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("onionMode", "Select the mode of operation", onionMode);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("OnionRoutingDummyEncryptionExample", LOG_LEVEL_INFO);
      LogComponentEnable ("onionrouting", LOG_LEVEL_INFO);

    }

  if ( onionMode > 3)
    {
      NS_FATAL_ERROR ("Wrong mode of operation selected, select one in range 0 to 3");
    }

  /* ... */


  //create nodes
  NodeContainer csmaNodes;
  csmaNodes.Create (nCsma);

  //create channel
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("10Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  //create devices
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  //Install internet stack
  InternetStackHelper stack;
  stack.Install (csmaNodes);

  //setup ip addressses
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  //set routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  //define route of the onion
  uint16_t routeLen = 5;
  uint8_t * ipRoute[routeLen];
  uint8_t * keys[routeLen];
  uint16_t layerContentLen = 27;
  uint8_t * layerContent[routeLen];


  //Install apps on nodes
  ApplicationContainer applications (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));

  csmaNodes.Get (0)->AddApplication (applications.Get (0));
  csmaNodes.Get (1)->AddApplication (applications.Get (1));
  csmaNodes.Get (2)->AddApplication (applications.Get (2));
  csmaNodes.Get (3)->AddApplication (applications.Get (3));
  csmaNodes.Get (4)->AddApplication (applications.Get (4));

  //setup encryption & address
  for (uint32_t i = 0; i < applications.GetN (); ++i)
    {
      /* code */
      applications.Get (i)->GetObject<MyApp> ()->Setup ();
    }


  //ip addresses of the route
  ipRoute[0] = IpToBuff (applications.Get (2)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[1] = IpToBuff (applications.Get (3)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[2] = IpToBuff (applications.Get (4)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[3] = IpToBuff (applications.Get (0)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[4] = IpToBuff (applications.Get (1)->GetObject<MyApp> ()->GetAddress ());

  //encryption keys of nodes in the route
  keys[0] = applications.Get (2)->GetObject<MyApp> ()->GetEncryptionKey ();
  keys[1] = applications.Get (3)->GetObject<MyApp> ()->GetEncryptionKey ();
  keys[2] = applications.Get (4)->GetObject<MyApp> ()->GetEncryptionKey ();
  keys[3] = applications.Get (0)->GetObject<MyApp> ()->GetEncryptionKey ();
  keys[4] = applications.Get (1)->GetObject<MyApp> ()->GetEncryptionKey ();

  //set content of each layer
  layerContent[0] = StringToUchar ("OnionLayer 4 secret content");
  layerContent[1] = StringToUchar ("OnionLayer 3 secret content");
  layerContent[2] = StringToUchar ("OnionLayer 2 secret content");
  layerContent[3] = StringToUchar ("OnionLayer 1 secret content");
  layerContent[4] = StringToUchar ("OnionLayer 0 secret content");


  //setup the route at node 0, the node 0 will send the onion
  applications.Get (0)->GetObject<MyApp> ()->SetRoute (routeLen,ipRoute,keys,layerContent,layerContentLen);


  applications.Start (Seconds (1));
  applications.Stop (Seconds (20));


  Simulator::Stop (Seconds (20));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}



//! @endcond