#include <frontend/device.h>

#define ASSERT_INT_EQUALS(message, expected, actual) do { if ((expected) != (actual)) { logfatal("assert failed! [%s] expected %d != actual %d", message, expected, actual); } } while(0)

int main(int argc, char** argv) {
    ASSERT_INT_EQUALS("zero", 0, trim_gamepad_axis(0));

    ASSERT_INT_EQUALS("min trimmed value", -84, trim_gamepad_axis(INT16_MIN));
    ASSERT_INT_EQUALS("max trimmed value",  84, trim_gamepad_axis(INT16_MAX));
}