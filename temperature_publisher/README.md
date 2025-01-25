# Temperature Publisher

This code publishes the temperature values from N temperature sensors to an MQTT broker every second, using Wi-Fi.  It is used as part of a project to record the temperature of my steam radiator and my living room, over time.

My specific setup uses an [Adafruit Feather M0 WiFi](https://www.adafruit.com/product/3044) with (2) [DS18B20 digital temperature sensors](https://www.adafruit.com/product/374) to publish the data over Wi-Fi to [Mosquito](https://mosquitto.org/).  Then, [Telegraf](https://www.influxdata.com/time-series-platform/telegraf/) subscribes to the topic and sends the data to an [InfluxDB database](https://www.influxdata.com/products/influxdb/), which is then read by [Grafana](https://grafana.com/oss/grafana/) to produce a time history chart of the temperatures.  Mosquito, Telegraf, InfluxDB, and Grafana, are running on a [Raspberry Pi Zero W](https://www.raspberrypi.com/products/raspberry-pi-zero-w/).

