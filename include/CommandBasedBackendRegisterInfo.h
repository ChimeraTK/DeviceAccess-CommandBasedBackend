// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "mapFileKeys.h"
#include <nlohmann/json.hpp>

#include <ChimeraTK/BackendRegisterInfoBase.h>
#include <ChimeraTK/DataDescriptor.h>

#include <iostream>
#include <memory>
#include <optional>
#include <variant>

using json = nlohmann::json;

namespace ChimeraTK {

  /**
    Holds info all about the command you're sending, but not about the device you're sending it to.
    */
  struct CommandBasedBackendRegisterInfo : public BackendRegisterInfoBase {
    class InteractionInfo {
     protected:
      struct ResponseLinesInfo {
        size_t nLines = 1;
        std::string delimiter;
      };
      struct ResponseBytesInfo {
        size_t nBytesReadResponse = 1;
      };
      /*
       * responseInfo stores information relavent to line delimited reading when reading lines,
       * or information about fixed byte reading when reading bytes.
       * For readability, interact with it through the getters and setters.
       * Default=ResponseLinesInfo
       * responseInfo variant type order indicies must match the SendCommandType enum values
       */
      std::variant<ResponseLinesInfo, ResponseBytesInfo> responseInfo;
      std::optional<TransportLayerType> transportLayerType = std::nullopt;

     public:
      std::string commandPattern = "";
      std::string responsePattern = "";
      std::string cmdLineDelimiter;
      /*
       * fixedRegexCharacterWidthOpt is the hexidecimal character width of the object to be searched for by the regex
       * in the reply string, and the character width inserted while writing.
       * It is set by either the json tags CHARACTER_WIDTH or BIT_WIDTH
       * For strings, its the number of characters in the string.
       * For unsigned decimal integers, its the number of digits without the sign character.
       * For hexInt binInt, and binFloat its the number of hexidecimal CHARACTERS to be searched for, so nibbles, equal
       * to the BIT_WIDTH / 4. Consequently we will be able to support things like 12b binary, but not 13 bit binary.
       * binFloat can only support 16b, 32b, and 64b, corresponding to fixedRegexCharacterWidthOpt = 4, 8, 16 nibbles
       * respectively.
       * It may be used to set the size of the DataType.
       */
      std::optional<size_t> fixedRegexCharacterWidthOpt = std::nullopt;
      std::optional<int> fractionalBitsOpt =
          std::nullopt; // can be negative, needs fixedRegexCharacterWidthOpt to be set
      bool isSigned = false;

      /*--------------------------------------------------------------------------------------------------------------*/
      InteractionInfo() : responseInfo(ResponseLinesInfo{}) {}

      // Set the entire InteractionInfo from json,
      // transportLayerType is exceptional: it typically needs to be set at all levels before other
      // CommandBasedRegisterInfo level quantities can be set. So, when we have to set the type earlier than the rest of
      // interactionInfo, we can skip setting it again by setting skipSetType to true.
      void populateFromJson(const json& j, const std::string& errorMessageDetail, bool skipSetType = false);

      /*
       * If an InteractionInfo is not active, then it is disabled.
       * For example, if readInfo.isActive() is true, then the register is readable;
       * but if readInfo.isActive is false, then it is write-only.
       */
      inline bool isActive() const { return not commandPattern.empty(); }
      inline bool isBinary() const { return _isBinary; }

      inline TransportLayerType getTransportLayerType() const {
        if(not hasTransportLayerType()) {
          throw ChimeraTK::logic_error("Attempting to get a TransportLayerType that has not been set");
        }
        return transportLayerType.value();
      }
      /*
       * These getters return the corresponding responceInfo member, if the corresponding
       * SendCommandType is used, otherwise return nullopt.
       */
      std::optional<size_t> getResponseNLines() const noexcept;
      std::optional<std::string> getResponseLinesDelimiter() const noexcept;
      std::optional<size_t> getResponseBytes() const noexcept;

      /*
       * These set the struct members if responceInfo is already in the corresponding
       * SendCommandType, otherwise they destructively set the SendCommandType,
       * overwriting responseInfo, and then set the struct members.
       */
      void setResponseDelimiter(std::string delimiter) noexcept;
      void setResponseNLines(size_t nLines) noexcept;
      void setResponseBytes(size_t nBytes) noexcept { responseInfo = ResponseBytesInfo{nBytes}; }
      void setTransportLayerType(TransportLayerType& type) noexcept;

      inline bool usesReadLines() const { return std::holds_alternative<ResponseLinesInfo>(responseInfo); }
      inline bool usesReadBytes() const { return std::holds_alternative<ResponseBytesInfo>(responseInfo); }
      inline bool hasTransportLayerType() const { return transportLayerType.has_value(); }
      /*--------------------------------------------------------------------------------------------------------------*/
     protected:
      bool _isBinary = false;
    }; // end InteractionInfo

    /*----------------------------------------------------------------------------------------------------------------*/
    /*----------------------------------------------------------------------------------------------------------------*/

    // Must have the option of 0 arguments
    explicit CommandBasedBackendRegisterInfo(const RegisterPath& registerPath_ = {}, InteractionInfo readInfo_ = {},
        InteractionInfo writeInfo_ = {}, uint nElements_ = 1);

    /**
     * @brief Create and populate a CommandBasedBackendRegisterInfo directly from json.
     * @param[in] defaultSerialDelimiter Sets the serial delimiters in case no delimiter is mentioned in the json.
     */
    explicit CommandBasedBackendRegisterInfo(
        const RegisterPath& registerPath_, const json& j, const std::string& defaultSerialDelimiter);

    /**
     * @brief Init must be run at the end of each constructor. It sets the dataDescriptor and validates the data.
     */
    void init();

    ~CommandBasedBackendRegisterInfo() override = default;

    [[nodiscard]] inline RegisterPath getRegisterName() const override { return registerPath; }

    CommandBasedBackendRegisterInfo& operator=(const CommandBasedBackendRegisterInfo& other) = default;

    [[nodiscard]] inline unsigned int getNumberOfElements() const override { return nElements; }
    [[nodiscard]] inline unsigned int getNumberOfChannels() const override { return nChannels; }
    [[nodiscard]] inline const DataDescriptor& getDataDescriptor() const override { return dataDescriptor; }
    [[nodiscard]] inline bool isReadable() const override { return readInfo.isActive(); }
    [[nodiscard]] inline bool isWriteable() const override { return writeInfo.isActive(); }
    [[nodiscard]] inline AccessModeFlags getSupportedAccessModes() const override { return {}; }

    [[nodiscard]] inline std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
    }

    unsigned int nChannels{1};
    unsigned int nElements{1};
    RegisterPath registerPath; // can be converted to string
    InteractionInfo readInfo;
    InteractionInfo writeInfo;

    DataDescriptor dataDescriptor;
  }; // end CommandBasedBackendRegisterInfo

  // Operators for cout:
  std::ostream& operator<<(std::ostream& os, const CommandBasedBackendRegisterInfo::InteractionInfo& iInfo);

} // end namespace ChimeraTK
