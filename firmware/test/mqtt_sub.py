import random

from paho.mqtt import client as mqtt_client


broker = ""
username = ""
password = ""
port = 
topic = ""
entopic = ""
client_id = 'client-sub-test'



def connect_mqtt() -> mqtt_client:
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


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        text = msg.payload.decode()
        val = float(text.split(",")[1])
        temp = (val )*100/20
        print(f"Received  '{msg.payload.decode()}' --- `{temp}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message


def run():
    client = connect_mqtt()
    subscribe(client)
    result = client.publish(entopic, "ON")
    client.loop_forever()


if __name__ == '__main__':
    run()