

静态协议栈使用说明







### BlueNRG-2 默认设置如下

   + BlueNRG-2使用 OTA_ServiceManager + 静态协议栈 Flash分布如下图

| Flash                  | size  |
| :--------------------- | :---- |
| NVM                    | 4KB   |
| APP                    | 134KB |
| BLE_OTA_ServiceManager | 10KB  |
| BLE Stack(Full stack)  | 108KB |
|                        |       |

### BlueNRG-2 默认宏定义

| 固件                    | MEMORY FLASH APP SIZE [Linker] | MEMORY FLASH APP OFFSET [Linker] | MEMORY RAM APP OFFSET [Linker] | RESET MANAGER SIZE [PreDefine] | SERVICE MANAGER SIZE [PreDefine] | SERVICE MANAGER OFFSET [PreDefine] |      |
| ----------------------- | ------------------------------ | -------------------------------- | ------------------------------ | ------------------------------ | -------------------------------- | ---------------------------------- | ---- |
| BLE_StaticStack         | 0x22800                        | none(default 0)                  | none(default 0)                | 0                              | none                             | none                               |      |
| BLE_OTA_ServiceManager  | 0x3000                         | 0x22800                          | 0x810                          | none                           | 0x25800                          | 0x22800                            |      |
| BLE_MultipleConnections | none                           | 0x25800                          | 0x810                          | none                           | none                             | none                               |      |
|                         |                                |                                  |                                |                                |                                  |                                    |      |



### BlueNRG-1使用 OTA_ServiceManager + 静态协议栈 Flash分布如下图

| Flash                   | size                               |
| :---------------------- | :--------------------------------- |
| NVM                     | 4KB                                |
| APP                     | 64KB    //                         |
| BLE_OTA_ServiceManager  | 10KB      // 0x0014800 ~  0x017000 |
| BLE Stack(basic statck) | 82KB      // 0x0000000 ~ 0x0014800 |
|                         |                                    |

### 

