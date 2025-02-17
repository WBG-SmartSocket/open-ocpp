/*
Copyright (c) 2020 Cedric Jimenez
This file is part of OpenOCPP.

OpenOCPP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenOCPP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with OpenOCPP. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STRING_H
#define STRING_H

#include <string>
#include <vector>
namespace ocpp
{
namespace helpers
{

/** @brief Space */
extern const std::string SPACE_STRING;

/** @brief Helper function to trim a string
 *  @param str String to trim
 *  @param chars Characters to remove from the start and from the end of the string
 *  @return Reference to the trimmed string
*/
std::string& trim(std::string& str, const std::string& chars = SPACE_STRING);

/** @brief Helper function to trim a string
 *  @param str String to trim
 *  @param chars Characters to remove from the start of the string
 *  @return Reference to the trimmed string
*/
std::string& ltrim(std::string& str, const std::string& chars = SPACE_STRING);

/** @brief Helper function to trim a string
 *  @param str String to trim
 *  @param chars Characters to remove from the end of the string
 *  @return Reference to the trimmed string
*/
std::string& rtrim(std::string& str, const std::string& chars = SPACE_STRING);

/** @brief Helper function to split a string
 *  @param str String to split
 *  @param separator Separator character
 *  @return Array of splitted substrings
*/
std::vector<std::string> split(const std::string& str, char separator);

/** @brief Helper function to replace a substring with another into a string
 *  @param str String where replace
 *  @param what Substring to search
 *  @param with Substring to use as a replacement
 *  @param replace_all If true, replace all occurences of the [what] substring, otherwise replace only the first one
 *  @return Reference to [str]
 */
std::string& replace(std::string& str, const std::string& what, const std::string& with, bool replace_all = true);

} // namespace helpers
} // namespace ocpp

#endif // STRING_H
