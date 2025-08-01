// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "mapFileKeys.h"

#include <ChimeraTK/BackendRegisterInfoBase.h>
#include <ChimeraTK/DataDescriptor.h>

#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <regex>
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
      std::variant<ResponseLinesInfo, ResponseBytesInfo> _responseInfo;
      std::optional<TransportLayerType> _transportLayerType = std::nullopt;

     public:
      std::string commandPattern;
      std::string responsePattern;
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
      InteractionInfo() : _responseInfo(ResponseLinesInfo{}) {}

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
      [[nodiscard]] inline bool isActive() const { return not commandPattern.empty(); }
      [[nodiscard]] inline bool isBinary() const { return _isBinary; }

      [[nodiscard]] inline TransportLayerType getTransportLayerType() const {
        if(not hasTransportLayerType()) {
          throw ChimeraTK::logic_error("Attempting to get a TransportLayerType that has not been set");
        }
        return _transportLayerType.value();
      }
      /*
       * These getters return the corresponding responceInfo member, if the corresponding
       * SendCommandType is used, otherwise return nullopt.
       */
      [[nodiscard]] std::optional<size_t> getResponseNLines() const noexcept;
      [[nodiscard]] std::optional<std::string> getResponseLinesDelimiter() const noexcept;
      [[nodiscard]] std::optional<size_t> getResponseBytes() const noexcept;

      /**
       * @brief Gets the regex pattern string for this InteractionInfo
       * @returns the string regex pattern
       */
      [[nodiscard]] std::string getRegexString() const;

      /*
       * These set the struct members if responceInfo is already in the corresponding
       * SendCommandType, otherwise they destructively set the SendCommandType,
       * overwriting responseInfo, and then set the struct members.
       */
      void setResponseDelimiter(std::string delimiter);
      void setResponseNLines(size_t nLines);
      void setResponseBytes(size_t nBytes) { _responseInfo = ResponseBytesInfo{nBytes}; }
      void setTransportLayerType(TransportLayerType& type) noexcept;

      [[nodiscard]] inline bool usesReadLines() const {
        return std::holds_alternative<ResponseLinesInfo>(_responseInfo);
      }
      [[nodiscard]] inline bool usesReadBytes() const {
        return std::holds_alternative<ResponseBytesInfo>(_responseInfo);
      }
      [[nodiscard]] inline bool hasTransportLayerType() const { return _transportLayerType.has_value(); }
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
     * @brief Validates the data
     * @param[in] errorMessageDetail Fills error strings for logic_erros as needed. It should mention which register this is.
     * @throws a ChimeraTK::logic_error if something is invalid
     */
    void validate(std::string& errorMessageDetail);

    /**
     * @brief finalize must be run at the end of each constructor. It sets the dataDescriptor and validate()'s the data.
     */
    void finalize();

    ~CommandBasedBackendRegisterInfo() override = default;

    // An Impl versions must be used internally, for anything that might be used by the constructor.
    [[nodiscard]] inline RegisterPath getRegisterName() const override { return getRegisterNameImpl(); }
    [[nodiscard]] inline unsigned int getNumberOfElements() const override { return getNumberOfElementsImpl(); }
    [[nodiscard]] inline unsigned int getNumberOfChannels() const override { return getNumberOfChannelsImpl(); }
    [[nodiscard]] inline const DataDescriptor& getDataDescriptor() const override { return dataDescriptor; }
    [[nodiscard]] inline bool isReadable() const override { return readInfo.isActive(); }
    [[nodiscard]] inline bool isWriteable() const override { return writeInfo.isActive(); }
    [[nodiscard]] inline AccessModeFlags getSupportedAccessModes() const override { return {}; }

    [[nodiscard]] inline std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
    }

    /**
     * @brief: Fetches the read response regex given the TransportLayerType
     * @throws std::regex_error or inja::ParserError
     */
    [[nodiscard]] std::regex getReadResponseRegex() const {
      return getResponseRegex(readInfo, "read for " + registerPath);
    }
    /**
     * @brief: Fetches the write response regex given the TransportLayerType
     * @throws std::regex_error or inja::ParserError
     */
    [[nodiscard]] std::regex getWriteResponseRegex() const {
      return getResponseRegex(writeInfo, "write for " + registerPath);
    }

    unsigned int nChannels{1};
    unsigned int nElements{1};
    RegisterPath registerPath; // can be converted to string
    InteractionInfo readInfo;
    InteractionInfo writeInfo;

    DataDescriptor dataDescriptor;

   protected:
    [[nodiscard]] inline RegisterPath getRegisterNameImpl() const { return registerPath; }
    [[nodiscard]] inline unsigned int getNumberOfElementsImpl() const { return nElements; }
    [[nodiscard]] inline unsigned int getNumberOfChannelsImpl() const { return nChannels; }

    /**
     * @brief: Fetches the appropriate regex given the TransportLayerType.
     * @param[in] InteractionInfo
     * @param[in] errorMessageDetial Info useful in the error message, preceeded in error strings by "for ".
     * @throws std::regex_error or inja::ParserError
     */
    [[nodiscard]] std::regex getResponseRegex(const InteractionInfo& info, const std::string& errorMessageDetail) const;

  }; // end CommandBasedBackendRegisterInfo

  // Operators for cout:
  std::ostream& operator<<(std::ostream& os, const CommandBasedBackendRegisterInfo::InteractionInfo& iInfo);

} // end namespace ChimeraTK
