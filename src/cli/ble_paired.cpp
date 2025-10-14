// MIT License
// Copyright (c) 2025 pezy

#include <bluetooth/device_discovery.hpp>
#include <chrono>
#include <iostream>
#include <string>

#include "argparse.hpp"

namespace {

void PrintDeviceInfo(const ble::PairedBluetoothDevice& device, bool verbose) {
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

void PrintErrorMessage(const std::string& message) { std::cerr << "Error: " << message << "\n"; }

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
    ble::DeviceQueryResult result = ble::GetPairedDevices();

    if (result.hasError()) {
      PrintErrorMessage(result.error_message);
      if (result.error_code != 0) {
        ble::ErrorCode error_code = static_cast<ble::ErrorCode>(result.error_code);
        PrintErrorMessage(ble::ErrorCodeToMessage(error_code));
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
      PrintDeviceInfo(device, verbose);
    }
  } catch (const ble::BluetoothException& e) {
    PrintErrorMessage(e.what());
    PrintErrorMessage(ble::ErrorCodeToMessage(e.errorCode()));
    return static_cast<int>(e.errorCode());
  } catch (const std::exception& e) {
    PrintErrorMessage("Program exception: " + std::string(e.what()));
    return 1;
  }

  return 0;
}
