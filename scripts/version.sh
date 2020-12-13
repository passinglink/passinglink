#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")

git -C "$SCRIPT_PATH" describe --tags
