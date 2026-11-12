#!/bin/bash
set -euo pipefail
SDK="${ANDROID_HOME:?ANDROID_HOME is required}"
OUT="${1:-integration-build/helper}"
mkdir -p "$OUT/classes" "$OUT/dex"
JAR="$SDK/platforms/android-35/android.jar"
mapfile -t SOURCES < <(find integration/java -name '*.java' -type f | sort)
javac -encoding UTF-8 -source 8 -target 8 -classpath "$JAR" -d "$OUT/classes" "${SOURCES[@]}"
mapfile -t CLASSES < <(find "$OUT/classes" -name '*.class' -type f | sort)
"$SDK/build-tools/35.0.0/d8" --lib "$JAR" --min-api 26 --output "$OUT/dex" "${CLASSES[@]}"
