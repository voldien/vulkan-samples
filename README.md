# Vulkan Samples #

[![Linux Build](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/voldien/vulkan-samples.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/voldien/vulkan-samples/context:cpp)

A set of vulkan sample for educational purposes.

## Required Packages

```bash
apt install libfmt-dev libglm-dev libsdl2-dev libassimp-dev libfreeimage-dev
```

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make # Build samples
make Shaders # Build Shaders
```

# Window Based Samples

## Startup Window - 
The simplest form of a vulkan window

## Triangle - 
Render a single triangle

## Triangle - Vertex Interpolation


## Shader Uniforms - Cube


## Texture


## SkyBox - CubeMap


## SkyBox - Panoramic


## Push Constant - VK_KHR_push_descriptor

## NormaMap

## Direction Light

## Shadow Mapping

## Tessellation Basic

## PN Triangle Tessellation

## Ambient Occlusion

## Billboard

## Particle System - Compute Shader

## ReactionDiffusion - Compute Shader

## Mandelbrot - Compute Shader 

## Video Playback

## Memory Benchmark - Benchmarking the performance of transfering 

memory on the device between various heap.
