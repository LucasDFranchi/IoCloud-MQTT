import json
import paho.mqtt.client as mqtt
import random
import time


class ServerEmulator:
    def __init__(self, broker, port, client_id, unique_device: str):
        """
        Initializes the ServerEmulator, which sets up the MQTT client to interact with
        the specified MifareClassic1K tag.

        Args:
            broker (str): The address of the MQTT broker (e.g., "mqtt.eclipse.org").
            port (int): The port to connect to on the MQTT broker (e.g., 1883).
            client_id (str): A unique client identifier for the MQTT client.
            unique_device (str): A unique ID used to identify the device.
        """
        self._client = mqtt.Client(client_id)
        self._broker = broker
        self._port = port
        self._unique_device = unique_device
        self._publish_topic_list = [
            f"/titanium/{self._unique_device}/temperature/config",
        ]
        self._subscribe_topic_list = [
            f"/titanium/{self._unique_device}/temperature/response",
        ]

        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc):
        """
        Callback method to handle successful connection to the MQTT broker.

        Args:
            client (mqtt.Client): The MQTT client instance.
            userdata (any): User data passed to the callback (unused).
            flags (dict): Additional flags sent by the broker upon connection.
            rc (int): The result code indicating the connection status.
        """
        print(f"Connected with result code {rc}")
        for topic in self._subscribe_topic_list:
            self._client.subscribe(topic)

    def _on_message(self, client, userdata, msg):
        """
        Callback method to handle incoming MQTT messages.

        Args:
            client (mqtt.Client): The MQTT client instance.
            userdata (any): User data passed to the callback (unused).
            msg (mqtt.MQTTMessage): The MQTT message received, containing the topic and payload.
        """
        try:
            topic = msg.topic
            payload = msg.payload.decode()

            if topic == f"/titanium/{self._unique_device}/temperature/response":
                response_read = json.loads(payload)
                self._received_data = response_read.get("data")
                print(f"Received: {response_read} from topic: {topic}")
        except Exception as e:
            print(f"An error occurred   {e}")

    def publish_config_message(self, time_interval: int):
        """
        Simulates publishing a configuration message to the MQTT broker. This method
        sends the block and sector configuration for the MifareClassic1K tag.

        Args:
            time_interval (int): The time interval that should transmit the data.
        """
        topic = f"/titanium/{self._unique_device}/temperature/config"
        temperature_config_json = {"time_interval": time_interval}
        payload = json.dumps(temperature_config_json)
        self._client.publish(topic, payload)
        print(f"Published: {payload} to topic: {topic}")

    def connect(self):
        """
        Connects to the MQTT broker and starts the MQTT client loop to handle
        messages and events.

        This method should be called to initiate the connection to the broker.
        """
        self._client.connect(self._broker, self._port, 60)
        self._client.loop_start()

    def disconnect(self):
        """
        Disconnects from the MQTT broker and stops the MQTT client loop.

        This method should be called when you want to terminate the connection
        to the broker.
        """
        self._client.loop_stop()
        self._client.disconnect()

def main():

    server_emulator = ServerEmulator(
        "mqtt.eclipseprojects.io", 1883, "server_emulator", "1C692031BE04"
    )

    server_emulator.connect()

    try:
        server_emulator.publish_config_message(random.randint(1, 10) * 1000)
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Disconnected from broker.")
    finally:
        server_emulator.disconnect()

if __name__ == "__main__":
    main()
