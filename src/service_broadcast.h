#ifndef _SERVICE_BROADCAST_H_
#define _SERVICE_BROADCAST_H_

#include <cstdint>

struct ServiceDesc {
  const char* name;    // user-friendly hostname
  const char* type;    // service type
  const char* domain;  // service domain (can be null)
  const char* host;    // host (can be null)
  const char* data;    // JSON data (can be null)
  uint16_t port;
};

class ServiceBroadcaster {
 public:
  virtual int Initialize() = 0;
  virtual void Shutdown() = 0;

  // Returns an ID handle for the service. Returns 0 if the operation failed.
  virtual uintptr_t AddService(const ServiceDesc& service) = 0;
  virtual int RemoveService(uintptr_t id) = 0;
};

#endif  // _SERVICE_BROADCAST_H_