# Movie Cube C++ OpenGL Videos

Movie Cube is a small C++ OpenGL project that renders a 3D cube with video frames playing on its faces. The scene uses GLFW, GLEW, and GLM, and it is configured to build through `cpp-starter-cli`, my CLI project: https://github.com/Lussskki/Node.js-Javascript-Cpp-CLI.

The cube stays fixed in the scene while the camera can move freely with keyboard and mouse controls. Video frames are loaded from `.ppm` image sequences, and audio is played from the original `.mp4` file through `ffplay`.

## Demo

[![Movie Cube C++ OpenGL Videos demo](https://img.youtube.com/vi/V_3aXiOl_Qo/maxresdefault.jpg)](https://youtu.be/V_3aXiOl_Qo)

[Watch the demo on YouTube](https://youtu.be/V_3aXiOl_Qo)

## Features

- OpenGL 3.3 cube rendering
- Video texture playback on all cube faces
- Audio playback using `ffplay`
- First-person camera movement
- Local project layout generated for `cpp-starter-cli`
- Bundled include and lib folders for the current Windows/MinGW setup

## Controls

- `W` move forward
- `S` move backward
- `A` move left
- `D` move right
- `Space` move up
- `Left Shift` move down
- Mouse to look around
- `Esc` close the window

## Build And Run

From the project folder:

```bash
cpp-starter-cli build
cpp-starter-cli run
```

The build output is `MovieCube.exe`.

## Video Frames

The app reads video frames from:

```text
assets/video_faces/face0/
```

Frames should be named like:

```text
frame0001.ppm
frame0002.ppm
frame0003.ppm
```

Convert an MP4 video into frames with FFmpeg:

```bash
ffmpeg -i your-video.mp4 -vf "fps=24,scale=256:256" assets/video_faces/face0/frame%04d.ppm
```

If only `face0` has frames, the same video appears on all six cube faces. Separate frame folders can be used for different videos per face:

```text
assets/video_faces/face0
assets/video_faces/face1
assets/video_faces/face2
assets/video_faces/face3
assets/video_faces/face4
assets/video_faces/face5
```

## Audio

Audio is played from the original MP4 path configured in `src/Cube.cpp`:

```cpp
const std::string AUDIO_VIDEO_PATH = "C:\\Users\\Luka\\Videos\\2025-09-26 23-28-31.mp4";
```

To use a different video with sound, update this path and rebuild.

## Project Structure

```text
Movie Cube/
+-- assets/video_faces/
+-- include/
+-- lib/
+-- src/
|   +-- Cube.cpp
|   +-- main.cpp
+-- .cpp-cli-config.json
+-- README.md
```

## Requirements

- Windows with MinGW `g++`
- `cpp-starter-cli`
- FFmpeg and `ffplay`
- OpenGL-compatible graphics driver
