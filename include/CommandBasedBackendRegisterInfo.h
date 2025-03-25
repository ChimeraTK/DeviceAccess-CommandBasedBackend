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
    // If updating this, also update registerTypeStrs in CommandBasedBackend.cc
    struct InteractionInfo {
      std::optional<TransportLayerType> transportLayerType;
      std::string commandPattern = "";
      std::string responsePattern = "";
      std::optional<size_t> fixedSizeNumberWidthOpt = std::nullopt;
      std::string cmdLineDelimiter;
      /*--------------------------------------------------------------------------------------------------------------*/
      struct ResponseLinesInfo {
        size_t nLines = 0;
        std::string delimiter;
      };
      struct ResponseBytesInfo {
        size_t nBytesReadResponse = 0;
      };
      enum SendCommandType {
        /* These serve to label the indicies of the types in the responseInfo variant
         * so as to name sendCommand functions to be used in CommandBasedBackend.
         * So, the numeric value MUST match the index order of variants in responseInfo
         */
        SEND_COMMAND_AND_READ_LINES = 0,
        SEND_COMMAND_AND_READ_BYTES = 1
      };
      std::variant<ResponseLinesInfo, ResponseBytesInfo>
          responseInfo; // Default=ResponseLinesInfo
                        // responseInfo variant type order must match the SendCommandType enum values.
      /*--------------------------------------------------------------------------------------------------------------*/
      InteractionInfo() : responseInfo(ResponseLinesInfo{}) {}
      inline bool isActive() const { return not commandPattern.empty(); }
      inline SendCommandType getSendCommandType() const { return static_cast<SendCommandType>(responseInfo.index()); }
      inline bool useReadLines() const { return (getSendCommandType() == SEND_COMMAND_AND_READ_LINES); }
      inline bool isReadBytes() const { return (getSendCommandType() == SEND_COMMAND_AND_READ_BYTES); }
      inline TransportLayerType getTransportLayerType() const { return transportLayerType.value(); }

      // get ResponseLinesInfo if its there
      inline std::optional<ResponseLinesInfo> getResponseLinesInfo() const {
        if(useReadLines()) {
          return std::get<ResponseLinesInfo>(responseInfo);
        }
        return std::nullopt;
      }

      // get ResponseBytesInfo if its there
      inline std::optional<ResponseBytesInfo> getResponseBytesInfo() const {
        // return isReadBytes() ? std::get<ResponseBytesInfo>(responseInfo) : std::nullopt;
        if(isReadBytes()) {
          return std::get<ResponseBytesInfo>(responseInfo);
        }
        return std::nullopt;
      }
      void populateFromJson(const json& j, std::string errorMessageDetail);
    };
    /*----------------------------------------------------------------------------------------------------------------*/
    /*----------------------------------------------------------------------------------------------------------------*/

    explicit CommandBasedBackendRegisterInfo(const RegisterPath& registerPath_ = {}, InteractionInfo readInfo_ = {},
        InteractionInfo writeInfo_ = {}, uint nElements_ = 1);

    explicit CommandBasedBackendRegisterInfo(
        const RegisterPath& registerPath_, const json& j, const std::string& defaultSerialDelimiter);

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
