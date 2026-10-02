// Exercise the production Android HID merge with a host SDL virtual pad.
// stdafx.h first fixes platform dependencies before this test selects the
// Android-only merge branches in hid.cpp and hid_test.cpp.
#include <stdafx.h>
#undef LO_PLATFORM_ANDROID
#define LO_PLATFORM_ANDROID 1
#include <hid/hid.cpp>
#include "hid_test.cpp"
