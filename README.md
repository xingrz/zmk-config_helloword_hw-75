zmk-config for Xikii HW-75 (瀚文 75)
========

![HW-75](https://github.com/peng-zhihui/HelloWord-Keyboard/raw/main/5.Docs/2.Images/hw1.jpg)

[瀚文 75 (HW-75)](https://github.com/peng-zhihui/HelloWord-Keyboard) 是一款由稚晖君 ([@peng-zhihui](https://github.com/peng-zhihui)) 设计、Xikii Industy 生产的模块化机械键盘。

本仓库是针对 HW-75 的 [ZMK](https://github.com/zmkfirmware/zmk) 编译配置。请注意，虽然 HW-75 的设计是开源的，但是 Xikii 官方团的 PCB 在开源版基础上有细微调整。本配置仅针对官方团的 PCB 适配。

## 烧录 (针对一般用户)

1. 请确保已经在 HW-75 中烧入了 DFU bootloader。详见[这篇文章](https://github.com/peng-zhihui/HelloWord-Keyboard/discussions/77)；
2. 按住键盘的 Fn 键插入 USB，打开 [WebDFU](https://devanlai.github.io/webdfu/dfu-util/) 选择固件进行烧录。

## 快速定制

本固件默认为针对 Mac 用户的按键布局。如果你需要在 Windows 上使用，可以自行修改 [`xikii_hw75.keymap`](config/boards/arm/xikii_hw75/xikii_hw75.keymap)。

你可以直接 Fork 本仓库到自己账号名下，并启用 GitHub Actions。修改提交相关文件之后，在自己的 Actions 中耐心等待云端编译完成，然后下载固件并烧录。

## 二次开发

参考 [ZMK 的上手文档](https://zmk.dev/docs/development/setup#prerequisites)。

```sh
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/path/to/gcc-arm-none-eabi-10.3-2021.10

west init -l config
west update
west zephyr-export
west build -s zmk/app -b xikii_hw75 -- -DZMK_CONFIG=$PWD/config
west flash
```

## 相关链接

* [使用开源的 DAPLink 调试器及 pyOCD 烧录程序](https://github.com/peng-zhihui/HelloWord-Keyboard/discussions/76)
* [一个只有 4KB 的 DFU bootloader](https://github.com/peng-zhihui/HelloWord-Keyboard/discussions/77)
* [ZMK Firmware](https://zmk.dev/)

## 协议

[Apache License 2.0](LICENSE)
