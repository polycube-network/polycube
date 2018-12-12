/**
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file utils.hpp
 * Main libviface utilities header file.
 * Define utility functions for libviface.
 */

#ifndef _VIFACE_UTILS_HPP
#define _VIFACE_UTILS_HPP

#include "viface/viface.hpp"

namespace viface
{
namespace utils
{
/**
 * @ingroup libviface Public Interface
 * @{
 */

/**
 * Parse a MAC address in string format.
 *
 * @param[in]  mac MAC address to parse in format "56:84:7a:fe:97:99".
 *
 * @return A vector of 6 bytes with the parsed MAC address.
 *         If the string cannot be parsed an invalid_argument exception is
 *         thrown.
 */
std::vector<uint8_t> parse_mac(std::string const& mac);

/**
 * Build a hexdump representation of a binary blob.
 *
 * @param[in]  bytes Binary blob (array of bytes) with the data to build
 *             representation.
 *
 * @return A string with the hexdump representation of the input binary blob.
 */
std::string hexdump(std::vector<uint8_t> const& bytes);

/**
 * Calculate the 32 bit CRC of the given binary blob.
 *
 * @param[in]  bytes Binary blob (array of bytes) with the data to calculate
 *             the 32 bit CRC.
 */
uint32_t crc32(std::vector<uint8_t> const& bytes);

/** @} */ // End of libviface
};
};
#endif // _VIFACE_UTILS_HPP
