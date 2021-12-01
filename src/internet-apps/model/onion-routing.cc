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




#include "onion-routing.h"

namespace ns3 {

/* ... */


NS_OBJECT_ENSURE_REGISTERED (OnionRouting);

NS_LOG_COMPONENT_DEFINE ("onionrouting");


TypeId
OnionRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OnionRouting")
    .SetParent<Object> ()
    .SetGroupName ("OnionRouting");
  return tid;

}



OnionRouting::OnionRouting (uint16_t sealPadding, const uint16_t protocolNumber)
{ 
  m_errno = ERROR_NOTERROR;
  //set seal bytes
  m_sealPadding = sealPadding;
  //set address bytes of the used IP protocol
  if (protocolNumber == Ipv4L3Protocol::PROT_NUMBER)
    {
      m_addressSize = 4;
    }
  else if (protocolNumber == Ipv6L3Protocol::PROT_NUMBER)
    {
      m_addressSize = 16;
    }
  else
    {
      NS_LOG_WARN ("The given (IP) protocol number is not valid.");
      m_errno = ERROR_PROT_NUMBER;
    }
}





void
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys,uint16_t routeLen)
{
  m_errno = ERROR_NOTERROR;

  if (routeLen < 4)
    {

      NS_LOG_LOGIC ("Route is too short, need at least 3 intermediate hops.");
      m_errno = ERROR_ROUTE_TO_SHORT;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      CreateOnion (cipher,route,keys, routeLen, routeLen, nullptr, 0, nullptr, 0);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");
    }
}



void 
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t routeLen, uint8_t * endContent, uint16_t endContentLen)
{
  m_errno = ERROR_NOTERROR;

  if (routeLen < 4)
    {

      NS_LOG_LOGIC ("Route is too short, need at least 3 intermediate hops.");
      m_errno = ERROR_ROUTE_TO_SHORT;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      CreateOnion (cipher,route,keys, routeLen, routeLen, nullptr, 0, endContent, endContentLen);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");
    }
}



void
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint8_t ** layerContent, uint16_t layerContentLen, uint16_t routeLen)
{
  m_errno = ERROR_NOTERROR;

  if (routeLen < 4)
    {

      NS_LOG_LOGIC ("Route is too short, need at least 3 intermediate hops.");
      m_errno = ERROR_ROUTE_TO_SHORT;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      CreateOnion (cipher,route,keys, routeLen, routeLen, layerContent, layerContentLen, nullptr, 0);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");
    }
}


void
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint8_t ** layerContent, uint16_t layerContentLen, uint16_t routeLen, uint8_t * endContent, uint16_t endContentLen)
{
  m_errno = ERROR_NOTERROR;

  if (routeLen < 4)
    {

      NS_LOG_LOGIC ("Route is too short, need at least 3 intermediate hops.");
      m_errno = ERROR_ROUTE_TO_SHORT;
    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      CreateOnion (cipher,route,keys, routeLen, routeLen, layerContent, layerContentLen, endContent, endContentLen);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");
    }
}

void
OnionRouting::CreateOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t index, uint16_t routeLen, uint8_t ** layerContent, uint16_t layerContentLen, uint8_t * endContent, uint16_t endContentLen)
{
  //number of bytes to be encrypted in this layer
  int plainLayerLen = m_addressSize + layerContentLen + OnionLength (index - 1,layerContentLen,endContentLen);

  m_onionStream << "(";       //fancy output
 
  if (index <= 2 && (endContentLen != 0 || layerContentLen != 0))       //stop recursion

    {
      for (int i = 0; i < m_addressSize; ++i) //Insert the zero address -- 0.0.0.0 (ipv4)
        {
          cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding + i] = 0;
        }

      if (endContentLen != 0)
        {
          //If end content is set include it & encrypt
          memcpy (&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding + m_addressSize], endContent, endContentLen);
          EncryptLayer (&cipher[m_addressSize + layerContentLen + m_sealPadding],&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding],m_addressSize + endContentLen,keys[routeLen - index + 1]);
        }
      else
        {
          //Include layer content & encrypt
          memcpy (&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding + m_addressSize], layerContent[routeLen - index + 1], layerContentLen);
          EncryptLayer (&cipher[m_addressSize + layerContentLen + m_sealPadding],&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding],m_addressSize + layerContentLen,keys[routeLen - index + 1]);
        }

    }
  else if (index > 2)        //recursion

    {
      CreateOnion (&cipher[ m_sealPadding + m_addressSize + layerContentLen],route, keys, index - 1,routeLen,layerContent, layerContentLen, endContent,endContentLen);
    }


  //Insert next hop address
  memcpy (&cipher[m_sealPadding], route[routeLen - index + 1], m_addressSize);
  AddressToStream (route[routeLen - index + 1]);      //fancy output

  if (layerContentLen != 0)
    {
      //Include layer content in the current encryption layer
      memcpy (&cipher[m_sealPadding + m_addressSize], layerContent[routeLen - index], layerContentLen);
    }

  EncryptLayer (cipher,&cipher[m_sealPadding],plainLayerLen,keys[routeLen - index]);       //encrypt

  m_onionStream << ") ";       //fancy output

} //create the onion





