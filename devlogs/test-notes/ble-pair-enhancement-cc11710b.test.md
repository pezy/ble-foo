# 测试日记

## 计划

请见[这里](../../.cursor/plans/ble-pair-enhancement-cc11710b.plan.md)

## 第一次测试

```sh
root@vita-01b2-065-x5:/app/ble# ./ble_pair
5C:52:30:0C:16:13 NZBaDLYfApomvsxC/t614wczg N/A Disconnected
```

默认无参，输出正常。

```sh
root@vita-01b2-065-x5:/app/ble# ./ble_pair E2:22:F2:80:18:40
Attempting to pair with device: E2:22:F2:80:18:40
Error: GDBus.Error:org.freedesktop.DBus.Error.UnknownObject: Method "Pair" with signature "" on interface "org.bluez.Device1" doesn't exist

Error: Pairing failed - Device may not be in pairing mode or pairing was rejected
```

主动匹配出错。

原因是该设备必须已经被发现，而我们当前的模式是只实现配对，完全不管扫描所致。
此时只要配合 bluetoothctl 里，`scan on` 和 `scan off` 确认设备扫描到。就可以输出正常了：

```sh
root@vita-01b2-065-x5:/app/ble# ./ble_pair E2:22:F2:80:18:40
Attempting to pair with device: E2:22:F2:80:18:40
Successfully paired with device: E2:22:F2:80:18:40
Pairing time: 1187 ms
```

✅ 测试成功！
