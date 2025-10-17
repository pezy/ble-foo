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
  argparse::ArgumentParser parser("ble_conn", "1.0");

  // Add description and epilog
  parser.add_description("Connect to or disconnect from a paired Bluetooth device");
  parser.add_epilog(
      "Usage:\n"
      "  ble_conn <MAC_ADDRESS>             # Connect to specified device\n"
      "  ble_conn <MAC_ADDRESS> -d          # Disconnect from specified device\n\n"
      "Examples:\n"
      "  ble_conn AA:BB:CC:DD:EE:FF         # Connect to device\n"
      "  ble_conn AA:BB:CC:DD:EE:FF -d      # Disconnect from device");

  // Add arguments
  parser.add_argument("mac_address").help("MAC address of device to connect to or disconnect from");

  parser.add_argument("-d", "--disconnect").flag().help("Disconnect from device instead of connecting");

  // Parse arguments
  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << parser;
    return 1;
  }

  // Get parsed values
  std::string mac_address = parser.get<std::string>("mac_address");
  bool disconnect = parser.get<bool>("-d");

  try {
    if (disconnect) {
      // Disconnect device mode
      std::cout << "Attempting to disconnect from device: " << mac_address << "\n";

      ble::Result result = ble::DisconnectDevice(mac_address);

      if (result.hasError()) {
        PrintErrorMessage(result.error_message);
        if (result.error_code != 0) {
          ble::ErrorCode error_code = static_cast<ble::ErrorCode>(result.error_code);
          PrintErrorMessage(ble::ErrorCodeToMessage(error_code));
        }
        return result.error_code;
      }

      std::cout << "Successfully disconnected from device: " << mac_address << "\n";
      std::cout << "Disconnect time: " << result.operation_time.count() << " ms\n";
    } else {
      // Connect device mode
      std::cout << "Attempting to connect to device: " << mac_address << "\n";

      ble::Result result = ble::ConnectDevice(mac_address);

      if (result.hasError()) {
        PrintErrorMessage(result.error_message);
        if (result.error_code != 0) {
          ble::ErrorCode error_code = static_cast<ble::ErrorCode>(result.error_code);
          PrintErrorMessage(ble::ErrorCodeToMessage(error_code));
        }
        return result.error_code;
      }

      std::cout << "Successfully connected to device: " << mac_address << "\n";
      std::cout << "Connection time: " << result.operation_time.count() << " ms\n";
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
