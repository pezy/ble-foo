// MIT License
// Copyright (c) 2025 pezy

#include <gio/gio.h>
#include <glib.h>

#include <algorithm>
#include <bluetooth/device_discovery.hpp>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ble {

// BluetoothException implementation
BluetoothException::BluetoothException(ErrorCode code, const std::string& message)
    : std::runtime_error(message), error_code_(code) {}

ErrorCode BluetoothException::errorCode() const { return error_code_; }

// PairedBluetoothDevice implementation
bool PairedBluetoothDevice::isValidMacAddress() const {
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

// DeviceQueryResult implementation
bool DeviceQueryResult::hasError() const { return !success || error_code != 0; }

size_t DeviceQueryResult::deviceCount() const { return devices.size(); }

namespace {

// Helper function to convert GVariant to MAC address string
std::string extract_mac_address(GVariant* device_dict) {
  GVariant* address_variant = g_variant_lookup_value(device_dict, "Address", G_VARIANT_TYPE_STRING);
  if (!address_variant) {
    return "";
  }

  const char* address = g_variant_get_string(address_variant, nullptr);
  std::string result(address ? address : "");

  g_variant_unref(address_variant);
  return result;
}

// Helper function to extract device name
std::optional<std::string> extract_device_name(GVariant* device_dict) {
  GVariant* name_variant = g_variant_lookup_value(device_dict, "Name", G_VARIANT_TYPE_STRING);
  if (!name_variant) {
    return std::nullopt;
  }

  const char* name = g_variant_get_string(name_variant, nullptr);
  std::string result(name ? name : "");

  g_variant_unref(name_variant);
  return result;
}

// Helper function to extract device class
std::optional<uint32_t> extract_device_class(GVariant* device_dict) {
  GVariant* class_variant = g_variant_lookup_value(device_dict, "Class", G_VARIANT_TYPE_UINT32);
  if (!class_variant) {
    return std::nullopt;
  }

  uint32_t device_class = g_variant_get_uint32(class_variant);
  g_variant_unref(class_variant);
  return device_class;
}

// Helper function to extract RSSI
std::optional<int16_t> extract_rssi(GVariant* device_dict) {
  GVariant* rssi_variant = g_variant_lookup_value(device_dict, "RSSI", G_VARIANT_TYPE_INT16);
  if (!rssi_variant) {
    return std::nullopt;
  }

  int16_t rssi = g_variant_get_int16(rssi_variant);
  g_variant_unref(rssi_variant);
  return rssi;
}

// Helper function to check if device is connected
bool extract_connected(GVariant* device_dict) {
  GVariant* connected_variant = g_variant_lookup_value(device_dict, "Connected", G_VARIANT_TYPE_BOOLEAN);
  if (!connected_variant) {
    return false;
  }

  gboolean connected = g_variant_get_boolean(connected_variant);
  g_variant_unref(connected_variant);
  return static_cast<bool>(connected);
}

bool extract_paired(GVariant* device_dict) {
  GVariant* paired_variant = g_variant_lookup_value(device_dict, "Paired", G_VARIANT_TYPE_BOOLEAN);
  if (!paired_variant) {
    return false;
  }

  gboolean paired = g_variant_get_boolean(paired_variant);
  g_variant_unref(paired_variant);
  return static_cast<bool>(paired);
}

// Helper function to create error result
DeviceQueryResult create_error_result(ErrorCode error_code, const std::string& error_message) {
  DeviceQueryResult result;
  result.devices.clear();
  result.success = false;
  result.error_code = static_cast<int>(error_code);
  result.error_message = error_message;
  result.query_time = std::chrono::milliseconds(0);
  return result;
}

}  // anonymous namespace

// Core implementation functions
DeviceQueryResult get_paired_devices() {
  auto start_time = std::chrono::steady_clock::now();

  DeviceQueryResult result;
  result.devices.clear();
  result.success = false;
  result.error_code = 0;
  result.error_message = "";

  GError* error = nullptr;

  // Connect to system D-Bus
  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    result =
        create_error_result(ErrorCode::DBusConnectionFailed, error ? error->message : "Failed to connect to D-Bus");
    if (error) g_error_free(error);
    auto end_time = std::chrono::steady_clock::now();
    result.query_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return result;
  }

  // Create proxy for BlueZ object manager
  GDBusProxy* object_manager_proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.bluez",
                                                           "/", "org.freedesktop.DBus.ObjectManager", nullptr, &error);

  if (!object_manager_proxy) {
    result = create_error_result(ErrorCode::BluetoothServiceUnavailable,
                                 error ? error->message : "Failed to create BlueZ adapter proxy");
    if (error) g_error_free(error);
    g_object_unref(connection);
    auto end_time = std::chrono::steady_clock::now();
    result.query_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return result;
  }

  // Get managed objects (devices)
  GVariant* objects_result =
      g_dbus_proxy_call_sync(object_manager_proxy, "GetManagedObjects", nullptr, G_DBUS_CALL_FLAGS_NONE,
                             -1,  // Default timeout
                             nullptr, &error);

  if (!objects_result) {
    result = create_error_result(ErrorCode::BluetoothServiceUnavailable,
                                 error ? error->message : "Failed to get managed objects");
    if (error) g_error_free(error);
    g_object_unref(object_manager_proxy);
    g_object_unref(connection);
    auto end_time = std::chrono::steady_clock::now();
    result.query_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return result;
  }

  // Parse the result and extract paired devices
  GVariantIter* objects_iter;
  g_variant_get(objects_result, "(a{oa{sa{sv}}})", &objects_iter);

  const char* object_path;
  GVariant* interfaces_variant;

  while (g_variant_iter_loop(objects_iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_variant)) {
    GVariantIter interfaces_iter;
    g_variant_iter_init(&interfaces_iter, interfaces_variant);

    const char* interface_name;
    GVariant* properties_variant;

    while (g_variant_iter_loop(&interfaces_iter, "{&s@a{sv}}", &interface_name, &properties_variant)) {
      if (g_strcmp0(interface_name, "org.bluez.Device1") == 0) {
        // Extract MAC address and create device entry
        std::string mac_address = extract_mac_address(properties_variant);
        if (!mac_address.empty()) {
          PairedBluetoothDevice device;
          device.mac_address = mac_address;
          device.device_name = extract_device_name(properties_variant);
          device.device_class = extract_device_class(properties_variant);
          device.rssi = extract_rssi(properties_variant);
          device.connected = extract_connected(properties_variant);
          bool is_paired = extract_paired(properties_variant);

          if (device.isValidMacAddress() && is_paired) {
            result.devices.push_back(device);
          }
        }
      }

      // properties_variant is automatically unreferenced by g_variant_iter_loop
    }

    // interfaces_variant is automatically unreferenced by g_variant_iter_loop
  }

  g_variant_iter_free(objects_iter);
  g_variant_unref(objects_result);
  g_object_unref(object_manager_proxy);
  g_object_unref(connection);

  // Set success and timing
  result.success = true;
  auto end_time = std::chrono::steady_clock::now();
  result.query_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

}  // namespace ble
