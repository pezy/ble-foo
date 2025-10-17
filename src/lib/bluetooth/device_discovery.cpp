// MIT License
// Copyright (c) 2025 pezy

#include <bluetooth/device_discovery.hpp>

// std
#include <algorithm>
#include <cstring>
#include <memory>

// sys
#include <gio/gio.h>
#include <glib.h>

namespace {

typedef struct _GDBusConnection GDBusConnection;
typedef struct _GDBusProxy GDBusProxy;
typedef struct _GVariant GVariant;
typedef struct _GVariantIter GVariantIter;
typedef struct _GError GError;

// RAII wrappers for GLib objects
class GObjectWrapper {
 public:
  // GDBusConnection wrapper
  using DBusConnection = std::unique_ptr<GDBusConnection, std::function<void(GDBusConnection*)>>;
  static DBusConnection make_dbus_connection(GDBusConnection* conn) {
    return DBusConnection(conn, [](GDBusConnection* c) {
      if (c) g_object_unref(c);
    });
  }

  // GDBusProxy wrapper
  using DBusProxy = std::unique_ptr<GDBusProxy, std::function<void(GDBusProxy*)>>;
  static DBusProxy make_dbus_proxy(GDBusProxy* proxy) {
    return DBusProxy(proxy, [](GDBusProxy* p) {
      if (p) g_object_unref(p);
    });
  }

  // GVariant wrapper
  using Variant = std::unique_ptr<GVariant, std::function<void(GVariant*)>>;
  static Variant make_variant(GVariant* variant) {
    return Variant(variant, [](GVariant* v) {
      if (v) g_variant_unref(v);
    });
  }

  // GVariantIter wrapper
  using VariantIter = std::unique_ptr<GVariantIter, std::function<void(GVariantIter*)>>;
  static VariantIter make_variant_iter(GVariantIter* iter) {
    return VariantIter(iter, [](GVariantIter* i) {
      if (i) g_variant_iter_free(i);
    });
  }

  // GError wrapper
  using Error = std::unique_ptr<GError, std::function<void(GError*)>>;
  static Error make_error(GError* error) {
    return Error(error, [](GError* e) {
      if (e) g_error_free(e);
    });
  }
};

// Helper function to convert GVariant to MAC address string
std::string ExtractMacAddress(GVariant* device_dict) {
  auto address_variant =
      GObjectWrapper::make_variant(g_variant_lookup_value(device_dict, "Address", G_VARIANT_TYPE_STRING));
  if (!address_variant) {
    return "";
  }

  const char* address = g_variant_get_string(address_variant.get(), nullptr);
  return std::string(address ? address : "");
}

// Helper function to extract device name
std::optional<std::string> ExtractDeviceName(GVariant* device_dict) {
  auto name_variant = GObjectWrapper::make_variant(g_variant_lookup_value(device_dict, "Name", G_VARIANT_TYPE_STRING));
  if (!name_variant) {
    return std::nullopt;
  }

  const char* name = g_variant_get_string(name_variant.get(), nullptr);
  return std::string(name ? name : "");
}

// Helper function to extract device class
std::optional<uint32_t> ExtractDeviceClass(GVariant* device_dict) {
  auto class_variant =
      GObjectWrapper::make_variant(g_variant_lookup_value(device_dict, "Class", G_VARIANT_TYPE_UINT32));
  if (!class_variant) {
    return std::nullopt;
  }

  return g_variant_get_uint32(class_variant.get());
}

// Helper function to extract RSSI
std::optional<int16_t> ExtractRssi(GVariant* device_dict) {
  auto rssi_variant = GObjectWrapper::make_variant(g_variant_lookup_value(device_dict, "RSSI", G_VARIANT_TYPE_INT16));
  if (!rssi_variant) {
    return std::nullopt;
  }

  return g_variant_get_int16(rssi_variant.get());
}

// Helper function to check if device is connected
bool ExtractConnected(GVariant* device_dict) {
  auto connected_variant =
      GObjectWrapper::make_variant(g_variant_lookup_value(device_dict, "Connected", G_VARIANT_TYPE_BOOLEAN));
  if (!connected_variant) {
    return false;
  }

  return static_cast<bool>(g_variant_get_boolean(connected_variant.get()));
}

bool ExtractPaired(GVariant* device_dict) {
  auto paired_variant =
      GObjectWrapper::make_variant(g_variant_lookup_value(device_dict, "Paired", G_VARIANT_TYPE_BOOLEAN));
  if (!paired_variant) {
    return false;
  }

  return static_cast<bool>(g_variant_get_boolean(paired_variant.get()));
}

// Helper function to create error result
ble::DeviceQueryResult CreateErrorResult(ble::ErrorCode error_code, const std::string& error_message) {
  ble::DeviceQueryResult result;
  result.devices.clear();
  result.success = false;
  result.error_code = static_cast<int>(error_code);
  result.error_message = error_message;
  result.query_time = std::chrono::milliseconds(0);
  return result;
}

// Helper function to create pair error result
ble::Result MakeErrorResult(ble::ErrorCode error_code, const std::string& error_message) {
  ble::Result result;
  result.success = false;
  result.error_code = static_cast<int>(error_code);
  result.error_message = error_message;
  result.operation_time = std::chrono::milliseconds(0);
  return result;
}

// Helper function to convert MAC address to D-Bus object path
std::string MacToObjectPath(const std::string& mac_address) {
  std::string path = "/org/bluez/hci0/dev_";
  for (char c : mac_address) {
    if (c == ':') {
      path += '_';
    } else {
      path += std::toupper(c);
    }
  }
  return path;
}

bool IsValidMacAddress(const std::string& mac_address) {
  if (mac_address.empty()) {
    return false;
  }

  // MAC address format: XX:XX:XX:XX:XX:XX (6 octets, 5 colons)
  if (mac_address.length() != 17) {
    return false;
  }

  // Check for colons at correct positions
  if (mac_address[2] != ':' || mac_address[5] != ':' || mac_address[8] != ':' || mac_address[11] != ':' ||
      mac_address[14] != ':') {
    return false;
  }

  // Check each octet is valid hexadecimal
  for (int i = 0; i < 17; ++i) {
    if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
      continue;  // Skip colons
    }
    char c = mac_address[i];
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }

