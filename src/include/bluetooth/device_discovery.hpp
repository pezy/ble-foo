// MIT License
// Copyright (c) 2025 pezy

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Forward declarations for GLib types
typedef struct _GDBusConnection GDBusConnection;
typedef struct _GDBusProxy GDBusProxy;
typedef struct _GVariant GVariant;
typedef struct _GVariantIter GVariantIter;
typedef struct _GError GError;

namespace ble {

// RAII wrappers for GLib objects
class GObjectWrapper {
 public:
  // GDBusConnection wrapper
  using DBusConnection = std::unique_ptr<GDBusConnection, std::function<void(GDBusConnection*)>>;
  static DBusConnection make_dbus_connection(GDBusConnection* conn);

  // GDBusProxy wrapper
  using DBusProxy = std::unique_ptr<GDBusProxy, std::function<void(GDBusProxy*)>>;
  static DBusProxy make_dbus_proxy(GDBusProxy* proxy);

  // GVariant wrapper
  using Variant = std::unique_ptr<GVariant, std::function<void(GVariant*)>>;
  static Variant make_variant(GVariant* variant);

  // GVariantIter wrapper
  using VariantIter = std::unique_ptr<GVariantIter, std::function<void(GVariantIter*)>>;
  static VariantIter make_variant_iter(GVariantIter* iter);

  // GError wrapper
  using Error = std::unique_ptr<GError, std::function<void(GError*)>>;
  static Error make_error(GError* error);
};

// Error codes
enum class ErrorCode {
  Success = 0,
  BluetoothServiceUnavailable = 1,
  PermissionDenied = 2,
  QueryTimeout = 3,
  DBusConnectionFailed = 4,
  UnknownError = 5
};

// Exception class
class BluetoothException : public std::runtime_error {
 public:
  BluetoothException(ErrorCode code, const std::string& message);
  ErrorCode errorCode() const;

 private:
  ErrorCode error_code_;
};

// Device information structure
struct PairedBluetoothDevice {
  std::string mac_address;
  std::optional<std::string> device_name;
  std::optional<uint32_t> device_class;
  std::optional<int16_t> rssi;
  bool connected;

  // Validation method
  bool isValidMacAddress() const;
};

// Query result structure
struct DeviceQueryResult {
  std::vector<PairedBluetoothDevice> devices;
  bool success;
  int error_code;
  std::string error_message;
  std::chrono::milliseconds query_time;

  // Convenience methods
  bool hasError() const;
  size_t deviceCount() const;
};

// Core interface functions
DeviceQueryResult get_paired_devices();

}  // namespace ble
