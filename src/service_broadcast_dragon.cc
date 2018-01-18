#include "service_broadcast_dragon.h"

#include <cstdio>
#include <cstring>

#include <unistd.h>

#include "logging.h"

struct InternalDragonService {
  DragonServiceBroadcaster* sb;
  const char* service_filename;
  ServiceDesc svc;
};

int DragonServiceBroadcaster::Initialize() {
  // No initialization!
  return 0;
}

void DragonServiceBroadcaster::Shutdown() {
  // Remove all service files.
  for (auto it = services_.begin(); it != services_.end(); ++it) {
    InternalDragonService* service_data = (InternalDragonService*)(*it);
    unlink(service_data->service_filename);

    delete service_data;
    services_.erase(it);
    break;
  }
}

uintptr_t DragonServiceBroadcaster::AddService(const ServiceDesc& service) {
  const char* filename = "/etc/avahi/services/dragonsb.service";
  FILE* file = fopen(filename, "w");
  if (!file) {
    PERROR("Failed to open %s\n", filename);
    return 0;
  }

  fprintf(file,
          "<?xml version=\"1.0\" standalone='no'?><!--*-nxml-*-->\n"
          "<!DOCTYPE service-group SYSTEM \"avahi-service.dtd\">\n"
          "<service-group>\n"
          "\t<name replace-wildcards=\"yes\">%s</name>\n"
          "\t<service>\n"
          "\t\t<type>%s</type>\n"
          "\t\t<port>%d</port>\n",
          (const char*)service.name, (const char*)service.type, service.port);

  if (service.data) {
    fprintf(file, "\t\t<txt-record>%s</txt-record>\n",
            (const char*)service.data);
  }

  fprintf(file,
          "\t</service>\n"
          "</service-group>\n");

  fclose(file);

  InternalDragonService* service_data = new InternalDragonService;
  std::memset(service_data, 0, sizeof(InternalDragonService));
  std::memcpy(&service_data->svc, &service, sizeof(ServiceDesc));
  service_data->sb = this;
  service_data->service_filename = filename;

  // TODO: Copy the strings into service_data.
  services_.push_back((uintptr_t)service_data);
  return (uintptr_t)service_data;
}

int DragonServiceBroadcaster::RemoveService(uintptr_t id) {
  for (auto it = services_.begin(); it != services_.end(); ++it) {
    if (*it == id) {
      InternalDragonService* service_data = (InternalDragonService*)id;
      unlink(service_data->service_filename);

      delete service_data;
      services_.erase(it);
      break;
    }
  }

  return -1;
}