#!/bin/bash

# === 配置区 ===
MQTT_BROKER="xuexinglab.tech"   # 替换为你的 MQTT Broker IP
MQTT_PORT=1883
MQTT_TOPIC="config/raspberry"
MQTT_USER=""                   # 如需认证，填写用户名（留空则不用）
MQTT_PASS=""                   # 密码（留空则不用）
# === 配置结束 ===

# 获取 eth0 或 wlan0 的 IPv4 地址（排除 127.0.0.1 和 IPv6）
get_ip() {
    ip addr show eth0 | grep -oP 'inet \K[0-9.]+(?=/)' 2>/dev/null || \
    ip addr show wlan0 | grep -oP 'inet \K[0-9.]+(?=/)' 2>/dev/null || \
    echo "NoIP"
}

IP=$(get_ip)
echo "Detected IP: $IP"

# 构建 mosquitto_pub 命令
CMD="mosquitto_pub -h $MQTT_BROKER -p $MQTT_PORT -t $MQTT_TOPIC -m \"$IP\" -r -q 1"

# 添加认证（如果设置了用户名）
if [ -n "$MQTT_USER" ]; then
    CMD="$CMD -u \"$MQTT_USER\""
fi
if [ -n "$MQTT_PASS" ]; then
    CMD="$CMD -P \"$MQTT_PASS\""
fi

# 执行发布
eval $CMD
echo "Published to MQTT topic '$MQTT_TOPIC': $IP"
