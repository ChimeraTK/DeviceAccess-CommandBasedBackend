// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/BackendRegisterInfoBase.h>
#include <ChimeraTK/DataDescriptor.h>

#include <memory>
namespace ChimeraTK {

  /**
    Holds info all about the command you're sending, but not about the device you're sending it to.
    */
  struct CommandBasedBackendRegisterInfo : public BackendRegisterInfoBase {
    /** Internal representation type to which we have to convert successfully.*/
    enum class InternalType { INT64 = 0, UINT64, HEX, DOUBLE, STRING, VOID };

    explicit CommandBasedBackendRegisterInfo(const RegisterPath& registerPath_ = {},
        std::string writeCommandPattern_ = "", std::string writeResponsePattern_ = "",
        std::string readCommandPattern_ = "", std::string readResponsePattern_ = "", uint nElements_ = 1,
        size_t nLinesReadResponse_ = 1, InternalType type = InternalType::INT64, std::string delimiter_ = "\r\n");
    ~CommandBasedBackendRegisterInfo() override = default;

    [[nodiscard]] inline RegisterPath getRegisterName() const override { return registerPath; }

    CommandBasedBackendRegisterInfo& operator=(const CommandBasedBackendRegisterInfo& other) = default;

    [[nodiscard]] inline unsigned int getNumberOfElements() const override { return nElements; }
    [[nodiscard]] inline unsigned int getNumberOfChannels() const override { return nChannels; }
    [[nodiscard]] inline const DataDescriptor& getDataDescriptor() const override { return dataDescriptor; }
    [[nodiscard]] inline bool isReadable() const override { return !readCommandPattern.empty(); }
    [[nodiscard]] inline bool isWriteable() const override { return !writeCommandPattern.empty(); }
    [[nodiscard]] inline AccessModeFlags getSupportedAccessModes() const override { return {}; }

    [[nodiscard]] inline std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
    }

    unsigned int nChannels{1};
    unsigned int nElements{1};
    RegisterPath registerPath;

    std::string writeCommandPattern;
    std::string writeResponsePattern;
    size_t nLinesWriteResponse{0}; // FIXME: extract from pattern; Ticket 13531
                                   // TODO remove default value if it's set in the constructor

    std::string readCommandPattern;
    std::string readResponsePattern;
    size_t nLinesReadResponse;

    InternalType internalType;
    DataDescriptor dataDescriptor;

    /**
     * The delimiter between lines.
     */
    std::string delimiter;

    [[nodiscard]] inline bool operator==(const CommandBasedBackendRegisterInfo& other) const = default;

  }; // end CommandBasedBackendRegisterInfo

} // end namespace ChimeraTK