  return true;
}

class ScopedTimer {
 public:
  explicit ScopedTimer(std::chrono::milliseconds& duration)
      : start_time_(std::chrono::steady_clock::now()), duration_ref_(duration) {}

  ~ScopedTimer() {
    const auto end_time = std::chrono::steady_clock::now();
    duration_ref_ = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
  }

 private:
  const std::chrono::steady_clock::time_point start_time_;
  std::chrono::milliseconds& duration_ref_;
};

}  // anonymous namespace

namespace ble {

DeviceQueryResult GetPairedDevices() {
  DeviceQueryResult result;
  ScopedTimer timer(result.query_time);

  GError* error = nullptr;

  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    result = CreateErrorResult(ErrorCode::DBusConnectionFailed, error ? error->message : "Failed to connect to D-Bus");
    if (error) {
      g_error_free(error);
    }
    return result;
  }

  auto connection_wrapper = GObjectWrapper::make_dbus_connection(connection);

  GDBusProxy* object_manager_proxy =
      g_dbus_proxy_new_sync(connection_wrapper.get(), G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.bluez", "/",
                            "org.freedesktop.DBus.ObjectManager", nullptr, &error);

  if (!object_manager_proxy) {
    result = CreateErrorResult(ErrorCode::BluetoothServiceUnavailable,
                               error ? error->message : "Failed to create BlueZ adapter proxy");
    if (error) g_error_free(error);
    return result;
  }

  // RAII wrapper for proxy
  auto proxy_wrapper = GObjectWrapper::make_dbus_proxy(object_manager_proxy);

  // Get managed objects (devices)
  GVariant* objects_result =
      g_dbus_proxy_call_sync(proxy_wrapper.get(), "GetManagedObjects", nullptr, G_DBUS_CALL_FLAGS_NONE,
                             -1,  // Default timeout
                             nullptr, &error);

  if (!objects_result) {
    result = CreateErrorResult(ErrorCode::BluetoothServiceUnavailable,
                               error ? error->message : "Failed to get managed objects");
    if (error) g_error_free(error);
    return result;
  }

  // RAII wrapper for objects result
  auto objects_result_wrapper = GObjectWrapper::make_variant(objects_result);

  // Parse the result and extract paired devices
  GVariantIter* objects_iter;
  g_variant_get(objects_result_wrapper.get(), "(a{oa{sa{sv}}})", &objects_iter);

  // RAII wrapper for objects iterator
  auto objects_iter_wrapper = GObjectWrapper::make_variant_iter(objects_iter);

  const char* object_path;
  GVariant* interfaces_variant;  // automatically unreferenced by g_variant_iter_loop

