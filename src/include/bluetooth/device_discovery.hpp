// MIT License
// Copyright (c) 2025 pezy

#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ble {

// Forward declaration for internal implementation
class BluetoothConnection;

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
