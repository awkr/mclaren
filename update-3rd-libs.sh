#!/usr/bin/env zsh

set -ex

pushd third-party/SDL
  git checkout main
  git pull
popd

pushd third-party/imgui
  git checkout master
  git pull
popd

pushd third-party/volk
  git checkout master
  git pull
popd

pushd third-party/microprofile
  git checkout master
  git pull
popd
