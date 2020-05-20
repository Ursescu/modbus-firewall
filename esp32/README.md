# ESP32 - ESP-IDF modbus firewall

Clone official esp-idf sdk repo and set `IDF_PATH` environment variable
to point it.

```bash
    git clone --recursive https://github.com/espressif/esp-idf.git ~/esp-idf

    cd ~/esp-idf && ./install.sh && . ./export.sh
```

Build app and watch the info using `idf.py` script.

```bash
    idf.py flash monitor
```