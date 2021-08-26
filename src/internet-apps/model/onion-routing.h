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



#ifndef ONION_ROUTING_H
#define ONION_ROUTING_H


#include "ns3/core-module.h"


namespace ns3 {


/**
 *  \defgroup onion-routing
 * 
 *  This section documents the API of the ns-3 OnionRouting module.
 *  For a functional description, please refer to the ns-3 manual here:
 *  http://www.nsnam.org/docs/models/html/onion-routing.html
 *
 *  Be sure to read the manual BEFORE going down to the API.
 */



/**
 * \ingroup onion-routing
 * \struct orLayer
 * \brief structure holding details resulting from layer decryption of an onion message
 *
 * 
 */


struct orLayer
{
  uint8_t *   nextHopIP;  //!< ip address given in the serialized form
  uint8_t *   innerLayer;  //!< inner content of the onion message without the next hop address
  uint16_t          innerLayerLen;   //!< length of the inner content of the onion message
};




/**
 * \ingroup onion-routing
 * \enum AddressType
 * 
 * \brief enum defined for the selection of the address type -- values are in bytes (4-Ipv4, 16-Ipv6)
 * 
 */

enum class AddressType
{
  IPV4 = 4, IPV6 = 16
};


/**
 * \ingroup onion-routing
 * \class OnionRouting
 * \brief Abstract class for creation and decryption of Onion messages.
 *
 * The OnionRouting abstract class include useful methods for the creation
 * and redirection of <a href="https://en.wikipedia.org/wiki/Onion_routing">Onion Messages</a>. 
 * The given class can be used to construct onion messages of the following features:<br>
 *   ONION_NO_CONTENT - onion message including only routing information<br>
 *      example: (((10.1.1.2) 10.1.1.1) 10.1.1.5)10.1.1.3<br><br>
 * 
 *   ONION_ENDCONTENT - onion message including content to be delivered to the last node in the path<br>
 *      example: ((((end_content,0.0.0.0) 10.1.1.2) 10.1.1.1) 10.1.1.5) 10.1.1.3<br><br>
 * 
 *   ONION_LAYERCONTENT - onion message including a content of fixed length (in bytes) in each layer<br>
 *      example: ((((layer_content,0.0.0.0) layer_content,10.1.1.2) layer_content,10.1.1.1) layer_content,10.1.1.5)10.1.1.3<br><br>
 * 
 *   ONION_LAYERCONTENT_ENDCONTENT - onion message including a content of fixed length in each layer and 
 *                                   content of arbitrary length to be delivered to the last node in the path<br>
 *      example: ((((end_content,0.0.0.0) layer_content,10.1.1.2) layer_content,10.1.1.1) layer_content,10.1.1.5)10.1.1.3<br><br>
 * 
 * 
 * 
 * 
 * 
 * <br>The given class can be used to construct onion circuits as described in 
 * \e Hiding \e Routing \e Information by david M. Goldschlag, Micheal G. Reed, and Paul F. Syverson, May 1996<br>
 * 
 *
 * We designed an abstract class to allow the use of an arbitrary encryption suite, 
 * by implementing methods EncryptLayer & DecryptLayer
 * 
 */



class OnionRouting : public Object
{
public:

/**
 *  Register this type.
 *  \return The object TypeId.
 */

  static TypeId GetTypeId (void);

/**
*
* \brief Constructor -- Setup parameters for the creation of onions
* 
*   \param [in] sealPadding size increase of the ciphertext in bytes, intorduced by the encryption method 
*   \param [in] addressType enum of type \p AddressType defining the address type used. 
*
*/

  OnionRouting (uint16_t sealPadding, AddressType addressType);

/**
*
* \brief Manage construction of the onion ONION_NO_CONTENT
* 
* The resulting onion message include only routing information and the last hop in the onion path will not recieve content
*   example: (((10.1.1.2) 10.1.1.1) 10.1.1.5)10.1.1.3<br><br>
* 
* Allow the construction of onions of route length > 4, and manage the LOG output 
* 
* 
*   \param [in,out] cipher memory locations on which the onion message will be stored
*   \param [in] route array of ip addresses defining the route of the onion message, ip addresses are stored in the serialized form
*   \param [in] keys array of encryption keys, keys are stored in the serialized form 
*   \param [in] routeLen the length of the route that the onion message will travel (equal to the number of ip addresses stored in the route parameter)
* 
*
*   \return an integer != 0 if errors occurred 
* 
*
*/

