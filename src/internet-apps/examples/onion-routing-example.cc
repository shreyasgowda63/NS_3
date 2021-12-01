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
 *   Network topology
 *
 * 
 *
 *
 *                        n2-------------n3
 *                       / \             /
 *                      /   \(1Mbps,3ms)/          
 *                     /     \         /
 *         (5Mbps,2ms)/       \       /
 *                   /         \     /
 *                  /           \   /
 *                 /             \ /
 *   n0-----------n1              n4----------n5
 *     (5Mbps,2ms)                 (5Mbps,2ms)
 *    
 *
 * - all links are point-to-point links with indicated delay
 * - onion messagess are sent using the UDP protocol
 * 
 *  Instructions:
 *        1) Download & setup libsodium library
 *        2) run in terminal: ./waf --run "src/internet-apps/examples/onion-routing-example.cc --onionMode=1"
 *             --note~ the argument onionMode defines the mode of operation of the example -- see the upper list
 *
 * 
 */

//! @cond Doxygen_Suppress


#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/onion-routing.h"
#include "ns3/applications-module.h"
 

#include <sodium.h>

#define ONION_NO_CONTENT 0
#define ONION_ENDCONTENT 1
#define ONION_LAYERCONTENT 2
#define ONION_LAYERCONTENT_ENDCONTENT 3



using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("OnionRoutingExample");


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




//Class that implements the Onion Routing class using the libsodium lybrary 
class OnionManager : public OnionRouting 
{
public:
  //get the typeid
  static TypeId GetTypeId (void);
  //constructor, need to setup encryption params
  OnionManager ();
  //dummy destructor
  ~OnionManager ();

  //Generate new key pair
  void GenerateNewKeyPair ();

  //return pk
  uint8_t * GetPublicKey ();
  //return sk
  uint8_t * GetSecretKey ();

  //implement encryption
  virtual void EncryptLayer (uint8_t * ciphertext, uint8_t* message, int len, uint8_t * key) const;
  //implement decryption
  virtual void DecryptLayer (uint8_t * innerLayer, uint8_t* onion, uint16_t onionLen, uint8_t * pk, uint8_t * sk) const;

  //the publickey
  uint8_t m_publickey[crypto_box_PUBLICKEYBYTES];
  //the secretkey
  uint8_t m_secretkey[crypto_box_SECRETKEYBYTES];
};


TypeId
OnionManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OnionManager")
    .SetParent<OnionRouting> ()
    .SetGroupName ("OnionRouting");
  return tid;

}




OnionManager::OnionManager ()
  : OnionRouting (crypto_box_SEALBYTES,Ipv4L3Protocol::PROT_NUMBER)
{}



OnionManager::~OnionManager ()
{}


void OnionManager::GenerateNewKeyPair ()
{
  crypto_box_keypair (m_publickey, m_secretkey);
}

uint8_t * OnionManager::GetPublicKey ()
{
  return m_publickey;
}

uint8_t * OnionManager::GetSecretKey ()
{
  return m_secretkey;
}


void OnionManager::EncryptLayer (uint8_t * ciphertext, uint8_t* message, int len, uint8_t * key) const
{
  if (crypto_box_seal (ciphertext, message, len, key) != 0)
    {
      NS_LOG_WARN ("Error during encryption");
      m_errno = ERROR_ENCRYPTION;
    }
}



void OnionManager::DecryptLayer (uint8_t * innerLayer, uint8_t* onion, uint16_t onionLen, uint8_t * pk, uint8_t * sk) const
{
  if (crypto_box_seal_open (innerLayer, onion, onionLen, pk, sk) != 0)
    {
      NS_LOG_WARN ("Messge corrupted or not for this node");
      m_errno = ERROR_DECRYPTION;
    }
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
  uint8_t * GetPublicKey ();
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

  Ptr<Socket>       m_socket;
  Address           m_peer;
  uint16_t          m_port;
  Ipv4Address       m_address;
  OnionManager      m_onionManager;
  uint8_t           m_onionMode;
  uint16_t          m_routeLen;
  uint8_t **        m_ipRoute;
  uint8_t **        m_keys;
  uint8_t **        m_layerContent;
  uint16_t          m_layerContentLen;

};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_port (4242),
    m_routeLen (0)
{}


//setup onion mode and length of data to be encrypted in layers
MyApp::MyApp (uint8_t onionMode,uint16_t layerContentLen)
  : m_socket (0),
    m_peer (),
    m_port (4242),
    m_onionMode (onionMode),
    m_routeLen (0),
    m_layerContentLen (layerContentLen)
{}


MyApp::~MyApp ()
{
  m_socket = 0;
}




