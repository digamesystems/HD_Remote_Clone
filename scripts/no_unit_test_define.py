# PlatformIO pre-script (esp32dev only).
#
# `pio test` adds the deprecated UNIT_TEST define, which makes IRremoteESP8266
# compile in host-stub mode (typedef std::string String, no real IR output).
# On-device tests need the real library, so strip UNIT_TEST right after
# PlatformIO adds it. PIO_UNIT_TESTING (the current define) is left alone.
Import("env")

from platformio.builder.tools.piotest import ConfigureTestTarget as _pio_configure


def _configure_test_target(e):
    _pio_configure(e)
    e.Replace(CPPDEFINES=[d for d in e.get("CPPDEFINES", []) if d != "UNIT_TEST"])


env.AddMethod(_configure_test_target, "ConfigureTestTarget")