  int BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t routeLen); 
  /**
*
* \brief Manage construction of the onion ONION_ENDCONTENT
* 
* The resulting onion message include routing information and the last hop in the onion path recieve the given content<br>
* The zero address -- 0.0.0.0 (ipv4) identifies the last hop in the path of the onion message<br>
*   example: ((((end_content,0.0.0.0) 10.1.1.2) 10.1.1.1) 10.1.1.5) 10.1.1.3<br><br>
* 
* Allow the construction of onions of route length > 4, and manage the LOG output 
* 
* 
*   \param [in,out] cipher memory locations on which the onion message will be stored
*   \param [in] route array of ip addresses defining the route of the onion message, ip addresses are stored in the serialized form
*   \param [in] keys array of encryption keys, keys are stored in the serialized form 
*   \param [in] routeLen the length of the route that the onion message will travel (equal to the number of ip addresses stored in the \p route)
*   \param [in] endContent location of the content to forward to the last node in the onion message path
*   \param [in] endContentLen length in bytes of the data stored at \p endContent
* 
*
*   \return an integer != 0 if errors occurred
* 
* 
*/
  int BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t routeLen, uint8_t * endContent, uint16_t endContentLen); 
  /**
*
* \brief Manage construction of the onion ONION_LAYERCONTENT
* 
* The resulting onion message include only routing information, and each hop in the route will receive data stored in \p layerContent<br>
* The zero address -- 0.0.0.0 (ipv4) identifies the last hop in the path of the onion message<br>
*   example: ((((layer_content,0.0.0.0) layer_content,10.1.1.2) layer_content,10.1.1.1) layer_content,10.1.1.5)10.1.1.3<br><br>
* 
*
* Allow the construction of onions of route length > 4, and manage the LOG output 
* 
* 
*   \param [in,out] cipher memory locations on which the onion message will be stored
*   \param [in] route array of ip addresses defining the route of the onion message, ip addresses are stored in the serialized form
*   \param [in] keys array of encryption keys, keys are stored in the serialized form 
*   \param [in] layerContent array of of pointers, pointing to data to be stored in a layer of the onion message the data is of fixed length in bytes
*   \param [in] layerContentLen length in bytes of the data to be stored in each layer of the onion message
*   \param [in] routeLen the length of the route that the onion message will travel (equal to the number of ip addresses stored in the \p route)
*
*   \return an integer != 0 if errors occurred
* 
*/
  int BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint8_t ** layerContent, uint16_t layerContentLen, uint16_t routeLen); 
  /**
*
* \brief Manage construction of the onion ONION_LAYERCONTENT_ENDCONTENT
* 
* The resulting onion message include only routing information, each hop in the route will receive data stored in \p layerContent, the last hop in the onion path recieve content only data stored in \p endContent<br>
* The zero address -- 0.0.0.0 (ipv4) identifies the last hop in the path of the onion message<br>
*      example: ((((end_content,0.0.0.0) layer_content,10.1.1.2) layer_content,10.1.1.1) layer_content,10.1.1.5)10.1.1.3<br><br>
* 
*
* Allow the construction of onions of route length > 4, and manage the LOG output 
* 
* 
*   \param [in,out] cipher memory locations on which the onion message will be stored
*   \param [in] route array of ip addresses defining the route of the onion message, ip addresses are stored in the serialized form
*   \param [in] keys array of encryption keys, keys are stored in the serialized form 
*   \param [in] layerContent array of of pointers, pointing to data to be stored in a layer of the onion message the data is of fixed length in bytes
*   \param [in] layerContentLen length in bytes of the data to be stored in each layer of the onion message 
*   \param [in] routeLen the length of the route that the onion message will travel (equal to the number of ip addresses stored in the \p route)
*   \param [in] endContent location of the content to forward to the last node in the onion message path
*   \param [in] endContentLen length in bytes of the data stored at \p endContent
*
*   \return an integer != 0 if errors occurred
*/
  int BuildOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint8_t ** layerContent, uint16_t layerContentLen, uint16_t routeLen, uint8_t * endContent, uint16_t endContentLen);     //setup creation of the onion

  /**
*
* \brief Constructs the onion message * 
* 
*   \param [in,out] cipher memory on which the onion message will be stored
*   \param [in] route array of ip addresses defining the route of the onion message, ip addresses are stored in the serialized form
*   \param [in] keys array of encryption keys, keys are stored in the serialized form 
*   \param [in] layerContent array of of pointers, pointing to data to be stored in a layer of the onion message the data is of fixed length in bytes
*   \param [in] layerContentLen length in bytes of the data to be stored in each layer of the onion message 
*   \param [in] index additional parameter used for the construction of the onion message
*   \param [in] routeLen the length of the route that the onion message will travel (equal to the number of ip addresses stored in the \p route)
*   \param [in] endContent location of the content to forward to the last node in the onion message path
*   \param [in] endContentLen length in bytes of the data stored at \p endContent
* 
*   \return an integer != 0 if errors occurred
*
*/

  int CreateOnion (uint8_t * cipher, uint8_t ** route, uint8_t ** keys, uint16_t index, uint16_t routeLen, uint8_t ** layerContent, uint16_t layerContentLen, uint8_t * endContent, uint16_t endContentLen); //create the onion

  /**
*
* \brief Decipher the outer layer of the onion and return details 
* 
*   \param [in] onion the onion message
*   \param [in] onionLen the length in bytes of the onion message
*   \param [in] publicKey encryption key 
*   \param [in] secretKey encryption key 
* <br>
*   \return orLayer * struct holding onion layer details
*
*/

  orLayer * PeelOnion (uint8_t * onion, uint16_t onionLen, uint8_t * publicKey, uint8_t * secretKey);      //peel one layer of the onion

  /**
*
* \brief virtual method, implement encryption
* 
*   \param [in,out] ciphertext memory on which the ciphertext will be stored
*   \param [in] plaintext memory locations containing the data to be encrypted
*   \param [in] len length in bytes of the \p plaintext 
*   \param [in] key encryption key 
* <br>
*   \return an integer != 0 if errors occurred
*
*/
  virtual int EncryptLayer (uint8_t * ciphertext, uint8_t* plaintext, int len, uint8_t * key) const = 0;

  /**
*
* \brief virtual method, implement decryption
* 
*   \param [in,out] plaintext memory locations containing the decrypted data
*   \param [in] ciphertext memory locations containing the encrypted data
*   \param [in] len length in bytes of the \p ciphertext
*   \param [in] publicKey encryption key 
*   \param [in] secretKey encryption key 
* <br>
*   \return an integer != 0 if errors occurred
*
*/
  virtual int DecryptLayer (uint8_t * plaintext, uint8_t* ciphertext, uint16_t len, uint8_t * publicKey, uint8_t * secretKey) const = 0;

  /**
*
* \brief Compute the length in bytes of the onion message at given parameters
* 
*   \param [in] routeLen the length of the route that the onion message will travel (equal to the number of ip addresses stored in the \p route)
*   \param [in] layerContentLen length in bytes of the data to be stored in layers of the onion message 
*   \param [in] endContentLen length in bytes of the data stored in the last hop's layer of the onion message
* 
* <br>
*   \return an integer detailing the length in bytes of the onion message at given parameters
*
*/
  uint16_t OnionLength (uint16_t routeLen,uint16_t layerContentLen,uint16_t endContentLen);

  /**
*
* \brief Output an ip address to a stream variable, used to LOG the onion message
* 
*   \param [in] ip serialized ip address
* 
*
*/

  void AddressToStream (uint8_t* ip);


  uint16_t                           m_sealPadding;       //!< size increase of the ciphertext in bytes, intorduced by the encryption method
  uint16_t                           m_addressSize;   //!< size in bytes of the used address type (4-Ipv4, 16-Ipv6)
  std::stringstream                  m_onionStream;  //!< stringstream used to LOG onion construction
};


}

#endif /* ONION_ROUTING_H */