uint8_t * MyApp::GetPublicKey ()
{
  return m_onionManager.GetPublicKey ();
}

Ipv4Address MyApp::GetAddress ()
{
  return m_address;
}



void MyApp::Setup ()
{

  //setup encryption
  m_onionManager.GenerateNewKeyPair ();


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
    .SetGroupName ("ORexample")
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
  else // the case (m_onionMode == ONION_LAYERCONTENT_ENDCONTENT)
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
  orLayer * onionLayer = m_onionManager.PeelOnion (cipher,cipherLen,m_onionManager.GetPublicKey (),m_onionManager.GetSecretKey ());


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
  uint8_t   onionMode = ONION_ENDCONTENT;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("onionMode", "Select the mode of operation", onionMode);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("OnionRoutingExample", LOG_LEVEL_INFO);
      LogComponentEnable ("onionrouting", LOG_LEVEL_INFO);

    }

  if ( onionMode > 3)
    {
      NS_FATAL_ERROR ("Wrong mode of operation selected, select one in range 0 to 3");
    }

  /* ... */


  //create the topology of six nodes
  NodeContainer nc;
  nc.Create (6);
  NodeContainer n0n1 = NodeContainer(nc.Get(0),nc.Get(1));
  NodeContainer n1n2 = NodeContainer(nc.Get(1),nc.Get(2));
  NodeContainer n2n3 = NodeContainer(nc.Get(2),nc.Get(3));
  NodeContainer n2n4 = NodeContainer(nc.Get(2),nc.Get(4));
  NodeContainer n3n4 = NodeContainer(nc.Get(3),nc.Get(4));
  NodeContainer n4n5 = NodeContainer(nc.Get(4),nc.Get(5)); 


  //Install internet stack
  InternetStackHelper stack;
  stack.Install (nc);

  //Create Point-to-Point Channels
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate",StringValue ("5Mbps"));
  p2p.SetChannelAttribute("Delay",StringValue("2ms"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);

  NetDeviceContainer d1d2 = p2p.Install (n1n2);

  NetDeviceContainer d4d5 = p2p.Install (n4n5);

  p2p.SetDeviceAttribute ("DataRate",StringValue ("1Mbps"));
  p2p.SetChannelAttribute("Delay",StringValue("3ms"));
  NetDeviceContainer d2d3 = p2p.Install (n2n3);
  NetDeviceContainer d3d4 = p2p.Install (n3n4);
  NetDeviceContainer d2d4 = p2p.Install (n2n4);



  //setup ip addressses
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = address.Assign (d0d1);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = address.Assign (d1d2);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i3 = address.Assign (d2d3);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = address.Assign (d3d4);

  address.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i4 = address.Assign (d2d4);

  address.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i5 = address.Assign (d4d5);

  //set routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  //define route of the onion
  uint16_t routeLen = 5;
  uint8_t * ipRoute[routeLen];
  uint8_t * keys[routeLen];
  uint16_t layerContentLen = 27; //hardcoded
  uint8_t * layerContent[routeLen];


  //Install apps on nodes
  ApplicationContainer applications (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));
  applications.Add (CreateObject<MyApp> (onionMode,layerContentLen));

  nc.Get (0)->AddApplication (applications.Get (0));
  nc.Get (1)->AddApplication (applications.Get (1));
  nc.Get (2)->AddApplication (applications.Get (2));
  nc.Get (3)->AddApplication (applications.Get (3));
  nc.Get (4)->AddApplication (applications.Get (4));
  nc.Get (5)->AddApplication (applications.Get (5));


  //setup encryption & address
  for (uint32_t i = 0; i < applications.GetN (); ++i)
    {
      /* code */
      applications.Get (i)->GetObject<MyApp> ()->Setup ();
    }


  //ip addresses of the route
  ipRoute[0] = IpToBuff (applications.Get (2)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[1] = IpToBuff (applications.Get (3)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[2] = IpToBuff (applications.Get (1)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[3] = IpToBuff (applications.Get (4)->GetObject<MyApp> ()->GetAddress ());
  ipRoute[4] = IpToBuff (applications.Get (5)->GetObject<MyApp> ()->GetAddress ());

  //encryption keys of nodes in the route
  keys[0] = applications.Get (2)->GetObject<MyApp> ()->GetPublicKey ();
  keys[1] = applications.Get (3)->GetObject<MyApp> ()->GetPublicKey ();
  keys[2] = applications.Get (1)->GetObject<MyApp> ()->GetPublicKey ();
  keys[3] = applications.Get (4)->GetObject<MyApp> ()->GetPublicKey ();
  keys[4] = applications.Get (5)->GetObject<MyApp> ()->GetPublicKey ();

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