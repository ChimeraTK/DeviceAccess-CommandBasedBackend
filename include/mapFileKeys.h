// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "stringUtils.h"

#include <ChimeraTK/BackendRegisterCatalogue.h>

#include <boost/bimap.hpp>

#include <array>
#include <optional>
#include <string>
#include <unordered_map>

/*
 * Structure of the Map File JSON:
 * {
 *  toStr(mapFileTopLevelKeys::MAP_FILE_FORMAT_VERSION): 2,
 *  toStr(mapFileTopLevelKeys::METADATA): {
 *      toStr(mapFileMetadataKeys::DEFAULT_RECOVERY_REGISTER): <string>,
 *      toStr(mapFileMetadataKeys::DELIMITER): <string>,
 *  },
 *  toStr(mapFileTopLevelKeys::REGISTERS): {
 *      "registerPath1":{
 *          toStr(mapFileRegisterKeys:: ...): <value>,
 *          toStr(mapFileRegisterKeys::TYPE) : toStr(<TransportLayerType 1>),
 *          toStr(mapFileRegisterKeys::WRITE):{
 *              toStr(mapFileInteractionInfoKeys:: ...) : <value>,
 *              [Writing TYPE is already set to TransportLayerType 1. Unless specified, items set at the Register level
 * automatically set values at the InteractionInfo level]
 *          },
 *          toStr(mapFileRegisterKeys::READ):{
 *              toStr(mapFileInteractionInfoKeys:: ...) : <value>
 *              toStr(mapFileInteractionInfoKeys::TYPE) : toStr(<TransportLayerType 2>)
 *              [TYPE is set to TransportLayerType 2 for reading. Values specified at the InteractionInfo level
 * overriding those at the Register level.].
 *
 *          }
 *      },
 *      "registerPath2":{ ... },
 *      "registerPath3":{ ... }
 * }
 *
 * Line delimiters can be set at four levels:
 * 1. The mapFileMetadataKeys::DELIMITER -- the default for all registers
 * 2. The mapFileRegisterKeys::DELIMITER -- overrides the delimiter for that register for all four use cases: {write
 * command, write responce, read command, read response}
 * 3. mapFileInteractionInfoKeys::DELIMITER, whihc sets both the command and responce delimiter for that read or write
 * interaction.
 * 4.  mapFileInteractionInfoKeys::COMMAND_DELIMITER and mapFileInteractionInfoKeys::RESPONSE_DELIMITER, which override
 * all other delimiters for that particular case.
 *
 *  Setting mapFileInteractionInfoKeys::N_RESPONSE_BYTES turns the interaction to binary mode, there is no responce
 * delimiter, and the interaction's mapFileInteractionInfoKeys::COMMAND_DELIMITER defaults to "", overriding the
 * metadata and register delimiter, unless DELIMITER or COMMAND_DELIMITER is explicitly set.
 */
/**********************************************************************************************************************/
/**********************************************************************************************************************/

const int requiredMapFileFormatVersion = 2;

/**********************************************************************************************************************/

/*
 * The point of these getMapForEnum functions is to avoid the oppertunity for bugs from mismatching enums and maps. With
 * them, the pairing only has to be correct in one place--in the getEnumMap function. Then, the enum class names are the
 * only specifiers of JSON structure..
 */
template<typename EnumType>
inline std::unordered_map<EnumType, std::string> getMapForEnum();

// Strictly speaking we don't need templates for this, but templating improves extensibility and style consistency.
template<typename EnumType>
inline boost::bimap<EnumType, std::string> getBimapForEnum();

/**********************************************************************************************************************/
// Static functions for internal use only

/*
 * For making boost::bimap literals.
 */
template<typename L, typename R>
boost::bimap<L, R> makeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
  return boost::bimap<L, R>(list.begin(), list.end());
}

/*
 * Takes a bimap<enums,string>, and a string, and returns an optional to the enum at bimap[str],
 * That optional is the enum if str is in the bimap, and nullopt if not.
 * The string-lookup is case insensitive
 */
template<typename EnumType>
static std::optional<EnumType> getEnumOptFromStrBimapCaseInsensitive(
    std::string str, const boost::bimap<EnumType, std::string> bmap) noexcept {
  toLowerCase(str);
  for(const auto& pair : bmap.left) {
    if(caseInsensitiveStrCompare(str, pair.second)) {
      return pair.first;
    }
  }
  return std::nullopt; // Return empty optional if no match is found
}

/**********************************************************************************************************************/

/*
 * For each of the enum classes below, we can fetch the corresponding json key string using toStr(enum).
 */
