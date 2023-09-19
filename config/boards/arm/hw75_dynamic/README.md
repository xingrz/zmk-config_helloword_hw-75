HW-75 Dynamic
==========

本目录是 HW-75 扩展模块的 ZMK 配置。

## 硬件版本

本配置基于稚晖君在[立创开源](https://oshwhub.com/pengzhihui/b11afae464c54a3e8d0f77e1f92dc7b7)发布的 PCB 适配，但有些许差异：

| 版本 | 文件前缀 | 备注 |
|--------|--------------------|------|
| A | `hw75_dynamic@A-zmk` | 稚晖君在[立创开源](https://oshwhub.com/pengzhihui/b11afae464c54a3e8d0f77e1f92dc7b7)的原始设计，三叶虫 (wow) 一期、二期团，大部分开源团使用的版本 |
| B | `hw75_dynamic@B-zmk` | 升级版墨水屏（支持局部刷新），三叶虫 (wow) 三期团 |

## 烧录

1. 从 [Releases](https://github.com/xingrz/zmk-config_helloword_hw-75/releases/latest) 下载最新的固件；
2. 参考[固件更新说明](https://github.com/xingrz/zmk-config_helloword_hw-75/wiki/%E5%9B%BA%E4%BB%B6%E6%9B%B4%E6%96%B0-(%E6%89%A9%E5%B1%95))烧入固件。

## 二次开发

### 环境准备

参考 [ZMK 的上手文档](https://zmk.dev/docs/development/setup#prerequisites)。

```sh
sudo apt-get update
sudo apt-get install -y python3-pip protobuf-compiler curl
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs
sudo pip install -U fonttools
sudo npm install -g lv_font_conv
```

### 编译

```sh
west init -l config
west update
west zephyr-export
west build -p -s zmk/app -b hw75_dynamic -- -DZMK_CONFIG=$PWD/config
west flash
```

默认会构建最新[硬件版本](#硬件版本)的配置。如果你需要构建不同版本，可以使用下面的命令：

```sh
west build -p -s zmk/app -b hw75_dynamic@A -- -DZMK_CONFIG=$PWD/config -DKEYMAP_FILE=$PWD/config/hw75_dynamic.keymap
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
pyocd rtt -t stm32f405rg
```

## 致谢

感谢[三叶虫](https://space.bilibili.com/21972064)提供用于开发的硬件。
