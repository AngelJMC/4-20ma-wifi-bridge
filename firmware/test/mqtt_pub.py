

import random
import time

from paho.mqtt import client as mqtt_client


broker = ""
username = ""
password = ""
port = 
topic = ""
topic2 = ""
topic3 = ""
client_id = "client-pub-test"


def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def publish(client):
    msg_count = 0
    result = client.publish(topic3, "ON")
    while True:
        time.sleep(5)
        msg = "ON" if msg_count % 2 else "OFF"
        msg1 = "OFF" if msg_count % 2 else "ON"
        result = client.publish(topic, msg)
        result = client.publish(topic2, msg1)
        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{msg}` to topic `{topic}`")
        else:
            print(f"Failed to send message to topic {topic}")
        msg_count += 1


def run():
    client = connect_mqtt()
    client.loop_start()
    publish(client)


if __name__ == '__main__':
    run()