// MIT License

#include <glib-unix.h>
#include <glib.h>
#include <unistd.h>

#include <array>
#include <bluetooth/advertising.hpp>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>

#include "argparse.hpp"

namespace {

void PrintError(const std::string& message) { std::cerr << "Error: " << message << "\n"; }

std::string GenerateUuidV4() {
  std::array<unsigned char, 16> data{};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 255);

  for (auto& byte : data) {
    byte = static_cast<unsigned char>(dist(gen));
  }

  data[6] = static_cast<unsigned char>((data[6] & 0x0F) | 0x40);
  data[8] = static_cast<unsigned char>((data[8] & 0x3F) | 0x80);

  char buffer[37];
  std::snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", data[0],
                data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11],
                data[12], data[13], data[14], data[15]);
  return std::string(buffer);
}

std::string GetDefaultName() {
  char buffer[256] = {0};
  if (gethostname(buffer, sizeof(buffer) - 1) == 0) {
    return std::string(buffer);
  }
  return "ble-foo";
}

struct LoopState {
  GMainLoop* loop{nullptr};
  bool stopped{false};
};

gboolean HandleUnixSignal(gpointer user_data) {
  auto* state = static_cast<LoopState*>(user_data);
  if (!state) {
    return G_SOURCE_REMOVE;
  }

  if (!state->stopped) {
    std::cout << "\nReceived exit signal, stopping advertisement...\n";
    auto stop_result = ble::StopAdvertisement();
    if (stop_result.hasError()) {
      PrintError(stop_result.error_message);
      if (stop_result.error_code != 0) {
        PrintError(ble::ErrorCodeToMessage(static_cast<ble::ErrorCode>(stop_result.error_code)));
      }
    } else {
      std::cout << "Advertisement stopped.\n";
    }
    state->stopped = true;
  }

  if (state->loop) {
    g_main_loop_quit(state->loop);
  }

  return G_SOURCE_REMOVE;
}

}  // namespace

int main(int argc, char* argv[]) {
  argparse::ArgumentParser parser("ble_ad", "1.0");
  parser.add_description("Start a BLE advertisement to allow phones to scan and initiate pairing");
  parser.add_epilog(
      "Examples:\n"
      "  ble_ad --name LivingRoomBeacon\n"
      "  ble_ad\n");

  parser.add_argument("--name", "-n").help("Set the advertisement name, if not provided, use hostname");

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& e) {
    PrintError(e.what());
    std::cerr << parser;
    return 1;
  }

  std::string local_name;
  if (parser.is_used("--name")) {
    local_name = parser.get<std::string>("--name");
  } else {
    local_name = GetDefaultName();
  }

  const auto uuid = GenerateUuidV4();
  std::cout << "Starting BLE advertisement...\n";
  std::cout << "Device name: " << local_name << "\n";
  std::cout << "Service UUID: " << uuid << "\n";

  ble::AdvertisementOptions options{.local_name = local_name, .service_uuid = uuid};
  ble::Result start_result = ble::StartAdvertisement(options);

  if (start_result.hasError()) {
    PrintError(start_result.error_message);
    if (start_result.error_code != 0) {
      PrintError(ble::ErrorCodeToMessage(static_cast<ble::ErrorCode>(start_result.error_code)));
    }
    return start_result.error_code == 0 ? 1 : start_result.error_code;
  }

  LoopState state;
  state.loop = g_main_loop_new(nullptr, FALSE);
  if (!state.loop) {
    PrintError("Failed to create GLib main loop");
    ble::StopAdvertisement();
    return 1;
  }

  std::cout << "Press Ctrl+C to exit.\n";

  guint sigint_id = g_unix_signal_add(SIGINT, HandleUnixSignal, &state);
  guint sigterm_id = g_unix_signal_add(SIGTERM, HandleUnixSignal, &state);

  g_main_loop_run(state.loop);

  if (!state.stopped) {
    auto stop_result = ble::StopAdvertisement();
    if (stop_result.hasError()) {
      PrintError(stop_result.error_message);
      if (stop_result.error_code != 0) {
        PrintError(ble::ErrorCodeToMessage(static_cast<ble::ErrorCode>(stop_result.error_code)));
      }
    }
  }

  if (sigint_id != 0) {
    g_source_remove(sigint_id);
  }
  if (sigterm_id != 0) {
    g_source_remove(sigterm_id);
  }

  if (state.loop) {
    g_main_loop_unref(state.loop);
    state.loop = nullptr;
  }

  return 0;
}
