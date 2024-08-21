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
  class CommandBasedBackendRegisterInfo : public BackendRegisterInfoBase {
   public:
    enum class IoDirection { READ_ONLY, WRITE_ONLY, READ_WRITE }; // TODO surely there's something like this already

    // CommandBasedBackendRegisterInfo() {} // needed and solves compiler bugs, replaced by default arguments
    CommandBasedBackendRegisterInfo(std::string commandName = "", RegisterPath registerPath = "", uint nChannels = 1,
        uint nElements = 1, IoDirection ioDir = IoDirection::READ_WRITE, AccessModeFlags accessModeFlags = {},
        const DataDescriptor& dataDescriptor = {})
    : _nChannels(nChannels), _nElements(nElements), _ioDir(ioDir), _accessModeFlags(accessModeFlags),
      _registerPath(registerPath), _commandName(commandName), _dataDescriptor(dataDescriptor) {}

    // Copy constructor
    CommandBasedBackendRegisterInfo(const CommandBasedBackendRegisterInfo& other)
    : _nChannels(other._nChannels), _nElements(other._nElements), _ioDir(other._ioDir),
      _accessModeFlags(other._accessModeFlags), _registerPath(other._registerPath), _commandName(other._commandName),
      _dataDescriptor(other._dataDescriptor) {}

    ~CommandBasedBackendRegisterInfo() override = default;

    /** Return full path name of the register (including modules) */
    [[nodiscard]] RegisterPath getRegisterName() const override { return _registerPath; }
    // The name identified in the DA interface.
    // coming in through the constructor.

    CommandBasedBackendRegisterInfo& operator=(const CommandBasedBackendRegisterInfo& other) = default; //

    /** Return number of elements per channel */
    [[nodiscard]] unsigned int getNumberOfElements() const override { return _nElements; }

    /** Return number of channels in register */
    [[nodiscard]] unsigned int getNumberOfChannels() const override { return _nChannels; }

    /** Return number of dimensions of this register */
    //[[nodiscard]] unsigned int getNumberOfDimensions() override { return getNumberOfDimensions(); }

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    [[nodiscard]] const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }

    /** Return whether the register is readable. */
    bool isReadable() const override { return _ioDir == IoDirection::READ_ONLY or _ioDir == IoDirection::READ_WRITE; }

    inline bool isNotReadable() const { return _ioDir == IoDirection::WRITE_ONLY; }

    inline bool isReadOnly() const { return _ioDir == IoDirection::READ_ONLY; }

    /** Return whether the register is writeable. */
    bool isWriteable() const override { return _ioDir == IoDirection::WRITE_ONLY or _ioDir == IoDirection::READ_WRITE; }

    inline bool isNotWritable() const { return _ioDir == IoDirection::READ_ONLY; }

    /** Return all supported AccessModes for this register */
    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const override { return _accessModeFlags; }

    /** Create copy of the object */
    [[nodiscard]] std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::make_unique<CommandBasedBackendRegisterInfo>(*this);
    }

    std::string getCommandName() { return _commandName; }

   protected:
    unsigned int _nChannels = 1;
    unsigned int _nElements = 1;
    IoDirection _ioDir; // RW flags
    AccessModeFlags _accessModeFlags;
    RegisterPath _registerPath;

    /** The SCPI base command.
     *  This will likely get more complicated
     */
    std::string _commandName;

    DataDescriptor _dataDescriptor;
  }; // end CommandBasedBackendRegisterInfo

} // end namespace ChimeraTK