template<typename EnumType>
inline std::string toStr(EnumType keyEnum) {
  auto map = getMapForEnum<EnumType>();
  auto it = map.find(keyEnum);
  if(it == map.end()) {
    throw ChimeraTK::logic_error("Unable to convert enum " + std::to_string(static_cast<int>(keyEnum)) + " to string.");
  }
  return it->second;
}

/**********************************************************************************************************************/

enum class mapFileTopLevelKeys {
  MAP_FILE_FORMAT_VERSION,
  METADATA,
  REGISTERS,
};

// Associate json key strings with mapFileTopLevelKeys enums.
template<>
inline std::unordered_map<mapFileTopLevelKeys, std::string> getMapForEnum<mapFileTopLevelKeys>() {
  static const std::unordered_map<mapFileTopLevelKeys, std::string> uMap = {
      // clang-format off
        {mapFileTopLevelKeys::MAP_FILE_FORMAT_VERSION, "mapFileFormatVersion"}, 
        {mapFileTopLevelKeys::METADATA, "metadata"},
        {mapFileTopLevelKeys::REGISTERS, "registers"} // clang-format on
  };
  return uMap;
}

/**********************************************************************************************************************/

enum class mapFileMetadataKeys {
  DEFAULT_RECOVERY_REGISTER,
  DELIMITER,
};

// Associate json key strings with mapFileMetadataKeys enums.
template<>
inline std::unordered_map<mapFileMetadataKeys, std::string> getMapForEnum<mapFileMetadataKeys>() {
  static const std::unordered_map<mapFileMetadataKeys, std::string> uMap = {
      // clang-format off
        {mapFileMetadataKeys::DEFAULT_RECOVERY_REGISTER, "defaultRecoveryRegister"},
        {mapFileMetadataKeys::DELIMITER, "delimiter"} // clang-format on
  };
  return uMap;
}

/**********************************************************************************************************************/

enum class mapFileRegisterKeys {
  WRITE,
  READ,
  N_ELEM,
  TYPE, // TYPE and below need to be in common with mapFileInteractionInfoKeys
  N_RESPONSE_BYTES,
  N_RESPONSE_LINES,
  DELIMITER,
  COMMAND_DELIMITER,
  RESPONSE_DELIMITER,
  CHARACTER_WIDTH,
  BIT_WIDTH,
  FRACTIONAL_BITS,
  SIGNED,
};

// Associate json key strings with mapFileRegisterKeys enums.
template<>
inline std::unordered_map<mapFileRegisterKeys, std::string> getMapForEnum<mapFileRegisterKeys>() {
  static const std::unordered_map<mapFileRegisterKeys, std::string> uMap = {
      // clang-format off
        {mapFileRegisterKeys::WRITE, "write"}, 
        {mapFileRegisterKeys::READ, "read"}, 
        {mapFileRegisterKeys::N_ELEM, "nElem"}, 
        {mapFileRegisterKeys::TYPE, "type"}, //TYPE and below need to be in common with mapFileInteractionInfoKeys
        {mapFileRegisterKeys::N_RESPONSE_BYTES, "nRespBytes"},
        {mapFileRegisterKeys::N_RESPONSE_LINES, "nRespLines"}, 
        {mapFileRegisterKeys::DELIMITER, "delimiter"},
        {mapFileRegisterKeys::COMMAND_DELIMITER, "cmdDelim"}, 
        {mapFileRegisterKeys::RESPONSE_DELIMITER, "respDelim"},
        {mapFileRegisterKeys::CHARACTER_WIDTH, "characterWidth"}, 
        {mapFileRegisterKeys::BIT_WIDTH, "bitWidth"}, 
        {mapFileRegisterKeys::FRACTIONAL_BITS, "fractionalBits"},
        {mapFileRegisterKeys::SIGNED, "signed"},
      // clang-format on
  };
  return uMap;
}

/**********************************************************************************************************************/

enum class mapFileInteractionInfoKeys {
  COMMAND,
  RESPESPONSE,
  TYPE, // TYPE and below need to be in common with mapFileRegisterKeys
  N_RESPONSE_BYTES,
  N_RESPONSE_LINES,
  DELIMITER,
  COMMAND_DELIMITER,
  RESPONSE_DELIMITER,
  CHARACTER_WIDTH,
  BIT_WIDTH,
  FRACTIONAL_BITS,
  SIGNED,
};

