#!/bin/sh
export ANDROID_NDK_HOME=/Users/chunchuantu/Library/Android/sdk/ndk/21.4.7075529
export NDK_ROOT=ANDROID_NDK_HOME=/Users/chunchuantu/Library/Android/sdk/ndk/21.4.7075529
export ANDROID_HOME=/Users/chunchuantu/Library/Android/sdk/
export ANDROID_PLATFORM=21
export JAVA_HOME=`/usr/libexec/java_home -v 1.8`
./gradlew :app:assembleRelease
