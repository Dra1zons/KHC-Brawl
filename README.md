# KHC Brawl — prototype sources

**NOT a working Brawl Stars mod. NOT a complete BSD clone.**

This repository builds a separate offline Android menu prototype and our own C++ core. It does not contain the original APK, game assets, protected BSD libraries, a Brawl Stars adapter, proxy services, or paid-feature activation.

## Download an APK
1. Open the **Actions** tab.
2. Open **Android prototype APK** and the most recent successful run.
3. Download the `KHC-Brawl-prototype-debug-...` artifact at the bottom of the run page.
4. Extract its ZIP. Install `KHC-Brawl-prototype-debug.apk` on an ARM64 Android device (Android 8.0+).

If there is no run, enable Actions if prompted, then open the workflow and select **Run workflow**. A failed run has no usable APK: inspect the first failed step and its log. Do not assume that a pushed workflow built successfully.

This is a debug APK with a CI-generated test signing key. It is NOT a release build. The key may differ across runs; updating over an earlier build may require uninstalling it, deleting local prototype settings. Stable release signing will be added separately with repository secrets, never committed keys.

## What the APK does
- Opens a locally stored menu/catalog from the supplied localization.
- Supports search, categories, descriptions and saving a local settings draft.
- Loads our own arm64 JNI library and checks the C++ runtime's explicit `NO_GAME_BRIDGE` status.

**Changing a draft setting does not change Brawl Stars or configure the C++ runtime.** The web draft and runtime settings are not yet connected. The prototype has no Internet permission and does not request game credentials.

## Native core
See `native-core/README.md`. It includes settings validation/storage, theme-selection logic, frame deadline calculation, capability gating and event dispatch contracts. Unit tests use a test double, not the real game.

## CI
GitHub Actions runs the host unit tests, installs Android SDK 35 / NDK r27c / CMake 3.22.1, and builds an ARM64 debug APK using Java 17 and Gradle 8.9. The NDK configuration requests flexible/16 KB page-size compatibility. Real installation and behavior on Android 17 / OriginOS still need device testing.

No Gradle wrapper binary is committed; CI installs Gradle explicitly. To build locally, install Gradle 8.9 and the SDK/NDK versions in the workflow, then run `gradle :app:assembleDebug`.

## Remaining blocker for an actual mod
Connect our code to the chosen game client and implement a version-specific game adapter. This is not achieved by packaging this menu. No working gameplay feature, official-server compatibility, or protection from account bans is claimed.
