// MIT

#include <bluetooth/advertising.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

// GIO
#include <gio/gio.h>

namespace {

struct ScopedTimer {
  explicit ScopedTimer(std::chrono::milliseconds& duration)
      : start_(std::chrono::steady_clock::now()), duration_ref_(duration) {}
  ~ScopedTimer() {
    const auto end = std::chrono::steady_clock::now();
    duration_ref_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
  }

 private:
  const std::chrono::steady_clock::time_point start_;
  std::chrono::milliseconds& duration_ref_;
};

struct GObjectWrapper {
  using DBusConnection = std::unique_ptr<GDBusConnection, std::function<void(GDBusConnection*)>>;
  using DBusProxy = std::unique_ptr<GDBusProxy, std::function<void(GDBusProxy*)>>;
  using Variant = std::unique_ptr<GVariant, std::function<void(GVariant*)>>;
  using NodeInfo = std::unique_ptr<GDBusNodeInfo, std::function<void(GDBusNodeInfo*)>>;

  static DBusConnection make_connection(GDBusConnection* connection) {
    return DBusConnection(connection, [](GDBusConnection* c) {
      if (c) g_object_unref(c);
    });
  }

  static DBusProxy make_proxy(GDBusProxy* proxy) {
    return DBusProxy(proxy, [](GDBusProxy* p) {
      if (p) g_object_unref(p);
    });
  }

  static Variant make_variant(GVariant* variant) {
    return Variant(variant, [](GVariant* v) {
      if (v) g_variant_unref(v);
    });
  }

  static NodeInfo make_node_info(GDBusNodeInfo* info) {
    return NodeInfo(info, [](GDBusNodeInfo* i) {
      if (i) g_dbus_node_info_unref(i);
    });
  }
};

constexpr const char* kBluezService = "org.bluez";
constexpr const char* kObjectManagerInterface = "org.freedesktop.DBus.ObjectManager";
constexpr const char* kPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr const char* kAdapterInterface = "org.bluez.Adapter1";
constexpr const char* kAdvertisingManagerInterface = "org.bluez.LEAdvertisingManager1";
constexpr const char* kAdvertisementInterface = "org.bluez.LEAdvertisement1";

const char kAdvertisementIntrospectionXml[] =
    "<node>"
    "  <interface name='org.bluez.LEAdvertisement1'>"
    "    <method name='Release'/>"
    "    <property name='Type' type='s' access='read'/>"
    "    <property name='ServiceUUIDs' type='as' access='read'/>"
    "    <property name='LocalName' type='s' access='read'/>"
    "  </interface>"
    "</node>";

struct AdvertisementContext {
  std::string object_path{"/org/ble/foo/advertisement0"};
  std::string service_uuid;
  std::string local_name;
  bool active{false};

  GDBusConnection* connection{nullptr};
  GDBusProxy* advertising_manager{nullptr};
  GDBusProxy* properties_proxy{nullptr};
  guint registration_id{0};
  GDBusNodeInfo* introspection{nullptr};
};

AdvertisementContext& Context() {
  static AdvertisementContext ctx;
  return ctx;
}

ble::Result MakeErrorResult(ble::ErrorCode code, const std::string& message) {
  ble::Result result;
  result.success = false;
  result.error_code = static_cast<int>(code);
  result.error_message = message;
  return result;
}

std::string TakeErrorMessage(GError** error) {
  if (!error || !*error) {
    return "Unknown error";
  }
  std::string message = (*error)->message ? (*error)->message : "Unknown error";
  g_error_free(*error);
  *error = nullptr;
  return message;
}

GDBusNodeInfo* EnsureIntrospection(AdvertisementContext& ctx, GError** error) {
  if (ctx.introspection) {
    return ctx.introspection;
  }
  ctx.introspection = g_dbus_node_info_new_for_xml(kAdvertisementIntrospectionXml, error);
  return ctx.introspection;
}

std::optional<std::string> GetDefaultAdapterPath(GDBusConnection* connection, GError** error) {
  GDBusProxy* object_manager = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, nullptr, kBluezService, "/",
                                                     kObjectManagerInterface, nullptr, error);
  if (!object_manager) {
    return std::nullopt;
  }

  auto object_manager_wrapper = GObjectWrapper::make_proxy(object_manager);

  GVariant* managed_objects = g_dbus_proxy_call_sync(object_manager_wrapper.get(), "GetManagedObjects", nullptr,
                                                     G_DBUS_CALL_FLAGS_NONE, -1, nullptr, error);
  if (!managed_objects) {
    return std::nullopt;
  }

  auto managed_objects_wrapper = GObjectWrapper::make_variant(managed_objects);

