#include <jni.h>
#include "modcore.hpp"

// JNI links our own core to our own prototype, NOT to Brawl Stars.
extern "C" JNIEXPORT jstring JNICALL
Java_io_ebensos_brawlmenu_MainActivity_nativeBridgeStatus(JNIEnv* env, jclass) {
    try {
        modcore::Runtime runtime({{"Default", "MenuMusic"}}, 1);
        const auto results = runtime.applySettings();
        for (const auto& result : results) {
            if (result.status != modcore::Status::NoBridge) {
                return env->NewStringUTF("UNEXPECTED_CORE_STATE");
            }
        }
        return env->NewStringUTF("LOADED / NO_GAME_BRIDGE");
    } catch (...) {
        return env->NewStringUTF("CORE_CHECK_FAILED");
    }
}
