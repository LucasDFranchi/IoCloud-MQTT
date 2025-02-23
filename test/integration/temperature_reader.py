import json
import paho.mqtt.client as mqtt
import random
import time

from datetime import datetime

class TemperatureSensor:
    def __init__(self):
        self._temperature = 0
        
    def read_temperature(self):
        self._temperature = random.randint(0, 0x0FFF)
        return self._temperature
    
class AHT10:
    def __init__(self):
        self._internal_temperature = 0
        self._humidity = 0
        
    def read_internal_temperature(self):
        self._internal_temperature = random.randint(0, 0x0FFF)
        return self._internal_temperature
    
    def read_humidity(self):
        self._humidity = random.randint(0, 0x0FFF)
        return self._humidity

class ReaderEmulator:
    def __init__(self, broker, port, client_id, unique_device: str):
        """
        Initializes the ReaderEmulator class, setting up the MQTT client to interact
        with a MifareClassic1K tag, and establishing connections to specified topics.

        Args:
            broker (str): The address of the MQTT broker (e.g., "mqtt.eclipse.org").
            port (int): The port number for connecting to the MQTT broker (e.g., 1883).
            client_id (str): A unique client identifier for the MQTT client instance.
            unique_device (str): A unique identifier for the device, used for topic names.
        """
        self._temperature_emulator = TemperatureSensor()
        self._aht10 = AHT10()
        self._client = mqtt.Client(client_id)  # Create an MQTT client
        self._broker = broker
        self._port = port
        self._unique_device = unique_device
        self._subscribe_topic_list = [
            f"/titanium/{self._unique_device}/temperature/config",
        ]
        self._publish_topic_list = [
            f"/titanium/{self._unique_device}/temperature/response",
        ]
        # Set up callbacks for MQTT events
        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        
        self._time_interval = 5000


    def _on_connect(self, client, userdata, flags, rc):
        """
        Callback for handling successful connection to the MQTT broker.

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
        Callback for handling incoming MQTT messages.

        Args:
            client (mqtt.Client): The MQTT client instance.
            userdata (any): User data passed to the callback (unused).
            msg (mqtt.MQTTMessage): The MQTT message received, containing the topic and payload.
        """
        topic = msg.topic
        payload = msg.payload.decode()

        if topic == f"/titanium/{self._unique_device}/temperature/config":
            temperature_config = json.loads(payload)
            self._time_interval = temperature_config.get("time_interval", 0)

    def publish_temperature_response(self):
        """
        Publishes a temperature response message to the MQTT broker.

        This function gathers temperature data from the temperature emulator 
        and internal temperature/humidity from the AHT10 sensor, formats it as 
        a JSON payload, and publishes it to the designated MQTT topic.

        MQTT Topic:
            /titanium/{device_id}/temperature/response

        Data Format:
            {
                "timestamp": "YYYY-MM-DD HH:MM:SS",
                "temperature": [float, float, float, float],
                "internal_temperature": float,
                "humidity": float
            }

        Returns:
            None
        """
        topic = f"/titanium/{self._unique_device}/temperature/response"
        temperature_response_json = {
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "temperature": ([
                self._temperature_emulator.read_temperature(),
                self._temperature_emulator.read_temperature(),
                self._temperature_emulator.read_temperature(),
                self._temperature_emulator.read_temperature()
                ]),
            "internal_temperature": self._aht10.read_internal_temperature(),
            "humidity": self._aht10.read_humidity(),  
        }
        payload = json.dumps(temperature_response_json)
        self._client.publish(topic, payload)
        print(f"Published: {payload} to topic: {topic}")

    def connect(self):
        """
        Establishes a connection to the MQTT broker.

        Initiates the MQTT client connection to the broker using the provided
        address and port. The connection is maintained and the client loop is started.
        """
        self._client.connect(self._broker, self._port, 60)
        self._client.loop_start()

    def disconnect(self):
        """
        Disconnects from the MQTT broker.

        Stops the MQTT client loop and disconnects from the broker.
        """
        self._client.loop_stop()
        self._client.disconnect()
        
    @property
    def time_interval(self):
        return self._time_interval


def main():
    reader_emulator = ReaderEmulator(
        "mqtt.eclipseprojects.io", 1883, "reader_emulator", "CCDBA72F0080"
    )
    reader_emulator.connect()

    try:
        while True:
            reader_emulator.publish_temperature_response()
            time.sleep(reader_emulator.time_interval/1000)
    except KeyboardInterrupt:
        print("Disconnected from broker.")
        reader_emulator.disconnect()


if __name__ == "__main__":
    main()