// Associate json key strings with mapFileInteractionInfoKeys enums.
template<>
inline std::unordered_map<mapFileInteractionInfoKeys, std::string> getMapForEnum<mapFileInteractionInfoKeys>() {
  static const std::unordered_map<mapFileInteractionInfoKeys, std::string> uMap = {
      // clang-format off
        {mapFileInteractionInfoKeys::COMMAND, "cmd"}, 
        {mapFileInteractionInfoKeys::RESPESPONSE, "resp"},

        //unordered_map<mapFileRegisterKeys.. is the single source of truth for these shared JSON key strings.
        {mapFileInteractionInfoKeys::TYPE, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::TYPE)},

        {mapFileInteractionInfoKeys::N_RESPONSE_BYTES, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::N_RESPONSE_BYTES)},

        {mapFileInteractionInfoKeys::N_RESPONSE_LINES, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::N_RESPONSE_LINES)},

        {mapFileInteractionInfoKeys::DELIMITER, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::DELIMITER)},

        {mapFileInteractionInfoKeys::COMMAND_DELIMITER, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::COMMAND_DELIMITER)},

        {mapFileInteractionInfoKeys::RESPONSE_DELIMITER, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::RESPONSE_DELIMITER)},

        {mapFileInteractionInfoKeys::CHARACTER_WIDTH, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::CHARACTER_WIDTH)},

        {mapFileInteractionInfoKeys::BIT_WIDTH, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::BIT_WIDTH)},

        {mapFileInteractionInfoKeys::FRACTIONAL_BITS, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::FRACTIONAL_BITS)},

        {mapFileInteractionInfoKeys::SIGNED, 
            getMapForEnum<mapFileRegisterKeys>().at(mapFileRegisterKeys::SIGNED)},
      // clang-format on
  };
  return uMap;
}
/**********************************************************************************************************************/

/** Internal representation type to which we have to convert successfully.*/
enum class TransportLayerType {
  DEC_INT,
  HEX_INT,
  BIN_INT,
  BIN_FLOAT,
  DEC_FLOAT,
  STRING,
  VOID,
};

template<>
inline boost::bimap<TransportLayerType, std::string> getBimapForEnum<TransportLayerType>() {
  static const auto bmap = makeBimap<TransportLayerType, std::string>({
      // clang-format off
            {TransportLayerType::DEC_INT, "decInt"},
            {TransportLayerType::HEX_INT, "hexInt"},
            {TransportLayerType::BIN_INT, "binInt"},
            {TransportLayerType::BIN_FLOAT, "binFloat"},
            {TransportLayerType::DEC_FLOAT, "decFloat"},
            {TransportLayerType::STRING, "string"},
            {TransportLayerType::VOID, "void"},
      // clang-format on
  });
  return bmap;
}

template<>
inline std::string toStr<TransportLayerType>(TransportLayerType keyEnum) {
  auto bimap = getBimapForEnum<TransportLayerType>();
  auto it = bimap.left.find(keyEnum);
  if(it == bimap.left.end()) {
    throw ChimeraTK::logic_error("Unable to convert TransportLayerType enum " +
        std::to_string(static_cast<int>(keyEnum)) + " to string; Check getBimapForEnum.");
  }
  return it->second; // Safe access
}
// To get a std::optional<TransportLayerType> from a string, use
// strToEnumOpt<TransportLayerType>(std::string typeString)

/*
 * signedTransportLayerTypeToDataTypeMap and unsignedTransportLayerTypeToDataTypeMap
 * hold the default relationships between the TransportLayerType and the DataType
 * for signed and unsigned values. These will be second-guessed.
 */
namespace ChimeraTK {
  static const std::unordered_map<TransportLayerType, DataType> signedTransportLayerTypeToDataTypeMap = {
      // clang-format off
            {TransportLayerType::DEC_INT, DataType::int64},
            {TransportLayerType::HEX_INT, DataType::int64},
            {TransportLayerType::BIN_INT, DataType::int64},
            {TransportLayerType::BIN_FLOAT, DataType::float64},
            {TransportLayerType::DEC_FLOAT, DataType::float64},
            {TransportLayerType::STRING, DataType::string},
            {TransportLayerType::VOID, DataType::Void}
      // clang-format on
  };

  static const std::unordered_map<TransportLayerType, DataType> unsignedTransportLayerTypeToDataTypeMap = {
      // clang-format off
            {TransportLayerType::DEC_INT, DataType::uint64},
            {TransportLayerType::HEX_INT, DataType::uint64},
            {TransportLayerType::BIN_INT, DataType::uint64},
            {TransportLayerType::BIN_FLOAT, DataType::float64},
            {TransportLayerType::DEC_FLOAT, DataType::float64},
            {TransportLayerType::STRING, DataType::string},
            {TransportLayerType::VOID, DataType::Void}
      // clang-format on
  };

} // namespace ChimeraTK

/**********************************************************************************************************************/

template<typename EnumType>
[[nodiscard]] std::optional<TransportLayerType> strToEnumOpt(const std::string& str) noexcept {
  return getEnumOptFromStrBimapCaseInsensitive(str, getBimapForEnum<EnumType>());
}

/**********************************************************************************************************************/
