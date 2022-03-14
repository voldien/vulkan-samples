# Vulkan Samples #

[![Linux Build](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/voldien/vulkan-samples.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/voldien/vulkan-samples/context:cpp)

A set of vulkan sample for educational purposes.

## Required Packages

```bash
apt install libfmt-dev libglm-dev libsdl2-dev
```

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make # Build samples
make Shaders # Build Shaders
```

## Startup Window

## Triangle

## Triangle - Vertex Interpolation


## Shader Uniforms - Cube

## Textured Cube

## SkyBox

## Direction Light


## Panoramic - CubeMap

## Panoramic - 

## Shadow Mapping

## Tessellation Basic

## PN Triangle Tessellation

## Ambient Occlusion

## Billboard


## Particle System - Compute Shader
## 

## ReactionDiffusion - Compute Shader

## Mandelbrot - Compute Shader 

## Video Playback

## Memory Benchmark


| Name - Headless | Description |
| --- | --- |
| StartUp Window | The simplest form of a vulkan window |
| Triangle | Render a single triangle |
| | |
| Memory Transfer     | Benchmark the transfer rate of memory.    |
| Particle Simulation | |
| Font Render    | |
