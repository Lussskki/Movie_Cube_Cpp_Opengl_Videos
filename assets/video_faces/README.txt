Put extracted video frames here.

Simple setup:
1. Put frames in assets/video_faces/face0.
2. Name them like frame0001.ppm, frame0002.ppm, frame0003.ppm.
3. The cube will play face0 on all six faces.

Separate videos:
- face0 = back
- face1 = front
- face2 = left
- face3 = right
- face4 = bottom
- face5 = top

Example ffmpeg command:
ffmpeg -i your-video.mp4 -vf "fps=24,scale=256:256" assets/video_faces/face0/frame%04d.ppm
