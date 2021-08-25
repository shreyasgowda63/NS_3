/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


/*
* MIT License
*
* Copyright (c) 2020 DLTLT
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*
* Author: Niki Hrovatin <niki.hrovatin@famnit.upr.si>
*
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



OnionRouting::OnionRouting (uint16_t keySize,uint16_t sealPadding, uint16_t addressSize)
{
  this->m_keySize = keySize;
  this->m_sealPadding = sealPadding;
  this->m_addressSize = addressSize;       //4B for IPv4
}





int 
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys,uint16_t routeLen)
{
  if (routeLen < 4)
    {

      NS_LOG_ERROR ("Route is to short, need at least 3 intermediate hops.");
      return 1;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      int status = CreateOnion (cipher,route,keys, routeLen, routeLen, nullptr, 0, nullptr, 0);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");

      return status;
    }
}



int 
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t routeLen, uint8_t * endContent, uint16_t endContentLen)
{
  if (routeLen < 4)
    {

      NS_LOG_ERROR ("Route is to short, need at least 3 intermediate hops.");
      return 1;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      int status = CreateOnion (cipher,route,keys, routeLen, routeLen, nullptr, 0, endContent, endContentLen);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");

      return status;
    }
}



int 
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint8_t ** layerContent, uint16_t layerContentLen, uint16_t routeLen)
{

  if (routeLen < 4)
    {

      NS_LOG_ERROR ("Route is to short, need at least 3 intermediate hops.");
      return 1;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      int status = CreateOnion (cipher,route,keys, routeLen, routeLen, layerContent, layerContentLen, nullptr, 0);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");

      return status;
    }
}



int 
OnionRouting::BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint8_t ** layerContent, uint16_t layerContentLen, uint16_t routeLen, uint8_t * endContent, uint16_t endContentLen)
{
  if (routeLen < 4)
    {

      NS_LOG_ERROR ("Route is to short, need at least 3 intermediate hops.");
      return 1;

    }
  else
    {
      NS_LOG_INFO ("Start creation of the onion");

      m_onionStream.str ("");

      int status = CreateOnion (cipher,route,keys, routeLen, routeLen, layerContent, layerContentLen, endContent, endContentLen);

      AddressToStream (route[0]);

      NS_LOG_INFO (m_onionStream.str () << "\nOnion ready");

      return status;
    }
}


int 
OnionRouting::CreateOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t index, uint16_t routeLen, uint8_t ** layerContent, uint16_t layerContentLen, uint8_t * endContent, uint16_t endContentLen)
{
  //number of bytes to be encrypted in this layer
  int plainLayerLen = m_addressSize + layerContentLen + OnionLength (index - 1,layerContentLen,endContentLen);

  m_onionStream << "(";       //fancy output

  int errors = 0;
 
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
          errors += EncryptLayer (&cipher[m_addressSize + layerContentLen + m_sealPadding],&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding],m_addressSize + endContentLen,keys[routeLen - index + 1]);
        }
      else
        {
          //Include layer content & encrypt
          memcpy (&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding + m_addressSize], layerContent[routeLen - index + 1], layerContentLen);
          errors += EncryptLayer (&cipher[m_addressSize + layerContentLen + m_sealPadding],&cipher[m_addressSize + layerContentLen + m_sealPadding + m_sealPadding],m_addressSize + layerContentLen,keys[routeLen - index + 1]);
        }

    }
  else if (index > 2)        //recursion

    {
      errors += CreateOnion (&cipher[ m_sealPadding + m_addressSize + layerContentLen],route, keys, index - 1,routeLen,layerContent, layerContentLen, endContent,endContentLen);
    }


  //Insert next hop address
  memcpy (&cipher[m_sealPadding], route[routeLen - index + 1], m_addressSize);
  AddressToStream (route[routeLen - index + 1]);      //fancy output

  if (layerContentLen != 0)
    {
      //Include layer content in the current encryption layer
      memcpy (&cipher[m_sealPadding + m_addressSize], layerContent[routeLen - index], layerContentLen);
    }

  errors += EncryptLayer (cipher,&cipher[m_sealPadding],plainLayerLen,keys[routeLen - index]);       //encrypt


  m_onionStream << ") ";       //fancy output
  return errors;

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



}

