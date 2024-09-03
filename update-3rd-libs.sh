#!/usr/bin/env zsh

set -ex

pushd third-party/SDL
  git pull
popd

pushd third-party/imgui
  git pull
popd

pushd third-party/volk
  git pull
popd

pushd third-party/VulkanMemoryAllocator
  git pull
popd

pushd third-party/microprofile
  git pull
popd
