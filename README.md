![](./resources/official_armmbed_example_badge.png)

# Google Cloud Mbed OS example


## Summary of steps:

## Downloading this project
1. [Install Mbed CLI](https://os.mbed.com/docs/mbed-os/latest/quick-start/offline-with-mbed-cli.html).

1. Clone this repository on your system, and change the current directory to where the project was cloned:

    ```
    $ git clone git@github.com:armmbed/mbed-os-example-google-cloud && cd mbed-os-example-google-cloud
    $ mbed deploy
    ```

    Alternatively, you can download the example project with Arm Mbed CLI using the `import` subcommand:

    ```
    $ mbed import mbed-os-example-google-cloud && cd mbed-os-example-google-cloud
    ```

## Building and running

1. Configure the example using [`mbed_app.json`](./mbed_app.json)
1. Follow the command to generate your credentials in [`iotc_RootCA_list.h`](./iotc_RootCA_list.h), [`iotc_RootCA_list.c`](./iotc_RootCA_list.c) from *.pem file:

    ```
    $ ./mbed-google-client/iot-device-sdk-embedded-c/tools/create_buffer.py --file_name ./roots.pem --array_name iotc_RootCA_list --out_path . --no-pretend
    ```

1. Connect a USB cable between the USB port on the board and the host computer.
1. <a name="build_cmd"></a> Run the following command to build the example project, program the microcontroller flash memory, and open a serial terminal:
    ```
    $ mbed compile -m detect -t <TOOLCHAIN> --flash --sterm
    ```

Alternatively, you can manually copy the binary to the board, which you mount on the host computer over USB.
The binary is located at `./BUILD/<TARGET>/<TOOLCHAIN>/mbed-os-example-google-cloud.bin`.

Depending on the target, you can build the example project with the `GCC_ARM` or `ARM` toolchain. After installing Arm Mbed CLI, run the command below to determine which toolchain supports your target:

```
$ mbed compile -S
```

## Expected output



## Troubleshooting
If you have problems, you can review the [documentation](https://os.mbed.com/docs/latest/tutorials/debugging.html) for suggestions on what could be wrong and how to fix it.

## Related Links

* [Mbed OS Stats API](https://os.mbed.com/docs/latest/apis/mbed-statistics.html).
* [Mbed OS Configuration](https://os.mbed.com/docs/latest/reference/configuration.html).
* [Mbed OS Serial Communication](https://os.mbed.com/docs/latest/tutorials/serial-communication.html).
* [Mbed OS bare metal](https://os.mbed.com/docs/mbed-os/latest/reference/mbed-os-bare-metal.html).
* [Mbed boards](https://os.mbed.com/platforms/).

### License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see contributing.md for more info.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide.
