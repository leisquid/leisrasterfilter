# Leisrasterfilter

## 简介

一坨用来示范打印机的 CUPS Raster filter 工作流程以及将 CUPS Raster 格式转为 bitmap 格式的源文件。采用 GNU Affero General Public License (下称 AGPL 3.0) 开源。

待仓库中的代码全部完成后，您可以在遵循 [AGPL 3.0](./COPYING) 的协议下用这些代码做您任何想做的事情。

部分代码参考 [fruitsamples/SampleRaster](https://github.com/fruitsamples/SampleRaster)。

## 编译

```sh
gcc -g `cups-config --cflags` ./rastertosample.c ./common.c `cups-config --libs` -o ./rastertosample
```

## 免责说明

(1) 此软件为演示工具，算法为个人想法，代码仅供参考，不代表 CUPS 官方或者标准实现。

(2) 此软件的代码可能存在潜在的缺陷，如不可移植或者其他环境不通过编译的情况。根据 [AGPL 3.0](./COPYING)，代码作者不会对特定用途做出任何担保。烦请根据您的实际环境自行解决这些问题。如代码有算法层面的问题，请提 issue 或者联系我。

(3) 根据 Free Software Foundation 的要求，如产生了涉及到 AGPL 3.0 的法律纠纷，请以 Free Software Foundation 发布的 AGPL 3.0 英文原版为准，而不是翻译版或者其他非 Free Software Foundation 发布的版本。Free Software Foundation 发布的 AGPL 3.0 可以在 <https://www.gnu.org/licenses/agpl-3.0.md> 找到；本软件使用的版本 ([./COPYING](./COPYING)) 是对 Free Software Foundation 发布版本的直接拷贝。

## Leissoft

计划以后自己所有以 `Leis` 开头的软件，在处理完可能会有的版权冲突后，都以 AGPL 3.0 或者更新版的协议开源。

这是我实际上的第一个开源软件，请多关照。

## 更新履历

### 23.10.10

开始软件试作。目前所有的代码均为未完工状态。

---

Leisquid Li

2023.10.10

更新于 23.10.19
