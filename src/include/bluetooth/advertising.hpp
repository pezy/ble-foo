// MIT

#pragma once

#include <bluetooth/device_discovery.hpp>
#include <string>

namespace ble {

struct AdvertisementOptions {
  std::string local_name;
  std::string service_uuid;
};

Result StartAdvertisement(const AdvertisementOptions& options);

Result StopAdvertisement();

}  // namespace ble
