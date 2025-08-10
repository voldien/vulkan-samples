# Vulkan Samples #

[![Linux Build](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml/badge.svg)](https://github.com/voldien/vulkan-samples/actions/workflows/linux-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A collection of Vulkan Samples, work in progress, for personal educational purposes.

## Required Packages

```bash
apt install libfmt-dev libglm-dev libsdl2-dev libassimp-dev libfreeimage-dev
```

## Build Instruction

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
make DownloadAsset
```

## Window Based Samples

### Startup Window -

The simplest form of a vulkan window

## Triangle - Vertex Interpolation

Render a single triangle with vertex color interpolation.

## Shader Uniforms - Cube

## Texture - Diffuse Texture

Render a texture cube with simple texture sampling on each face.

## SkyBox - CubeMap

Render Skybox using cubemap, six 2D textures.

## SkyBox - Panoramic

Render skybox using a single texture encoded with equirectangular for projecting to sphere .

## Push Constant - VK_KHR_push_descriptor

Render element using push constant for updating model matrix.

## NormaMap

Render geometry with normal map to add additional details on each face.

## Particle System - Compute Shader

Particle system smoke, compute with compute shader.

## ReactionDiffusion - Compute Shader

## Mandelbrot - Compute Shader

A Mandelbrot render using compute shader.

## Game of Life - Compute Shader

A Mandelbrot render using compute shader.

## Video Playback

## Memory Benchmark - Benchmarking the performance of transfering

memory on the device between various heap.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

Models downloaded from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)
