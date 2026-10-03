# Android AArch64 FFmpeg configuration

`config.h` was generated from the pinned Xenia FFmpeg fork at
`15ece0882e8d5875051ff5b73c5a8326f7cee9f5`, using Android NDK
`28.2.13676358` and API level 26. The fork's tracked `config.h` is a platform
dispatcher; move it aside in a disposable checkout before running `configure`.
Set `ANDROID_SDK_ROOT` to the SDK directory first.

```sh
git -C /path/to/cached/XeniaFFmpeg worktree add --detach /tmp/lo-ffmpeg-android \
  15ece0882e8d5875051ff5b73c5a8326f7cee9f5
cd /tmp/lo-ffmpeg-android
mv config.h config.h.fork
export PATH="${ANDROID_SDK_ROOT}/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin:${PATH}"
./configure \
  --cc=aarch64-linux-android26-clang \
  --cxx=aarch64-linux-android26-clang++ \
  --ld=aarch64-linux-android26-clang \
  --ar=llvm-ar --nm=llvm-nm --ranlib=llvm-ranlib --strip=llvm-strip \
  --arch=aarch64 --target-os=android --enable-cross-compile \
  --disable-everything --disable-programs --disable-all --disable-autodetect \
  --enable-avcodec --enable-avutil --enable-decoder=xmaframes
cp config.h /path/to/LostOdysseyRecomp/thirdparty/ffmpeg-config/android-aarch64/config.h
```

The enabled `xmaframes` decoder selects `wmapro`. The generated header also
records the exact configure arguments in `FFMPEG_CONFIGURATION`.
