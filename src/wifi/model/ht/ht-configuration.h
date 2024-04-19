/*
 * Copyright (c) 2018  Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef HT_CONFIGURATION_H
#define HT_CONFIGURATION_H

#include "ns3/object.h"

namespace ns3
{
namespace wifi
{

/**
 * \brief HT configuration
 * \ingroup wifi
 *
 * This object stores HT configuration information, for use in modifying
 * AP or STA behavior and for constructing HT-related information elements.
 *
 */
class HtConfiguration : public Object
{
  public:
    HtConfiguration();
    ~HtConfiguration() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Enable or disable SGI support.
     *
     * \param enable true if SGI is to be supported,
     *               false otherwise
     */
    void SetShortGuardIntervalSupported(bool enable);
    /**
     * \return whether the device supports SGI.
     *
     * \return true if SGI is supported,
     *         false otherwise.
     */
    bool GetShortGuardIntervalSupported() const;
    /**
     * Enable or disable LDPC support.
     *
     * \param enable true if LDPC is to be supported,
     *               false otherwise
     */
    void SetLdpcSupported(bool enable);
    /**
     * \return whether the device supports LDPC.
     *
     * \return true if LDPC is supported,
     *         false otherwise.
     */
    bool GetLdpcSupported() const;
    /**
     * Enable or disable 40 MHz operation support.
     *
     * \param enable true if both 20 MHz and 40 MHz operation is to be supported,
     *               false if only 20 MHz operation is to be supported
     */
    void Set40MHzOperationSupported(bool enable);
    /**
     * \return true if both 20 MHz and 40 MHz operation is supported, false if
     *         only 20 MHz operation is supported
     */
    bool Get40MHzOperationSupported() const;

  private:
    bool m_sgiSupported;   ///< flag whether short guard interval is supported
    bool m_ldpcSupported;  ///< flag whether LDPC coding is supported
    bool m_40MHzSupported; ///< whether 40 MHz operation is supported
};

} // namespace wifi
} // namespace ns3

#endif /* HT_CONFIGURATION_H */
