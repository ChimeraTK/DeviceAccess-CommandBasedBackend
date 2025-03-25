// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "stringUtils.h"
#include <unordered_map>

#include <array>
#include <optional>
#include <string>

/*
 * Structure of the Map File JSON:
 * {
 *  toStr(mapFileTopLevelKeys::MAP_FILE_FORMAT_VERSION): 2,
 *  toStr(mapFileTopLevelKeys::METADATA): {
 *      toStr(mapFileMetadataKeys::DEFAULT_RECOVERY_REGISTER): <string>,
 *      toStr(mapFileMetadataKeys::DELIMITER): <string>,
 *  },
 *  toStr(mapFileTopLevelKeys::REGISTERS: {
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
// TODO refactor this to use maps rather than arrays. Maybe undordered_map, maybe bimap
//   Will have to modify throwIfHasInvalidJsonKeyCaseInsensitive

enum class mapFileTopLevelKeys {
  MAP_FILE_FORMAT_VERSION = 0,
  METADATA,
  REGISTERS,
  // Add other keys here.
  SIZE // Keep this at the end so as to automatically be the count of keys.
};

static const std::array<std::string, static_cast<size_t>(mapFileTopLevelKeys::SIZE)> topLevelKeyStrs = {
    // Indexed by mapFileTopLevelKeys so keep them in the same order.
    "mapFileFormatVersion", "metadata", "registers"};

inline std::string toStr(mapFileTopLevelKeys keyEnum) {
  return topLevelKeyStrs[static_cast<int>(keyEnum)];
}

/**********************************************************************************************************************/

enum class mapFileMetadataKeys {
  DEFAULT_RECOVERY_REGISTER = 0,
  DELIMITER,
  // Add other keys here.
  SIZE // Keep this at the end so as to automatically be the count of keys.
};

static const std::array<std::string, static_cast<size_t>(mapFileMetadataKeys::SIZE)> metadataKeyStrs = {
    // Indexed by mapFileMetadataKeys so keep them in the same order.
    "defaultRecoveryRegister", "delimiter"};

inline std::string toStr(mapFileMetadataKeys keyEnum) {
  return metadataKeyStrs[static_cast<int>(keyEnum)];
}
/**********************************************************************************************************************/

enum class mapFileRegisterKeys {
  WRITE = 0,
  READ,
  TYPE,
  N_ELEM,
  DELIMITER,
  COMMAND_DELIMITER,
  RESPONSE_DELIMITER,
  FIXED_SIZE_NUM_WIDTH,
  // Add other keys here.
  SIZE // Keep this at the end so as to automatically be the count of keys.
};
static const std::array<std::string, static_cast<size_t>(mapFileRegisterKeys::SIZE)> registerKeyStrs = {
    // Indexed by mapFileRegisterKeys so keep them in the same order.
    "write",     // WRITE
    "read",      // READ,
    "type",      // TYPE,
    "nElem",     // N_ELEM,
    "delimiter", // DELIMITER,
    "cmdDelim",  // COMMAND_DELIMITER,
    "respDelim", // RESPONSE_DELIMITER,
    "fixedWidth" // FIXED_SIZE_NUM_WIDTH,
};

inline std::string toStr(mapFileRegisterKeys keyEnum) {
  return registerKeyStrs[static_cast<int>(keyEnum)];
}

/**********************************************************************************************************************/

enum class mapFileInteractionInfoKeys {
  COMMAND = 0,
  RESPESPONSE,
  TYPE,
  N_RESPONSE_BYTES,
  N_RESPONSE_LINES,
  DELIMITER,
  COMMAND_DELIMITER,
  RESPONSE_DELIMITER,
  FIXED_SIZE_NUM_WIDTH,
  // TODO add checksum
  //  Add other keys here.
  SIZE // Keep this at the end so as to automatically be the count of keys.
};
static const std::array<std::string, static_cast<size_t>(mapFileInteractionInfoKeys::SIZE)> interactionInfoKeyStrs = {
    // Indexed by mapFileInteractionInfoKeys so keep them in the same order.
    "cmd",        // COMMAND
    "resp",       // RESPESPONSE,
    "type",       // TYPE,
    "nRespBytes", // N_RESPOCNE_BYTES,
    "nRespLines", // N_RESPONCE_LINES,
    "delimiter",  // DELIMITER,
    "cmdDelim",   // COMMAND_DELIMITER,
    "respDelim",  // RESPONSE_DELIMITER,
    "fixedWidth"  // FIXED_SIZE_NUM_WIDTH,
};
inline std::string toStr(mapFileInteractionInfoKeys keyEnum) {
  return registerKeyStrs[static_cast<int>(keyEnum)];
}

/**********************************************************************************************************************/

/**
 * Internal representation type to which we have to convert successfully.
 * To add a new type, you must update
 * CommandBasedBackendRegisterInfo.cc: populate the data descriptor in the CommandBasedBackendRegisterInfo
 * CommandBasedBackendRegisterAccessor.cc: update getRegex, getToUserTypeFunction, and getToTransportLayerFunction.
 */
enum class TransportLayerType {
  // must be in the same order
  DEC_INT = 0,
  HEX_INT,
  BIN_INT,
  DEC_FLOAT,
  STRING,
  VOID,
  // Add other keys here.
  SIZE // Keep this at the end so as to automatically be the count of keys.
};

static const std::array<std::string, static_cast<size_t>(TransportLayerType::SIZE)> registerTypeStrs = {
    // Indexed by TransportLayerType so keep them in the same order:
    "decInt",
    "hexInt",
    "binInt",
    "decFloat",
    "string",
    "void",
};

inline std::string toStr(TransportLayerType eType) {
  return registerTypeStrs[static_cast<int>(eType)];
}

[[nodiscard]] std::optional<TransportLayerType> typeStrToEnum(std::string typeStr) noexcept {
  toLowerCase(typeStr);
  for(size_t iType = 0; iType < registerTypeStrs.size(); ++iType) {
    if(caseInsensitiveStrCompare(typeStr, registerTypeStrs[iType])) {
      return static_cast<TransportLayerType>(iType);
    }
  }
  return std::nullopt; // Return empty optional if no match is found
}

/**********************************************************************************************************************/
