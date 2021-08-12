# Vulkan Samples #

[![Linux Build](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml)


```bash
apt install libfmt-dev libvulkan-dev libglm-dev libsdl2-dev
```

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
make Shaders
```

## Startup Window ##

## Triangle ## 

## Triangle - Vertex Interpolation ##

## Particle System - Compute Shader ##

##  Shader Uniforms - Cube ##

| Name - Headless | Description |
| --- | --- |
| StartUp Window | The simplest form of a vulkan window |
| Triangle | Render a single triangle |
| | |
| Memory Transfer     | Benchmark the transfer rate of memory.    |
