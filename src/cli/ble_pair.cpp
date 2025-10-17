// MIT License
// Copyright (c) 2025 pezy

#include <bluetooth/device_discovery.hpp>
#include <chrono>
#include <iostream>
#include <string>

#include "argparse.hpp"

namespace {

void PrintErrorMessage(const std::string& message) { std::cerr << "Error: " << message << "\n"; }

}  // anonymous namespace

int main(int argc, char* argv[]) {
  // Create argument parser
  argparse::ArgumentParser parser("ble_pair", "1.0");

  // Add description and epilog
  parser.add_description("List paired Bluetooth devices or pair with a specific device");
  parser.add_epilog(
      "Usage:\n"
      "  ble_pair                           # List all paired devices\n"
      "  ble_pair <MAC_ADDRESS>             # Pair with specified device\n\n"
      "Output format for listing:\n"
      "  MAC_ADDRESS DEVICE_NAME RSSI CONNECTION_STATUS\n\n"
      "Examples:\n"
      "  ble_pair                           # List paired devices\n"
      "  ble_pair AA:BB:CC:DD:EE:FF         # Pair with device");

  // Add arguments
  parser.add_argument("mac_address")
      .nargs(argparse::nargs_pattern::optional)
      .help("MAC address of device to pair with (optional)");

  // Parse arguments
  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << parser;
    return 1;
  }

  // Get parsed values
  std::string mac_address;
  if (parser.is_used("mac_address")) {
    mac_address = parser.get<std::string>("mac_address");
  } else {
    mac_address = "";
  }

  try {
    if (mac_address.empty()) {
      // List paired devices mode
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
        std::cout << "No paired Bluetooth devices found\n";
        return 0;
      }

      // Print results in the new format: MAC_ADDRESS DEVICE_NAME RSSI CONNECTION_STATUS
      for (const auto& device : result.devices) {
        std::cout << device.mac_address << " ";

        // Device name
        if (device.device_name && !device.device_name->empty()) {
          std::cout << *device.device_name << " ";
        } else {
          std::cout << "N/A ";
        }

        // RSSI
        if (device.rssi) {
          std::cout << *device.rssi << "dBm ";
        } else {
          std::cout << "N/A ";
        }

        std::cout << (device.paired ? "Paired" : "Unpaired") << " ";
        std::cout << (device.connected ? "Connected" : "Disconnected") << "\n";
      }
    } else {
      // Pair device mode
      std::cout << "Attempting to pair with device: " << mac_address << "\n";

      ble::Result result = ble::PairDevice(mac_address);

      if (result.hasError()) {
        PrintErrorMessage(result.error_message);
        if (result.error_code != 0) {
          ble::ErrorCode error_code = static_cast<ble::ErrorCode>(result.error_code);
          PrintErrorMessage(ble::ErrorCodeToMessage(error_code));
        }
        return result.error_code;
      }

      std::cout << "Successfully paired with device: " << mac_address << "\n";
      std::cout << "Pairing time: " << result.operation_time.count() << " ms\n";
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
