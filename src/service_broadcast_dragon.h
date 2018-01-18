#ifndef _DRAGON_SERVICE_BROADCAST_H_
#define _DRAGON_SERVICE_BROADCAST_H_

#include "service_broadcast.h"

#include <list>

class DragonServiceBroadcaster : public ServiceBroadcaster {
 public:
  virtual int Initialize();
  virtual void Shutdown();

  // Returns an ID handle for the service. Returns 0 if the operation failed.
  virtual uintptr_t AddService(const ServiceDesc& service);
  virtual int RemoveService(uintptr_t id);

private:
  std::list<uintptr_t> services_;
};

#endif  // _DRAGON_SERVICE_BROADCAST_H_