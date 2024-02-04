# JiaqiWu's Strategy:
- Data transmission policy: transmit distance data only when a significant change (> 5 cm) is detected.
- Deep sleep strategy: out of range (> 50cm) for 10s will trigger deep sleep mode, sleep mode last for 30s.
- Measurement frequency: when the devices is wake-up and distance is <50cm, measure data at 1Hz.
- Data sending frequency: during measurement, when distance change >5cm, send data at 1Hz.
- Wake-up policy: after deep sleep mode, come into wake-up mode, measure distance, and connect with WIFI.

## Result:
- 1min power consumption average is 20.18mAh
- 24h estimate is about 484mAh

