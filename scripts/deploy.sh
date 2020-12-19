#!/bin/bash

SCRIPT_PATH=$(dirname "$(realpath -s "$BASH_SOURCE")")

set -euo pipefail


BOARDS=(pl_e73 pl_dongle stm32f4_disco)
BOARDS_JSON='["pl_e73", "pl_dongle", "stm32f4_disco"]'

TAG=${GITHUB_REF#refs/tags/}
BRANCH=${GITHUB_REF#refs/heads/}
VERSION=$("$SCRIPT_PATH/version.sh")
COMMIT=$(git -C $SCRIPT_PATH rev-parse --short HEAD)

PAGES_DIR="gh-pages"

if [[ "$TAG" != "$GITHUB_REF" ]]; then
  if [[ "$TAG" != v* ]]; then
    echo "Unexpected tag format, skipping"
    exit 0
  fi
  echo "Tag '$TAG' pushed"
  RAW_VERSION=${TAG#v}
  FILENAME="$TAG.bin"
  jq "
    .releases += {
      \"$RAW_VERSION\": {
        \"date\": \"$(date -I)\",
        \"filename\": \"$FILENAME\",
        \"boards\": $BOARDS_JSON
      }
    }
  " $PAGES_DIR/releases.json > $PAGES_DIR/releases.json.new
  mv $PAGES_DIR/releases.json.new $PAGES_DIR/releases.json

  for BOARD in ${BOARDS[@]}; do
    cp artifacts/$BOARD/$BOARD.bin $PAGES_DIR/$BOARD/$FILENAME
  done

  COMMIT_MESSAGE="Update binaries for release $TAG"
elif [[ "$BRANCH" != "$GITHUB_REF" ]]; then
  if [[ "$BRANCH" != "master" ]]; then
    echo "Pushed branch isn't master, aborting";
    exit 0
  fi

  echo "Branch '$BRANCH' pushed: version $VERSION"
  FILENAME="nightly-$COMMIT.bin"
  jq "
    .nightly += {
      \"version\": \"$VERSION\",
      \"date\": \"$(date -I)\",
      \"filename\": \"$FILENAME\",
      \"boards\": $BOARDS_JSON
    }
  " $PAGES_DIR/releases.json > $PAGES_DIR/releases.json.new
  mv $PAGES_DIR/releases.json.new $PAGES_DIR/releases.json

  for BOARD in ${BOARDS[@]}; do
    rm $PAGES_DIR/$BOARD/nightly-*.bin
    cp artifacts/$BOARD/$BOARD.bin $PAGES_DIR/$BOARD/$FILENAME
  done

  COMMIT_MESSAGE="Update binaries for commit $COMMIT"
fi

git -C $PAGES_DIR config --local user.name "GitHub Actions"
git -C $PAGES_DIR add -A
git -C $PAGES_DIR commit -m "$COMMIT_MESSAGE"
git -C $PAGES_DIR push origin HEAD:gh-pages