  GVariantIter* objects_iter = nullptr;
  g_variant_get(managed_objects_wrapper.get(), "(a{oa{sa{sv}}})", &objects_iter);
  if (!objects_iter) {
    return std::nullopt;
  }

  std::optional<std::string> adapter_path;
  GVariantIter* interfaces_iter = nullptr;
  const char* object_path = nullptr;
  GVariant* interfaces_variant = nullptr;

  while (g_variant_iter_loop(objects_iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_variant)) {
    g_variant_get(interfaces_variant, "a{sa{sv}}", &interfaces_iter);
    const char* interface_name = nullptr;
    GVariant* properties_variant = nullptr;
    while (g_variant_iter_loop(interfaces_iter, "{&s@a{sv}}", &interface_name, &properties_variant)) {
      if (g_strcmp0(interface_name, kAdapterInterface) == 0) {
        adapter_path = object_path;
        break;
      }
    }
    if (interfaces_iter) {
      g_variant_iter_free(interfaces_iter);
      interfaces_iter = nullptr;
    }
    if (adapter_path) {
      break;
    }
  }

  g_variant_iter_free(objects_iter);
  return adapter_path;
}

bool EnsureAdapterBooleanProperty(GDBusProxy* properties_proxy, const char* property, bool desired_value,
                                  GError** error) {
  GVariant* get_result =
      g_dbus_proxy_call_sync(properties_proxy, "Get", g_variant_new("(ss)", kAdapterInterface, property),
                             G_DBUS_CALL_FLAGS_NONE, -1, nullptr, error);
  if (!get_result) {
    return false;
  }

  auto get_result_wrapper = GObjectWrapper::make_variant(get_result);
  GVariant* value_variant = nullptr;
  g_variant_get(get_result_wrapper.get(), "(v)", &value_variant);
  gboolean current_value = g_variant_get_boolean(value_variant);
  g_variant_unref(value_variant);

  if (static_cast<bool>(current_value) == desired_value) {
    return true;
  }

  GVariant* set_params =
      g_variant_new("(ssv)", kAdapterInterface, property, g_variant_new_variant(g_variant_new_boolean(desired_value)));
  GVariant* set_result =
      g_dbus_proxy_call_sync(properties_proxy, "Set", set_params, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, error);
  if (!set_result) {
    return false;
  }
  g_variant_unref(set_result);
  return true;
}

GVariant* CreatePropertyValue(const AdvertisementContext& ctx, const gchar* property_name, GError** error) {
  if (g_strcmp0(property_name, "Type") == 0) {
    return g_variant_new_string("peripheral");
  }
  if (g_strcmp0(property_name, "ServiceUUIDs") == 0) {
    const char* uuids[] = {ctx.service_uuid.c_str()};
    return g_variant_new_strv(uuids, 1);
  }
  if (g_strcmp0(property_name, "LocalName") == 0) {
    return g_variant_new_string(ctx.local_name.c_str());
  }
  if (error) {
    *error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, "Unknown property");
  }
  return nullptr;
}

void AdvertisementMethodCall(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                             const gchar* interface_name, const gchar* method_name, GVariant* parameters,
                             GDBusMethodInvocation* invocation, gpointer user_data) {
  (void)connection;
  (void)sender;
  (void)object_path;
  (void)parameters;
  (void)user_data;

  if (g_strcmp0(interface_name, kAdvertisementInterface) == 0 && g_strcmp0(method_name, "Release") == 0) {
    g_dbus_method_invocation_return_value(invocation, nullptr);
    return;
  }

  g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.Failed", "Unsupported method");
}

GVariant* AdvertisementGetProperty(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                   const gchar* interface_name, const gchar* property_name, GError** error,
                                   gpointer user_data) {
  (void)connection;
  (void)sender;
  (void)object_path;
  (void)interface_name;

  auto* ctx = static_cast<AdvertisementContext*>(user_data);
  return CreatePropertyValue(*ctx, property_name, error);
}

const GDBusInterfaceVTable kAdvertisementVTable = {AdvertisementMethodCall, AdvertisementGetProperty, nullptr, {0}};

}  // namespace

