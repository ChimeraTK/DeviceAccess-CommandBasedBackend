// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Checksum.h"
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
  struct CommandBasedBackendRegisterInfo;

  enum class ReadWriteMode : bool { READ, WRITE };

  class InteractionInfo {
   protected:
    ReadWriteMode _readWriteMode; // For error reporting.
    struct ResponseLinesInfo {
      size_t nLines = 0;
      std::string delimiter;
    };
    struct ResponseBytesInfo {
      size_t nBytesReadResponse = 0;
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
    std::vector<std::string> commandChecksumPayloadStrs; // semgnetns of the commandPattern which are inja tempaltes of
                                                         // the checksum payloads that will be inputs to the checksums.

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
    std::optional<int> fractionalBitsOpt = std::nullopt; // can be negative, needs fixedRegexCharacterWidthOpt to be set
    bool isSigned = false;

    std::vector<checksum> commandChecksumEnums;
    std::vector<checksum> responseChecksumEnums;

    std::string errorMessageDetail;

    /*----------------------------------------------------------------------------------------------------------------*/
    explicit InteractionInfo(ReadWriteMode readWriteMode)
    : _readWriteMode(readWriteMode), _responseInfo(ResponseLinesInfo{}) {}

    /**
     * Set the entire InteractionInfo from json,
     * transportLayerType is exceptional: it typically needs to be set at all levels before other
     * CommandBasedRegisterInfo level quantities can be set. So, when we have to set the type earlier than the rest of
     * interactionInfo, we can skip setting it again by setting skipSetType to true.
     */
    void populateFromJson(const json& j, bool skipSetType = false);

    /*
     * If an InteractionInfo is not active, then it is disabled.
     * For example, if readInfo.isActive() is true, then the register is readable;
     * but if readInfo.isActive is false, then it is write-only.
     */
    [[nodiscard]] inline bool isActive() const { return not commandPattern.empty(); }
    [[nodiscard]] inline bool isBinary() const { return _isBinary; }

    [[nodiscard]] inline std::string readWriteStr() const {
      return (_readWriteMode == ReadWriteMode::READ ? "read" : "write");
    }
    [[nodiscard]] TransportLayerType getTransportLayerType() const;

    /*
     * These getters return the corresponding responceInfo member, if the corresponding
     * SendCommandType is used, otherwise return nullopt.
     */
    [[nodiscard]] std::optional<size_t> getResponseNLines() const noexcept;
    [[nodiscard]] std::optional<std::string> getResponseLinesDelimiter() const noexcept;
    [[nodiscard]] std::optional<size_t> getResponseBytes() const noexcept;

    /**
     * @brief Gets the regex pattern string for this InteractionInfo's type
     * @returns the string regex pattern as a capture group
     */
    [[nodiscard]] std::string getRegexString() const;

    /**
     * @brief: Fetches the appropriate regex given the TransportLayerType.
     * @param[in] nElements The number of elements from the CommandBasedBackendRegisterInfo
     * @throws std::regex_error or inja::ParserError
     */
    [[nodiscard]] std::regex getResponseDataRegex(size_t nElements) const;
    [[nodiscard]] std::regex getResponseChecksumRegex(size_t nElements) const;
    [[nodiscard]] std::regex getResponseChecksumPayloadRegex(size_t nElements) const;

    /*
     * Get the DataType best suited to the InteractionInfo info.
     * This considers iInfo.isSigned, and the fixedRegexCharacterWidthOpt
     * returns the smallest datatype that meets those needs.
     */
    [[nodiscard]] DataType getDataType() const;

    /*
     * These set the struct members if responceInfo is already in the corresponding
     * SendCommandType, otherwise they destructively set the SendCommandType,
     * overwriting responseInfo, and then set the struct members.
     */
    void setResponseDelimiter(std::string delimiter);
    void setResponseNLines(size_t nLines);
    void setResponseBytes(size_t nBytes) { _responseInfo = ResponseBytesInfo{nBytes}; }
    void setTransportLayerType(TransportLayerType& type) noexcept;
    inline void setErrorMessageDetail(const std::string& detail) noexcept {
      errorMessageDetail = readWriteStr() + " for " + detail;
    }

    [[nodiscard]] inline bool usesReadLines() const { return std::holds_alternative<ResponseLinesInfo>(_responseInfo); }
    [[nodiscard]] inline bool usesReadBytes() const { return std::holds_alternative<ResponseBytesInfo>(_responseInfo); }
    [[nodiscard]] inline bool hasTransportLayerType() const { return _transportLayerType.has_value(); }
    /*----------------------------------------------------------------------------------------------------------------*/
   protected:
    bool _isBinary = false;

    /**
     * @brief Validates the line endings for the interaction info.
     * Enforces that responseLinesDelimiter is not empty if the interaction uses read lines.
     * @throws ChimeraTK::logic_error if invalid.
     */
    void throwIfBadEndings() const;

    /**
     * @brief Validates fixedRegexCharacterWidthOpt, particularly its interaction with the transportLayerType.
     * Ensures that fixedRegexCharacterWidthOpt is set if iInfo.isBinary().
     * transportLayerType must be set before running this
     * @throws ChimeraTK::logic_error If fixedRegexCharacterWidthOpt is invalid for the type.
     */
    void throwIfBadFixedWidth() const;

    /**
     * @brief Validates fractionalBitsOpt, and its interactions.
     * Checks interactions with the transportLayerType, fractionalBitsOpt, fixedRegexCharacterWidthOpt, and signed.
     * transportLayerType must be set before running this
     * @throws ChimeraTK::logic_error If fractionalBitsOpt or its interactions is invalid.
     */
    void throwIfBadFractionalBits() const;

    /**
     * @brief Validates the signed property, making sure its a valid property for the TransportLayerType.
     * transportLayerType must be set before running this
     * @throws ChimeraTK::logic_error If the signed property is invalid for the type.
     */
    void throwIfBadSigned() const;

    /**
     * @brief Throws unless the transport layer type is set.
     * @throws ChimeraTK::logic_error
     */
    void throwIfTransportLayerTypesIsNotSet() const;

    /**
     * @brief Validates that the number of checksum entries matches the number of inja checksum payloads in the command
     * and response pattern.
     * @throws ChimeraTK::logic_error If there is a size missmatch, or an invalid checksum inja patterns.
     */
    void throwIfBadChecksum(size_t nElements) const;

    friend struct CommandBasedBackendRegisterInfo;
  }; // end InteractionInfo

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /**
    Holds info all about the command you're sending, but not about the device you're sending it to.
    */
  struct CommandBasedBackendRegisterInfo : public BackendRegisterInfoBase {
    // Must have the option of 0 arguments
    explicit CommandBasedBackendRegisterInfo(const RegisterPath& registerPath_ = {},
        InteractionInfo readInfo_ = InteractionInfo(ReadWriteMode::READ),
        InteractionInfo writeInfo_ = InteractionInfo(ReadWriteMode::WRITE), uint nElements_ = 1);

    /**
     * @brief Create and populate a CommandBasedBackendRegisterInfo directly from json.
     * @param[in] defaultSerialDelimiter Sets the serial delimiters in case no delimiter is mentioned in the json.
     */
    explicit CommandBasedBackendRegisterInfo(
        const RegisterPath& registerPath_, const json& j, const std::string& defaultSerialDelimiter);

    /**
     * @brief Validates the data
     * @throws a ChimeraTK::logic_error if something is invalid
     */
    void validate() const;

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
     * @brief: Fetches the read response data regex given the TransportLayerType
     * @throws std::regex_error
     */
    [[nodiscard]] std::regex getReadResponseDataRegex() const {
      return readInfo.getResponseDataRegex(getNumberOfElementsImpl());
    }
    /**
     * @brief: Fetches the write response data regex given the TransportLayerType
     * @throws std::regex_error
     */
    [[nodiscard]] std::regex getWriteResponseDataRegex() const {
      return writeInfo.getResponseDataRegex(getNumberOfElementsImpl());
    }
    [[nodiscard]] std::regex getReadResponseChecksumRegex() const {
      return readInfo.getResponseChecksumRegex(getNumberOfElementsImpl());
    }
    [[nodiscard]] std::regex getWriteResponseChecksumRegex() const {
      return writeInfo.getResponseChecksumRegex(getNumberOfElementsImpl());
    }
    [[nodiscard]] std::regex getReadResponseChecksumPayloadRegex() const {
      return readInfo.getResponseChecksumPayloadRegex(getNumberOfElementsImpl());
    }
    [[nodiscard]] std::regex getWriteResponseChecksumPayloadRegex() const {
      return writeInfo.getResponseChecksumPayloadRegex(getNumberOfElementsImpl());
    }

    unsigned int nChannels{1};
    unsigned int nElements{1};
    RegisterPath registerPath; // can be converted to string
    InteractionInfo readInfo{ReadWriteMode::READ};
    InteractionInfo writeInfo{ReadWriteMode::WRITE};

    DataDescriptor dataDescriptor;

   protected:
    std::string _errorMessageDetail;
    /*----------------------------------------------------------------------------------------------------------------*/
    // Private getters
    [[nodiscard]] inline RegisterPath getRegisterNameImpl() const { return registerPath; }
    [[nodiscard]] inline unsigned int getNumberOfElementsImpl() const { return nElements; }
    [[nodiscard]] inline unsigned int getNumberOfChannelsImpl() const { return nChannels; }

    /**
     * @brief Gets a single coherent data type from the two possible data types in writeInfo and readInfo, for the sake
     * of setting the DataDescriptor.
     * @throws ChimeraTK::logic_error if the readInfo and writeInfo have incompatible data types.
     */
    [[nodiscard]] DataType getDataType() const;

    /*----------------------------------------------------------------------------------------------------------------*/
    // Privte Setters
    /**
     * Sets errorMessageDetail in the CommandBasedBackendRegisterInfo and InteractionInfo's
     */
    void setErrorMessageDetail();

    /**
     * @brief Sets iInfo.nElements from JSON, if present, or else sets it to 1.
     * @param[in] j nlohmann::json from the map file
     * @throws ChimeraTK::logic_error if nElements is set <= 0 in the JSON.
     */
    void setNElementsFromJson(const json& j);

    /*----------------------------------------------------------------------------------------------------------------*/

    /**
     * @brief Synchronizes the TransportLayerType of writeInfo and readInfo in case one is missing its TransportLayerType
     * The type is copied from the other InteractionInfo
     * The InteractionInfo is expected to lack a transport type if it is never activated, but the type is needed
     * Doing this synchronization simplifies later logic, such as throwIfBadNElements.
     */
    void synchronizeTransportLayerTypes();

    /*----------------------------------------------------------------------------------------------------------------*/
    // Private Data Validaters

    /**
     * @brief This enforces that the register is either readable, writeable, or both.
     * @throws ChimeraTK::logic_error
     */
    void throwIfBadActivation() const;

    /**
     * @brief This checks for the obvious error of having a command response patterns without corresponding command
     * patterns. Requires the number of elements to already be set.
     * @throws ChimeraTK::logic_error
     */
    void throwIfBadCommandAndResponsePatterns() const;

    /**
     * @brief Validates nElements, particularly its interaction with VOID type.
     * Enforces that void type registers must be write-only, have nElements = 1, and have no inja variables in their commands.
     * @throws ChimeraTK::logic_error If nElements is incompatible with void type.
     */
    void throwIfBadNElements() const;

    /**
     * @brief Throws unless at least one of the two interaction Infos has a type set.
     * @throws ChimeraTK::logic_error If no type has been set.
     */
    void throwIfATransportLayerTypeIsNotSet() const;
  }; // end CommandBasedBackendRegisterInfo
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  // Operators for cout:
  std::ostream& operator<<(std::ostream& os, const InteractionInfo& iInfo);

} // end namespace ChimeraTK
