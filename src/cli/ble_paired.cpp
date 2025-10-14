// MIT License
// Copyright (c) 2025 pezy

#include <bluetooth/device_discovery.hpp>
#include <chrono>
#include <iostream>
#include <string>

#include "argparse.hpp"

namespace {

void print_device_info(const ble::PairedBluetoothDevice& device, bool verbose) {
  if (verbose) {
    std::cout << device.mac_address;

    if (device.device_name && !device.device_name->empty()) {
      std::cout << " [" << *device.device_name << "]";
    }

    if (device.rssi) {
      std::cout << " (RSSI: " << *device.rssi << " dBm)";
    }

    std::cout << " [" << (device.connected ? "Connected" : "Disconnected") << "]\n";
  } else {
    std::cout << device.mac_address << "\n";
  }
}

void print_error_message(const std::string& message) { std::cerr << "Error: " << message << "\n"; }

std::string error_code_to_message(ble::ErrorCode code) {
  switch (code) {
    case ble::ErrorCode::Success:
      return "Success";
    case ble::ErrorCode::BluetoothServiceUnavailable:
      return "Bluetooth service unavailable - Ensure Bluetooth is running and BlueZ service is active";
    case ble::ErrorCode::PermissionDenied:
      return "Permission denied - Ensure sufficient permissions to access Bluetooth devices";
    case ble::ErrorCode::QueryTimeout:
      return "Query timeout";
    case ble::ErrorCode::DBusConnectionFailed:
      return "D-Bus connection failed - Ensure system D-Bus service is running";
    case ble::ErrorCode::UnknownError:
      return "Unknown error";
    default:
      return "Undefined error code";
  }
}

}  // anonymous namespace

int main(int argc, char* argv[]) {
  // Create argument parser
  argparse::ArgumentParser parser("ble_paired", "1.0");

  // Add description and epilog
  parser.add_description("List paired Bluetooth devices with optional detailed information");
  parser.add_epilog(
      "Output format:\n"
      "  Default: One MAC address per line\n"
      "  Verbose: MAC [device_name] (RSSI: dBm) [status]\n\n"
      "Examples:\n"
      "  ble_paired                           # Simple MAC address output\n"
      "  ble_paired -v                        # Detailed information");

  // Add arguments
  parser.add_argument("-v", "--verbose").flag().help("Show detailed information (device name, RSSI, etc.)");

  // Parse arguments
  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  // Get parsed values
  bool verbose = parser.get<bool>("--verbose");

  try {
    ble::DeviceQueryResult result = ble::get_paired_devices();

    if (result.hasError()) {
      print_error_message(result.error_message);
      if (result.error_code != 0) {
        ble::ErrorCode error_code = static_cast<ble::ErrorCode>(result.error_code);
        print_error_message(error_code_to_message(error_code));
      }
      return result.error_code;
    }

    if (result.deviceCount() == 0) {
      if (verbose) {
        std::cout << "No paired Bluetooth devices found\n";
      }
      return 0;
    }

    // Print results
    if (verbose) {
      std::cout << "Found " << result.deviceCount() << " paired Bluetooth devices:\n";
      std::cout << "Query time: " << result.query_time.count() << " ms\n\n";
    }

    for (const auto& device : result.devices) {
      print_device_info(device, verbose);
    }
  } catch (const ble::BluetoothException& e) {
    print_error_message(e.what());
    print_error_message(error_code_to_message(e.errorCode()));
    return static_cast<int>(e.errorCode());
  } catch (const std::exception& e) {
    print_error_message("Program exception: " + std::string(e.what()));
    return 1;
  }

  return 0;
}