namespace ble {

Result StartAdvertisement(const AdvertisementOptions& options) {
  Result result;
  ScopedTimer timer(result.operation_time);

  auto& ctx = Context();
  if (ctx.active) {
    return MakeErrorResult(ErrorCode::UnknownError, "Advertisement already active");
  }

  GError* error = nullptr;

  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::DBusConnectionFailed, message);
  }

  auto connection_wrapper = GObjectWrapper::make_connection(connection);

  auto adapter_path = GetDefaultAdapterPath(connection_wrapper.get(), &error);
  if (!adapter_path) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }

  GDBusProxy* properties_proxy =
      g_dbus_proxy_new_sync(connection_wrapper.get(), G_DBUS_PROXY_FLAGS_NONE, nullptr, kBluezService,
                            adapter_path->c_str(), kPropertiesInterface, nullptr, &error);
  if (!properties_proxy) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }

  auto properties_proxy_wrapper = GObjectWrapper::make_proxy(properties_proxy);

  if (!EnsureAdapterBooleanProperty(properties_proxy_wrapper.get(), "Powered", true, &error)) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }

  if (!EnsureAdapterBooleanProperty(properties_proxy_wrapper.get(), "Discoverable", true, &error)) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }

  if (!EnsureAdapterBooleanProperty(properties_proxy_wrapper.get(), "Pairable", true, &error)) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }

  GDBusProxy* advertising_manager =
      g_dbus_proxy_new_sync(connection_wrapper.get(), G_DBUS_PROXY_FLAGS_NONE, nullptr, kBluezService,
                            adapter_path->c_str(), kAdvertisingManagerInterface, nullptr, &error);
  if (!advertising_manager) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }

  auto advertising_manager_wrapper = GObjectWrapper::make_proxy(advertising_manager);

  auto* introspection = EnsureIntrospection(ctx, &error);
  if (!introspection) {
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::UnknownError, message);
  }

  ctx.service_uuid = options.service_uuid;
  ctx.local_name = options.local_name;

  guint registration_id =
      g_dbus_connection_register_object(connection_wrapper.get(), ctx.object_path.c_str(), introspection->interfaces[0],
                                        &kAdvertisementVTable, &ctx, nullptr, &error);
  if (registration_id == 0) {
    auto message = TakeErrorMessage(&error);
    ctx.service_uuid.clear();
    ctx.local_name.clear();
    return MakeErrorResult(ErrorCode::UnknownError, message);
  }

  ctx.registration_id = registration_id;

  GVariantBuilder dict_builder;
  g_variant_builder_init(&dict_builder, G_VARIANT_TYPE("a{sv}"));
  GVariant* register_params = g_variant_new("(oa{sv})", ctx.object_path.c_str(), g_variant_builder_end(&dict_builder));

  GVariant* register_result = g_dbus_proxy_call_sync(advertising_manager_wrapper.get(), "RegisterAdvertisement",
                                                     register_params, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
  if (!register_result) {
    g_dbus_connection_unregister_object(connection_wrapper.get(), registration_id);
    ctx.registration_id = 0;
    ctx.service_uuid.clear();
    ctx.local_name.clear();
    auto message = TakeErrorMessage(&error);
    return MakeErrorResult(ErrorCode::BluetoothServiceUnavailable, message);
  }
  g_variant_unref(register_result);

  ctx.connection = static_cast<GDBusConnection*>(g_object_ref(connection_wrapper.get()));
  ctx.advertising_manager = static_cast<GDBusProxy*>(g_object_ref(advertising_manager_wrapper.get()));
  ctx.properties_proxy = static_cast<GDBusProxy*>(g_object_ref(properties_proxy_wrapper.get()));
  ctx.active = true;
  result.success = true;
  return result;
}

Result StopAdvertisement() {
  Result result;
  ScopedTimer timer(result.operation_time);

  auto& ctx = Context();
  if (!ctx.active) {
    result.success = true;
    return result;
  }

  GError* error = nullptr;

  if (ctx.advertising_manager) {
    GVariant* unregister_result = g_dbus_proxy_call_sync(ctx.advertising_manager, "UnregisterAdvertisement",
                                                         g_variant_new("(o)", ctx.object_path.c_str()),
                                                         G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (!unregister_result && error) {
      result = MakeErrorResult(ErrorCode::UnknownError, TakeErrorMessage(&error));
    } else if (unregister_result) {
      g_variant_unref(unregister_result);
    }
  }

  if (ctx.connection && ctx.registration_id != 0) {
    g_dbus_connection_unregister_object(ctx.connection, ctx.registration_id);
  }

  if (ctx.connection) {
    g_object_unref(ctx.connection);
    ctx.connection = nullptr;
  }

  if (ctx.advertising_manager) {
    g_object_unref(ctx.advertising_manager);
    ctx.advertising_manager = nullptr;
  }

  if (ctx.properties_proxy) {
    g_object_unref(ctx.properties_proxy);
    ctx.properties_proxy = nullptr;
  }

  ctx.registration_id = 0;
  ctx.service_uuid.clear();
  ctx.local_name.clear();
  ctx.active = false;

  result.success = result.error_code == 0;
  return result;
}

}  // namespace ble