  while (g_variant_iter_loop(objects_iter_wrapper.get(), "{&o@a{sa{sv}}}", &object_path, &interfaces_variant)) {
    GVariantIter interfaces_iter;
    g_variant_iter_init(&interfaces_iter, interfaces_variant);

    const char* interface_name;
    GVariant* properties_variant;  // automatically unreferenced by g_variant_iter_loop

    while (g_variant_iter_loop(&interfaces_iter, "{&s@a{sv}}", &interface_name, &properties_variant)) {
      if (g_strcmp0(interface_name, "org.bluez.Device1") == 0) {
        bool is_paired = ExtractPaired(properties_variant);
        if (!is_paired) {
          continue;
        }

        // Extract MAC address and create device entry
        std::string mac_address = ExtractMacAddress(properties_variant);
        if (mac_address.empty()) {
          continue;
        }

        BluetoothDevice device;
        device.mac_address = mac_address;
        device.device_name = ExtractDeviceName(properties_variant);
        device.device_class = ExtractDeviceClass(properties_variant);
        device.rssi = ExtractRssi(properties_variant);
        device.connected = ExtractConnected(properties_variant);
        result.devices.push_back(device);
      }
    }
  }

  // Set success and timing
  result.success = true;

  return result;
}

bool IsDevicePaired(const std::string& mac_address) {
  auto result = GetPairedDevices();
  if (result.hasError()) {
    return false;
  }
  return std::find_if(result.devices.begin(), result.devices.end(), [mac_address](const BluetoothDevice& device) {
           return device.mac_address == mac_address;
         }) != result.devices.end();
}

Result PairDevice(const std::string& mac_address, int timeout_seconds) {
  if (IsDevicePaired(mac_address)) {
    return MakeErrorResult(ErrorCode::Success, "Device already paired");
  }

  Result result;
  ScopedTimer timer(result.operation_time);

  if (!IsValidMacAddress(mac_address)) {
    result = MakeErrorResult(ErrorCode::DeviceNotFound, "Invalid MAC address format");
    return result;
  }

  GError* error = nullptr;

  // Connect to system D-Bus
  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    result = MakeErrorResult(ErrorCode::DBusConnectionFailed, error ? error->message : "Failed to connect to D-Bus");
    if (error) {
      g_error_free(error);
    }
    return result;
  }

  // RAII wrapper for connection
  auto connection_wrapper = GObjectWrapper::make_dbus_connection(connection);

  // Convert MAC address to D-Bus object path
  std::string device_path = MacToObjectPath(mac_address);

  // Create proxy for the specific device
  GDBusProxy* device_proxy =
      g_dbus_proxy_new_sync(connection_wrapper.get(), G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.bluez",
                            device_path.c_str(), "org.bluez.Device1", nullptr, &error);

  if (!device_proxy) {
    result = MakeErrorResult(ErrorCode::DeviceNotFound, error ? error->message : "Device not found or not accessible");
    if (error) g_error_free(error);
    return result;
  }

  // RAII wrapper for device proxy
  auto device_proxy_wrapper = GObjectWrapper::make_dbus_proxy(device_proxy);

  // Call Pair method
  GVariant* pair_result =
      g_dbus_proxy_call_sync(device_proxy_wrapper.get(), "Pair", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE,
                             timeout_seconds * 1000,  // Convert to milliseconds
                             nullptr, &error);

  if (!pair_result) {
    if (error && g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_TIMEOUT)) {
      result = MakeErrorResult(ErrorCode::PairingTimeout, "Pairing operation timed out");
    } else {
      result = MakeErrorResult(ErrorCode::PairingFailed, error ? error->message : "Pairing operation failed");
    }
    if (error) g_error_free(error);
    return result;
  }

  // Clean up pair result
  g_variant_unref(pair_result);

  // Set success and timing
  result.success = true;

  return result;
}

// Error code utility function
std::string ErrorCodeToMessage(ErrorCode code) {
  switch (code) {
    case ErrorCode::Success:
      return "Success";
    case ErrorCode::BluetoothServiceUnavailable:
      return "Bluetooth service unavailable - Ensure Bluetooth is running and BlueZ service is active";
    case ErrorCode::PermissionDenied:
      return "Permission denied - Ensure sufficient permissions to access Bluetooth devices";
    case ErrorCode::QueryTimeout:
      return "Query timeout";
    case ErrorCode::DBusConnectionFailed:
      return "D-Bus connection failed - Ensure system D-Bus service is running";
    case ErrorCode::UnknownError:
      return "Unknown error";
    case ErrorCode::PairingFailed:
      return "Pairing failed - Device may not be in pairing mode or pairing was rejected";
    case ErrorCode::DeviceNotFound:
      return "Device not found - Ensure device is discoverable and within range";
    case ErrorCode::PairingTimeout:
      return "Pairing timeout - Device did not respond within the timeout period";
    case ErrorCode::ConnectionFailed:
      return "Connection failed - Device may not be available or connection was rejected";
    case ErrorCode::DisconnectFailed:
      return "Disconnect failed - Unable to disconnect from device";
    case ErrorCode::ConnectionTimeout:
      return "Connection timeout - Device did not respond within the timeout period";
    default:
      return "Undefined error code";
  }
}

