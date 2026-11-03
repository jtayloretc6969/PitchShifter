#!/bin/bash
set -euo pipefail
if [[ "$(uname -s)" != Darwin ]]; then
  echo 'This installer must run on macOS.' >&2
  exit 1
fi
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
if ! xcode-select -p >/dev/null 2>&1; then
  echo 'Install Apple developer tools with: xcode-select --install'
  echo 'Once installation finishes, run this script again.'
  exit 1
fi
if command -v cmake >/dev/null 2>&1; then
  cmake_bin="$(command -v cmake)"
elif [[ -x /Applications/CMake.app/Contents/bin/cmake ]]; then
  cmake_bin=/Applications/CMake.app/Contents/bin/cmake
else
  echo 'Install the macOS Universal CMake application from https://cmake.org/download/'
  echo 'Move CMake.app to Applications, then run this script again.'
  exit 1
fi
"$cmake_bin" -S "$project_dir" -B "$project_dir/build-mac" -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
"$cmake_bin" --build "$project_dir/build-mac" --config Release --parallel 3
"$cmake_bin" --build "$project_dir/build-mac" --config Release --target test
plugin_source="$project_dir/build-mac/FluteShift_artefacts/Release/VST3/FluteShift.vst3"
# JUCE generates moduleinfo.json after linking. Seal the completed bundle.
codesign --force --sign - --timestamp=none "$plugin_source"
codesign --verify --deep --strict --verbose=2 "$plugin_source"
plugin_dest="$HOME/Library/Audio/Plug-Ins/VST3/FluteShift.vst3"
mkdir -p "$(dirname "$plugin_dest")"
if [[ -e "$plugin_dest" ]]; then
  echo "An existing plugin is already installed at: $plugin_dest"
  echo "Build succeeded. Move the existing plugin aside, then rerun this script."
  exit 1
fi
ditto "$plugin_source" "$plugin_dest"
echo "Installed: $plugin_dest"
echo 'In Ableton Live, enable VST3 system folders in Settings > Plug-Ins and rescan.'
