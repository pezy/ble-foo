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
    const bool is_disconnect = disconnect;
    const auto action_str = is_disconnect ? "disconnect from" : "connect to";
    const auto success_str = is_disconnect ? "Successfully disconnected from" : "Successfully connected to";
    const auto time_label = is_disconnect ? "Disconnect time" : "Connection time";

    std::cout << "Attempting to " << action_str << " device: " << mac_address << "\n";
    ble::Result result = is_disconnect ? ble::DisconnectDevice(mac_address) : ble::ConnectDevice(mac_address);

    if (result.hasError()) {
      PrintErrorMessage(result.error_message);
      if (result.error_code != 0) {
        ble::ErrorCode error_code = static_cast<ble::ErrorCode>(result.error_code);
        PrintErrorMessage(ble::ErrorCodeToMessage(error_code));
      }
      return result.error_code;
    }

    std::cout << success_str << " device: " << mac_address << "\n";
    std::cout << time_label << ": " << result.operation_time.count() << " ms\n";
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