orLayer * OnionRouting::PeelOnion (uint8_t * onion, uint16_t onionLen, uint8_t * publicKey, uint8_t * secretKey)
{
  uint8_t * innerLayer = new uint8_t[onionLen - (m_sealPadding)];

  DecryptLayer (innerLayer,onion,onionLen,publicKey,secretKey);

  orLayer * layer = new orLayer;
  layer->nextHopIP = innerLayer;
  layer->innerLayer = &innerLayer[m_addressSize];
  layer->innerLayerLen = onionLen - m_sealPadding - m_addressSize;

  return layer;
}





uint16_t OnionRouting::OnionLength (uint16_t routeLen,uint16_t layerContentLen,uint16_t endContentLen)
{
  routeLen = routeLen - 1;
  if (endContentLen == 0 && layerContentLen == 0)
    {
      return routeLen * (m_sealPadding + m_addressSize);
    }
  else if (endContentLen == 0)
    {
      return routeLen * (m_sealPadding + m_addressSize + layerContentLen) + m_sealPadding + m_addressSize + layerContentLen;
    }
  else
    {
      return routeLen * (m_sealPadding + m_addressSize + layerContentLen) + m_sealPadding + m_addressSize + endContentLen;
    }
}




void OnionRouting::AddressToStream (uint8_t* ip)
{
  m_onionStream << (int) ip[0];
  for (int i = 1; i < m_addressSize; ++i)
    {
      m_onionStream << "."  << (int) ip[i];
    }
}




enum OnionRouting::OnionErrno
OnionRouting::GetErrno (void)
{
  NS_LOG_FUNCTION (this);
  return m_errno;
}



//OnionRoutingDummyEncryption -- Implemented class onion routing with dummy encryption


OnionRoutingDummyEncryption::OnionRoutingDummyEncryption (uint16_t sealPadding, const uint16_t protocolNumber) : OnionRouting (sealPadding,protocolNumber)
{
  if (sealPadding < 4)
    {
      NS_FATAL_ERROR ("Seal padding must be at least 4-Bytes");
    }
}


TypeId
OnionRoutingDummyEncryption::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OnionRoutingDummyEncryption")
    .SetParent<OnionRouting> ()
    .SetGroupName ("OnionRouting");
  return tid;

}


void OnionRoutingDummyEncryption::GenerateNewKey (void)
{
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  uint32_t key = x->GetInteger (0,UINT32_MAX);
  memcpy (m_encryptionkey, &key, 4);
}


uint8_t * OnionRoutingDummyEncryption::GetEncryptionKey (void)
{
  return m_encryptionkey;
}



void OnionRoutingDummyEncryption::EncryptLayer (uint8_t * ciphertext, uint8_t* message, int len, uint8_t * key) const
{
  memcpy (ciphertext, &key[0], 4); //include the key
  for (int i = 0; i < m_sealPadding - 4; ++i) //insert seal padding, all zeros
    {
      ciphertext[4 + i] = 0;
    }
}



void OnionRoutingDummyEncryption::DecryptLayer (uint8_t * innerLayer, uint8_t* onion, uint16_t onionLen, uint8_t * pk, uint8_t * sk) const
{
  for (int i = 0; i < 4; ++i)
    {
      if (onion[i] != pk[i])
        {
          NS_LOG_INFO ("Messge corrupted or not for this node");
          m_errno = ERROR_DECRYPTION;
          return;
        }
    }
  memcpy (innerLayer,&onion[m_sealPadding],onionLen - m_sealPadding);
}







}

