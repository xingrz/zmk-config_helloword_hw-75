HW-75 Dynamic
==========

本目录是 HW-75 扩展模块的 ZMK 配置。

## PCB

本配置基于稚晖君在[立创开源](https://oshwhub.com/pengzhihui/b11afae464c54a3e8d0f77e1f92dc7b7)发布的 PCB 适配。

## 开发进度

本固件正处于开发阶段，一些特性可能暂时缺失或未最终定型，请保持关注。

- [x] 状态显示 (OLED)
- [x] 旋钮
- [ ] 旋钮力反馈 (FOC)
- [ ] 墨水屏
- [ ] 上位机
- [ ] 键盘联动

## 烧录

```sh
pyocd load --pack=Keil.STM32F4xx_DFP.2.16.0.pack --target stm32f405rg hw75_dynamic-zmk.hex
```

## 二次开发

参考 [ZMK 的上手文档](https://zmk.dev/docs/development/setup#prerequisites)。

```sh
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/path/to/gcc-arm-none-eabi-10.3-2021.10

west init -l config
west update
west zephyr-export
west build -s zmk/app -b hw75_dynamic -- -DZMK_CONFIG=$PWD/config
west flash
```

### 日志

日志通过 SWD 口使用 SEGGER RTT 协议输出，默认关闭，可通过如下编译选项开启：

```sh
west build -s zmk/app -b hw75_dynamic -- -DZMK_CONFIG=$PWD/config \
    -DCONFIG_ZMK_RTT_LOGGING=y \
    -DCONFIG_ZMK_LOG_LEVEL_DBG=y
```

查看日志：

```sh
pyocd rtt --pack=Keil.STM32F4xx_DFP.2.16.0.pack --target stm32f405rg
```

## 致谢

感谢[三叶虫本虫](https://space.bilibili.com/21972064)提供用于开发的硬件。
