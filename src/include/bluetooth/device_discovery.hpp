// MIT License
// Copyright (c) 2025 pezy

#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ble {

// Error codes
enum class ErrorCode {
  Success = 0,
  BluetoothServiceUnavailable = 1,
  PermissionDenied = 2,
  QueryTimeout = 3,
  DBusConnectionFailed = 4,
  UnknownError = 5,
  PairingFailed = 6,
  DeviceNotFound = 7,
  PairingTimeout = 8,
  ConnectionFailed = 9,
  DisconnectFailed = 10,
  ConnectionTimeout = 11
};

// Exception class
class BluetoothException : public std::runtime_error {
 public:
  BluetoothException(ErrorCode code, const std::string& message) : std::runtime_error(message), error_code_(code) {}
  ErrorCode errorCode() const { return error_code_; }

 private:
  const ErrorCode error_code_;
};

// Device information structure
struct BluetoothDevice {
  std::string mac_address;
  std::optional<std::string> device_name;
  std::optional<uint32_t> device_class;
  std::optional<int16_t> rssi;
  bool connected;
};

// Query result structure
struct DeviceQueryResult {
  std::vector<BluetoothDevice> devices;
  bool success{false};
  int error_code{0};
  std::string error_message{""};
  std::chrono::milliseconds query_time{0};

  // Convenience methods
  bool hasError() const { return !success || error_code != 0; }
  size_t deviceCount() const { return devices.size(); }
};

struct Result {
  bool success{false};
  int error_code{0};
  std::string error_message{""};
  std::chrono::milliseconds operation_time{0};

  // Convenience methods
  bool hasError() const { return !success || error_code != 0; }
};

// Core interface functions
DeviceQueryResult GetPairedDevices();

bool IsDevicePaired(const std::string& mac_address);

Result PairDevice(const std::string& mac_address, int timeout_seconds = 30);

Result ConnectDevice(const std::string& mac_address, int timeout_seconds = 30);

Result DisconnectDevice(const std::string& mac_address, int timeout_seconds = 10);

// Error code utility function
std::string ErrorCodeToMessage(ErrorCode code);

}  // namespace ble