ble::Result ConnectDevice(const std::string& mac_address, int timeout_seconds) {
  ble::Result result;
  ScopedTimer timer(result.operation_time);

  if (!IsValidMacAddress(mac_address)) {
    result = MakeErrorResult(ble::ErrorCode::DeviceNotFound, "Invalid MAC address format");
    return result;
  }

  GError* error = nullptr;

  // Connect to system D-Bus
  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    result =
        MakeErrorResult(ble::ErrorCode::DBusConnectionFailed, error ? error->message : "Failed to connect to D-Bus");
    if (error) {
      g_error_free(error);
    }
    return result;
  }

  // RAII wrapper for connection
  auto connection_wrapper = GObjectWrapper::make_dbus_connection(connection);

  // Convert MAC address to D-Bus object path
  std::string device_path = MacToObjectPath(mac_address);

  // Create proxy for the specific device
  GDBusProxy* device_proxy =
      g_dbus_proxy_new_sync(connection_wrapper.get(), G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.bluez",
                            device_path.c_str(), "org.bluez.Device1", nullptr, &error);

  if (!device_proxy) {
    result =
        MakeErrorResult(ble::ErrorCode::DeviceNotFound, error ? error->message : "Device not found or not accessible");
    if (error) g_error_free(error);
    return result;
  }

  // RAII wrapper for device proxy
  auto device_proxy_wrapper = GObjectWrapper::make_dbus_proxy(device_proxy);

  // Call Connect method
  GVariant* connect_result =
      g_dbus_proxy_call_sync(device_proxy_wrapper.get(), "Connect", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE,
                             timeout_seconds * 1000,  // Convert to milliseconds
                             nullptr, &error);

  if (!connect_result) {
    if (error && g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_TIMEOUT)) {
      result = MakeErrorResult(ble::ErrorCode::ConnectionTimeout, "Connection operation timed out");
    } else {
      result =
          MakeErrorResult(ble::ErrorCode::ConnectionFailed, error ? error->message : "Connection operation failed");
    }
    if (error) g_error_free(error);
    return result;
  }

  // Clean up connect result
  g_variant_unref(connect_result);

  // Set success and timing
  result.success = true;

  return result;
}

ble::Result DisconnectDevice(const std::string& mac_address, int timeout_seconds) {
  ble::Result result;
  ScopedTimer timer(result.operation_time);

  // Validate MAC address format
  if (!IsValidMacAddress(mac_address)) {
    result = MakeErrorResult(ble::ErrorCode::DeviceNotFound, "Invalid MAC address format");
    return result;
  }

  GError* error = nullptr;

  // Connect to system D-Bus
  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    result =
        MakeErrorResult(ble::ErrorCode::DBusConnectionFailed, error ? error->message : "Failed to connect to D-Bus");
    if (error) {
      g_error_free(error);
    }
    return result;
  }

  // RAII wrapper for connection
  auto connection_wrapper = GObjectWrapper::make_dbus_connection(connection);

  // Convert MAC address to D-Bus object path
  std::string device_path = MacToObjectPath(mac_address);

  // Create proxy for the specific device
  GDBusProxy* device_proxy =
      g_dbus_proxy_new_sync(connection_wrapper.get(), G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.bluez",
                            device_path.c_str(), "org.bluez.Device1", nullptr, &error);

  if (!device_proxy) {
    result =
        MakeErrorResult(ble::ErrorCode::DeviceNotFound, error ? error->message : "Device not found or not accessible");
    if (error) g_error_free(error);
    return result;
  }

  // RAII wrapper for device proxy
  auto device_proxy_wrapper = GObjectWrapper::make_dbus_proxy(device_proxy);

  // Call Disconnect method
  GVariant* disconnect_result = g_dbus_proxy_call_sync(device_proxy_wrapper.get(), "Disconnect", g_variant_new("()"),
                                                       G_DBUS_CALL_FLAGS_NONE, timeout_seconds * 1000, nullptr, &error);

  if (!disconnect_result) {
    result = MakeErrorResult(ble::ErrorCode::DisconnectFailed, error ? error->message : "Disconnect operation failed");
    if (error) g_error_free(error);
    return result;
  }

  // Clean up disconnect result
  g_variant_unref(disconnect_result);

  // Set success and timing
  result.success = true;

  return result;
}

}  // namespace ble
