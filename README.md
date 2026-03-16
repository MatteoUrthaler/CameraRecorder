# Camera Recording System

A Windows-based multi-camera video recording system using OpenCV for capturing and JSON for configuration. It continuously records from multiple IP cameras in fixed-length video segments.

## Features

- Records from multiple RTSP/HTTP camera streams in parallel.
- Per-camera control via JSON (`active`, `name`, `stream`).
- Fixed recording duration in minutes for all cameras.
- Automatic restart of camera workers after each finished segment.
- Graceful shutdown on console close or Ctrl+C, with cleanup of child processes.
- Simple batch script wrapper with timestamped logging.

## Project Structure

- `CameraStart.exe`: records a single camera stream to `saves/<CamName>/<CamName>_YYYYMMDD_HHMMSS.mp4`.
- `Controller.exe`: reads `CamInfo.json`, launches one worker per active camera, restarts them as they finish, and handles shutdown.
- `CamInfo.json`: configuration for camera list and global recording length.
- `Start.bat`: starts the controller, redirects output to `logs/controller_log_<timestamp>.txt`.

## Configuration

- `name`: logical camera name, used for output folder and filenames.
- `stream`: RTSP/HTTP URL of the camera stream.
- `active`: 1 to enable, 0 to disable a camera.
- `video_length_min`: length of each recording segment in minutes.

## Runtime Requirements

- Windows (uses Win32 APIs like `CreateProcess`, `WaitForSingleObject`, `TerminateProcess`, `SetConsoleCtrlHandler`).
- OpenCV with FFMPEG for video capture and encoding.
- `nlohmann::json` header for JSON parsing.
- PowerShell available for the batch script logging.

## Usage

1. Build `CameraStart.exe` and `Controller.exe` into the same directory as `CamInfo.json`.
2. Edit `CamInfo.json` to add or remove cameras and set `video_length_min`.
3. Adjust the `cd` path in `Start.bat` to point to the directory containing the executables and `CamInfo.json`.
4. Run `Start.bat` to start the controller; video files will appear under `saves/<CameraName>/`.
