![](./resources/official_armmbed_example_badge.png)

# Mbed OS example for Google cloud

The example project is part of the [Arm Mbed OS Official Examples](https://os.mbed.com/code/). It contains an application that uses the [Google Cloud IoT device SDK](https://github.com/GoogleCloudPlatform/iot-device-sdk-embedded-c/) to connect to an IoT core instance using the MQTT protocol and publishes a message every 3 seconds for 30 seconds or until a message is received.

You can build the project with all supported [Mbed OS build tools](https://os.mbed.com/docs/mbed-os/latest/tools/index.html). However, this example project specifically refers to the command-line interface tool [Arm Mbed CLI](https://github.com/ARMmbed/mbed-cli#installing-mbed-cli).
(Note: To see a rendered example you can import into the Arm Online Compiler, please see our [import quick start](https://os.mbed.com/docs/mbed-os/latest/quick-start/online-with-the-online-compiler.html#importing-the-code).)

It has been tested on K64F with Ethernet and DISCO_L475VG_IOT01A with WiFi, but any Mbed OS 6 targets with Internet access should work.

## Mbed OS build tools

### Mbed CLI 2
Starting with version 6.5, Mbed OS uses Mbed CLI 2. It uses Ninja as a build system, and CMake to generate the build environment and manage the build process in a compiler-independent manner. If you are working with Mbed OS version prior to 6.5 then check the section [Mbed CLI 1](#mbed-cli-1).
1. [Install Mbed CLI 2](https://os.mbed.com/docs/mbed-os/latest/build-tools/install-or-upgrade.html).
1. From the command-line, import the example: `mbed-tools import mbed-os-example-for-google-iot-cloud`
1. Change the current directory to where the project was imported.

### Mbed CLI 1
1. [Install Mbed CLI 1](https://os.mbed.com/docs/mbed-os/latest/quick-start/offline-with-mbed-cli.html).
1. From the command-line, import the example: `mbed import mbed-os-example-for-google-iot-cloud`
1. Change the current directory to where the project was imported.

## Configuring the Google Cloud IoT Core

1. Follow the below steps to generate the key pair and certificate using OpenSSL

    1. create a P-256 Elliptic Curve key pair
        ```
        $ openssl ecparam -genkey -name prime256v1 -noout -out ec_private.pem
        $ openssl ec -in ec_private.pem -pubout -out ec_public.pem        
        ```
        Once you have generated the certificates, you will need to place `ec_private.pem` in `clientKey` array in the [`google_cloud_credentials.h`](./google_cloud_credentials.h) file of this example and `ec_public.pem` to be uploaded in Google Cloud portal while creating the device.

1. Create a Google Cloud account if you don't have one, and log in to it.

    __NOTE:__ If you have an admin for your Google Cloud account, please contact them to add a user to the account. You should obtain your login credentials from your admin in this case.

1. Follow Google Cloud IoT Core official documentation [here](https://cloud.google.com/iot/docs/how-tos) to
    * use existing project created by admin or create a new project if you have permission
    * create a device registry and add a device to the created registry
1. Follow Google Cloud pub/sub official documentation [here](https://cloud.google.com/pubsub/docs) to
    * create a pub/sub topic
    * create a subscription for the created topic

1. Update all the cloud credentials in [`mbed_app.json`](./mbed_app.json) file,
    * set project id `"google-cloud-project-id".value`
    * set cloud region `"google-cloud-region".value`
    * set device registry `"google-cloud-registry".value`
    * set device id `"google-cloud-device-id".value`
    * set a topic `"google-cloud-mqtt-topic".value` that both your device and the cloud can publish messages to

## Building and running

1. (If you want to use *WiFi*) set `nsapi.default-wifi-ssid` to your WiFi name and `nsapi.default-wifi-password` to your WiFi password, keeping any escaped quotes (`\"`). If you use a different target, replace `"DISCO_L475VG_IOT01A"` with your target and remove `"target.components_add": ["wifi_ism43362"]` (unless it uses the same ISM43362 WiFi module). For example, to use NUCLEO-F429ZI:
    ```json
    "NUCLEO-F429ZI": {
        "target.network-default-interface-type": "WIFI",
        "nsapi.default-wifi-security": "WPA_WPA2",
        "nsapi.default-wifi-ssid": "\"SSID\"",
        "nsapi.default-wifi-password": "\"PASSWORD\""
    }
    ```

1. For Ethernet (e.g on K64F), connect a cable to the port.
1. Connect your development board to your PC with a USB cable.
1. Compile, flash and run the example

    * Mbed CLI 2

    ```bash
    $ mbed-tools compile -m <TARGET> -t <TOOLCHAIN> --flash  --sterm
    ```

    * Mbed CLI 1

    ```bash
    $ mbed compile -m <TARGET> -t <TOOLCHAIN> --flash  --sterm
    ```

For example, `<TARGET>` can be `DISCO_L475VG_IOT01A` and `<TOOLCHAIN>` can be `GCC_ARM` if you want to use this combination.

## Expected output

The example starts fetching time from an NTP server because the [`security processes`](https://github.com/GoogleCloudPlatform/iot-device-sdk-embedded-c/blob/master/doc/user_guide.md#accurate-real-time-clock) require correct date/time when it tries to connect with Google Cloud IoT Core:
```
Time: Thu Aug 13 17:36:32 2020
```
**Note**: If it fails to fetch time or the reported time makes no sense (e.g. the year 2100), the NTP server might be unreachable for a moment. It is usually enough to reset the board to rerun the example.

Then it connects with MQTT host and respective port
```
connected to mqtt.2030.ltsapis.goog:8883
```

Now it initializes the Google Cloud IoT device SDK and starts sending one message on every 3 seconds:
```
publishing msg "10 messages left to send, or until we receive a reply"
Message sent successfully
publishing msg "9 messages left to send, or until we receive a reply"
Message sent successfully
publishing msg "8 messages left to send, or until we receive a reply"
Message sent successfully
...
```
## Sending cloud-to-device messages

On the Google Cloud portal, go to the page of your device, you can send a message to your device using "SEND COMMAND".

Once the message is received by your device, it prints:
```
Message body: Hello from Google Cloud IoT Core
```

## Tracing the IoT Core client

To monitor incoming/outgoing IoT Core events, you can enable tracing by setting `"google-client.iotc-debug-output"` in [`mbed_app.json`](./mbed_app.json) to `true` and if you want BSP debug logs then set `"google-client.iotc-bsp-debug-log"` also to `true`. This can be useful for debugging issues related to connections or credentials.

Sample trace for the first message sent:

```
Time: Thu Aug 13 18:07:28 2020
[1597342048000][iotc.c:446 (iotc_connect_to)] New host:port [mqtt.2030.ltsapis.goog]:[8883]
[1597342048000][iotc.c:462 (iotc_connect_to)] new backoff value: 0
[1597342048000][iotc_control_topic_layer.c:77 (iotc_control_topic_layer_init)] control topic layer initializing..
[1597342048000][iotc_tls_layer.c:560 (iotc_tls_layer_init)] BSP TLS initialization successfull
[1597342048000][iotc_tls_layer.c:591 (iotc_tls_layer_init)] successfully initialized BSP TLS module
[1597342048000][iotc_io_net_layer.c:57 (iotc_io_net_layer_connect)] Connecting layer [0] to the endpoint: mqtt.2030.ltsapis.goog:8883
[1597342049000][iotc_io_net_layer.c:90 (iotc_io_net_layer_connect)] Connection successful!
...
```

## Troubleshooting
If you have problems, you can review the [documentation](https://os.mbed.com/docs/latest/tutorials/debugging.html) for suggestions on what could be wrong and how to fix it.

## Related links
* [Mbed boards](https://os.mbed.com/platforms/)
* [Mbed OS Configuration](https://os.mbed.com/docs/latest/reference/configuration.html).
* [Mbed OS Serial Communication](https://os.mbed.com/docs/latest/tutorials/serial-communication.html).
* [Google Cloud IoT Device SDK for Embedded C](https://github.com/GoogleCloudPlatform/iot-device-sdk-embedded-c)
* [Google Cloud IoT Core](https://cloud.google.com/iot-core)

## License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide.
