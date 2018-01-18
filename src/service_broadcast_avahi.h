#ifndef _AVAHI_SERVICE_BROADCAST_H_
#define _AVAHI_SERVICE_BROADCAST_H_

#include <avahi-client/publish.h>
#include <string>
#include <vector>

#include "service_broadcast.h"

// Forward declarations
struct AvahiClient;
struct AvahiThreadedPoll;
struct AvahiEntryGroup;

class AvahiServiceBroadcaster : public ServiceBroadcaster {
 public:
  virtual int Initialize() override;
  virtual void Shutdown() override;

  // Returns an ID handle for the service. Returns 0 if the operation failed.
  virtual uintptr_t AddService(const ServiceDesc& service) override;
  virtual int RemoveService(uintptr_t id) override;

 private:
  static void EntryGroupCallback(AvahiEntryGroup* g, AvahiEntryGroupState state,
                                 void* userdata);
  static void ClientCallback(AvahiClient* c, AvahiClientState state,
                             void* userdata);

  std::string name_;
  AvahiThreadedPoll* threaded_poll_;
  AvahiClient* avahi_client_;

  std::vector<ServiceDesc*> services_;
};

#endif  // _AVAHI_SERVICE_BROADCAST_H_