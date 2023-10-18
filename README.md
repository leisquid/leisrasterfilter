# Leisrasterfilter

## 简介

一坨用来示范打印机的 raster filter 工作流程以及将 CUPS Raster 格式转为 bitmap 格式的源文件。采用 AGPL 3.0 协议开源。

待仓库中的代码全部完成后，您可以在遵循 AGPL 3.0 的协议下用这些代码做您任何想做的事情。

部分代码参考 [fruitsamples/SampleRaster](https://github.com/fruitsamples/SampleRaster)。

## 编译

```sh
gcc -g `cups-config --cflags` ./rastertosample.c ./common.c `cups-config --libs` -o ./rastertosample
```

Leisquid Li

2023.10.10

一编于 23.10.16

二编于 23.10.17
