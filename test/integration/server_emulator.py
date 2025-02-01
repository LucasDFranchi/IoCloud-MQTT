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
            f"/titanium/{self._unique_device}/tag/command/config",
            f"/titanium/{self._unique_device}/tag/command/write",
        ]
        self._subscribe_topic_list = [
            f"/titanium/{self._unique_device}/tag/response/read",
            f"/titanium/{self._unique_device}/tag/response/write",
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
        topic = msg.topic
        payload = msg.payload.decode()

        if topic == f"/titanium/{self._unique_device}/tag/response/read":
            response_read = json.loads(payload)
            self._wait = False
            self._received_data = response_read.get("data")
            print(f"Received: {response_read} from topic: {topic}")

        elif topic == f"/titanium/{self._unique_device}/tag/response/write":
            command_write = json.loads(payload)
            print(f"Received: {command_write} from topic: {topic}")

    def publish_config_message(self, sector: int, block: int):
        """
        Simulates publishing a configuration message to the MQTT broker. This method
        sends the block and sector configuration for the MifareClassic1K tag.

        Args:
            sector (int): The sector to configure.
            block (int): The block within the sector to configure.
        """
        topic = f"/titanium/{self._unique_device}/tag/command/config"
        command_config_json = {"block": block, "sector": sector}
        payload = json.dumps(command_config_json)
        self._client.publish(topic, payload)
        self._wait = True
        print(f"Published: {payload} to topic: {topic}")

    def publish_write_message(self, sector: int, block: int, data: list):
        """
        Simulates publishing a write message to the MQTT broker. This method sends
        the block, sector, and data to be written to the MifareClassic1K tag.

        Args:
            sector (int): The sector to write to.
            block (int): The block within the sector to write to.
            data (list): A list of data to be written to the block.
        """
        topic = f"/titanium/{self._unique_device}/tag/command/write"
        command_write_json = {"block": block, "sector": sector, "data": data}
        payload = json.dumps(command_write_json)
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

    def server_wait_on_receive(self):
        """
        Waits for a response to be received from the server. This method pauses
        execution until a response is received (via MQTT) to proceed.

        This method is typically used after publishing a configuration or write
        message to ensure the server has time to respond.
        """
        while self._wait:
            time.sleep(1)


def main():
    _NUM_OF_SECTORS = 16
    _NUM_OF_DATA_BLOCKS = 3
    _BLOCK_SIZE = 16

    server_emulator = ServerEmulator(
        "mqtt.eclipseprojects.io", 1883, "server_emulator", "CCDBA72F0080"
    )

    server_emulator.connect()

    try:
        while True:
            for sector in range(_NUM_OF_SECTORS):
                for block in range(_NUM_OF_DATA_BLOCKS):
                    server_emulator.publish_write_message(
                        sector,
                        block,
                        [random.randint(0, 255) for _ in range(_BLOCK_SIZE)],
                    )
                    time.sleep(0.1)
                    server_emulator.publish_config_message(sector, block)
                    server_emulator.server_wait_on_receive()
    except KeyboardInterrupt:
        print("Disconnected from broker.")
        server_emulator.disconnect()


if __name__ == "__main__":
    main()
