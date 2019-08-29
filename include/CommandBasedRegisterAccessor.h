#ifndef COMMANDBASEDREGISTERACCESSOR_H
#define COMMANDBASEDREGISTERACCESSOR_H

#include <ChimeraTK/SyncNDRegisterAccessor.h>

namespace CommandBasedBackend{

  template<typename UserType>
  class CommandBasedRegisterAccessor : public ChimeraTK::SyncNDRegisterAccessor<UserType> {

    CommandBasedRegisterAccessor(){
      try {

      } catch (...) {
        this->shutdown();
        throw;
      }
    }

    virtual ~CommandBasedRegisterAccessor(){ this->shutdown(); }

  };

} // CommandBasedBackend
#endif // COMMANDBASEDREGISTERACCESSOR_H
