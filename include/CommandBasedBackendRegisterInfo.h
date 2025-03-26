// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "mapFileKeys.h"
#include <nlohmann/json.hpp>

#include <ChimeraTK/BackendRegisterInfoBase.h>
#include <ChimeraTK/DataDescriptor.h>

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

     public:
      std::optional<TransportLayerType> transportLayerType;
      std::string commandPattern = "";
      std::string responsePattern = "";
      std::optional<size_t> fixedSizeNumberWidthOpt = std::nullopt;
      std::string cmdLineDelimiter;

      /*--------------------------------------------------------------------------------------------------------------*/
      InteractionInfo() : responseInfo(ResponseLinesInfo{}) {}

      // Set the entire InteractionInfo from json.
      void populateFromJson(const json& j, const std::string& errorMessageDetail);

      /*
       * If an InteractionInfo is not active, then it is disabled.
       * For example, if readInfo.isActive() is true, then the register is readable;
       * but if readInfo.isActive is false, then it is write-only.
       */
      inline bool isActive() const { return not commandPattern.empty(); }

      inline TransportLayerType getTransportLayerType() const { return transportLayerType.value(); }
      /*
       * These getters return the corresponding responceInfo member, if the corresponding
       * SendCommandType is used, otherwise return nullopt.
       */
      std::optional<size_t> getResponseNLines() const;
      std::optional<std::string> getResponseLinesDelimiter() const;
      std::optional<size_t> getResponseBytes() const;

      /*
       * These set the struct members if responceInfo is already in the corresponding
       * SendCommandType, otherwise they destructively set the SendCommandType,
       * overwriting responseInfo, and then set the struct members.
       */
      void setResponseDelimiter(std::string delimiter);
      void setResponseNLines(size_t nLines);
      void setResponseBytes(size_t nBytes) { responseInfo = ResponseBytesInfo{nBytes}; }

      inline bool usesReadLines() const { return (getSendCommandType() == SEND_COMMAND_AND_READ_LINES); }
      inline bool usesReadBytes() const { return (getSendCommandType() == SEND_COMMAND_AND_READ_BYTES); }
      /*--------------------------------------------------------------------------------------------------------------*/
     protected:
      /*
       * SendCommandType labels the indicies of the types in the responseInfo variant
       * so as to name sendCommand functions to be used in CommandBasedBackend.
       * The numeric value MUST match the index order of variants in responseInfo
       */
      enum SendCommandType { SEND_COMMAND_AND_READ_LINES = 0, SEND_COMMAND_AND_READ_BYTES = 1 };
      inline SendCommandType getSendCommandType() const { return static_cast<SendCommandType>(responseInfo.index()); }
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

} // end namespace ChimeraTK
