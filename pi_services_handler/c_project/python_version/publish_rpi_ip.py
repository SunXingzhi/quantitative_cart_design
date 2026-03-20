#!/usr/bin/env python3
import subprocess
import re
import paho.mqtt.client as mqtt

# === 配置区 ===
MQTT_BROKER = "xuexinglab.tech"  # 替换为你的 MQTT Broker 地址，例如 "192.168.1.100"
MQTT_PORT = 1883
MQTT_TOPIC = "config/raspberry"
MQTT_USERNAME = None  # 如需认证，填写用户名
MQTT_PASSWORD = None  # 如需认证，填写密码
# === 配置结束 ===

def get_local_ip():
    """通过 ifconfig 获取 eth0 或 wlan0 的 IPv4 地址"""
    try:
        result = subprocess.run(["ifconfig"], capture_output=True, text=True, check=True)
        output = result.stdout

        # 按接口分段
        interfaces = re.split(r'\n(?=\w)', output)
        for iface in interfaces:
            if iface.startswith("eth0") or iface.startswith("wlan0"):
                # 查找 inet 地址（排除 127.0.0.1）
                match = re.search(r'inet (\d+\.\d+\.\d+\.\d+)', iface)
                if match:
                    ip = match.group(1)
                    if not ip.startswith("127."):
                        return ip
        return "No valid IP found"
    except Exception as e:
        return f"Error: {e}"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker")
    else:
        print(f"Failed to connect, return code {rc}")

def main():
    ip = get_local_ip()
    print(f"Detected IP: {ip}")

    client = mqtt.Client()
    if MQTT_USERNAME and MQTT_PASSWORD:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.on_connect = on_connect

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
        client.loop_start()
        client.publish(MQTT_TOPIC, ip, qos=1, retain=True)
        print(f"Published IP to topic '{MQTT_TOPIC}': {ip}")
        client.loop_stop()
        client.disconnect()
    except Exception as e:
        print(f"MQTT publish failed: {e}")

if __name__ == "__main__":
    main()
