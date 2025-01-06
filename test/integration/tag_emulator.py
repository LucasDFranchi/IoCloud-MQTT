import json
import paho.mqtt.client as mqtt
import random
import time

from datetime import datetime


class Sector:
    _BLOCK_SIZE = 16  # The size of each block in bytes
    _BLOCKS_PER_SECTOR = (
        4  # The number of blocks per sector (including the sector trailer)
    )

    def __init__(self):
        """
        Initializes a new sector with empty blocks (initialized to 0x00).

        MIFARE Classic Tag Structure (1K example):

        +-----------------------+
        | Block 0 (Data Block 0) |  <- Sector 0, Block 0
        +-----------------------+
        | Block 1 (Data Block 1) |  <- Sector 0, Block 1
        +-----------------------+
        | Block 2 (Data Block 2) |  <- Sector 0, Block 2
        +-----------------------+
        | Block 3 (Sector Trailer)|  <- Sector 0, Block 3
        +-----------------------+

        Sector Trailer includes access control settings - Disabled.

        @note: This class is a simplified version of the MIFARE Classic sector structure, and
        don't support authentication.
        """
        self._blocks = [
            bytes([0] * self._BLOCK_SIZE) for _ in range(self._BLOCKS_PER_SECTOR)
        ]

    def write_block(self, block: int, data: bytes) -> bool:
        """
        Writes 16 bytes of data to a specific block, excluding the sector trailer.

        Args:
            block (int): The block index (0 to 2) where the data will be written.
            data (bytes): The 16-byte data to be written to the block.

        Returns:
            bool: True if the block is successfully written, False otherwise.
                - False if the data is not exactly 16 bytes or if the block index is invalid.
                - The sector trailer (block 3) is protected from write operations.
        """
        if not isinstance(data, bytes) or len(data) != self._BLOCK_SIZE:
            return False

        if 0 <= block < self._BLOCKS_PER_SECTOR - 1:
            self._blocks[block] = data
            return True

        return False

    def read_block(self, block: int) -> bytes | None:
        """
        Reads the data from a specific block.

        Args:
            block (int): The block index (0 to 3) to read data from.

        Returns:
            bytes | None: The data of the block as a bytes object, or None if the block index is invalid.
        """
        if 0 <= block < self._BLOCKS_PER_SECTOR:
            return [int(b) for b in self._blocks[block]]
        return None

    @property
    def BLOCK_SIZE(self):
        return self._BLOCK_SIZE

    @property
    def DATA_BLOCKS_PER_SECTION(self):
        return self._BLOCKS_PER_SECTOR - 1


class MifareClassic1K:
    _SECTOR_COUNT = 16

    def __init__(self):
        """
        Initializes a MIFARE Classic 1K tag with 16 sectors.

        Each sector contains 4 blocks (3 data blocks + 1 sector trailer).
        The tag is initialized with default values (0x00) for all blocks and keys.

        The memory structure of the MIFARE Classic 1K tag is as follows:

        +-----------------+-------------------------+------------------+
        | Sector Number   | Block Numbers           | Block Content    |
        +-----------------+-------------------------+------------------+
        | Sector 0        | Block 0, Block 1, Block 2, Block 3 (sector trailer) |
        | Sector 1        | Block 0, Block 1, Block 2, Block 3 (sector trailer) |
        | Sector 2        | Block 0, Block 1, Block 2, Block 3 (sector trailer) |
        | ...             | ...                     | ...              |
        | Sector 15       | Block 0, Block 1, Block 2, Block 3 (sector trailer) |
        +-----------------+-------------------------+------------------+

        - Each sector has 4 blocks, numbered 0 to 3.
        - Blocks 0 to 2 contain data, and Block 3 is reserved as the sector trailer containing keys and access control data.

        Attributes:
            _sectors (list): A list of 16 Sector objects, each representing a sector in the MIFARE Classic 1K tag.
        """
        self._sectors = [Sector() for _ in range(self._SECTOR_COUNT)]
        self._uuid = self._generate_random_number()

    def _generate_random_number(self) -> int:
        """
        Generates a random integer within a range that corresponds to a random number
        of bytes between 4 and 7. The number of bytes is chosen randomly, and the
        resulting number is within the valid range for the selected byte length.

        The function randomly selects a number of bytes (between 4 and 7), and then
        calculates the maximum value that can be represented by that number of bytes.
        It then generates a random number within that range.

        Returns:
            int: A randomly generated integer that can fit within 4 to 7 bytes.

        Example:
            # Generates a random number between 4 and 7 bytes
            random_number = _generate_random_number()
        """
        num_bytes = random.randint(4, 7)

        min_value = 0
        max_value = (1 << (num_bytes * 8)) - 1

        random_number = random.randint(min_value, max_value)

        return random_number

    def read_block(self, sector, block):
        """
        Reads a specific block of a sector from the MIFARE Classic 1K tag.

        Args:
            sector (int): The index of the sector to read from (0 to 15).
            block (int): The index of the block within the sector to read (0 to 3).

        Returns:
            bytes | None: The data of the specified block, or None if the block index is invalid.
        """
        return self._sectors[sector].read_block(block)

    def write_block(self, sector, block, data):
        """
        Writes data to a block of the specified sector, ensuring that the data is 16 elements long.
        If the data has less than 16 elements, it is padded with 0x00.

        Args:
            sector (int): The sector index to write to.
            block (int): The block index within the sector.
            data (list): The data to write, should be a list with length between 0 and 16.

        Returns:
            bool: True if the block is successfully written, False otherwise.
        """
        if len(data) < self._sectors[block].BLOCK_SIZE:
            data = data + [0] * (
                self._sectors[block].BLOCK_SIZE - len(data)
            )  # Pad with 0x00

        return self._sectors[sector].write_block(block, bytes(data))

    def dump(self):
        """
        Dumps the contents of the entire MIFARE Classic 1K tag to the console.
        This will print each block of each sector with its corresponding data.

        This method iterates over all 16 sectors and prints the data of each of the 4 blocks within each sector.
        Useful for debugging and inspecting the contents of the tag.
        """
        for sector_index, sector in enumerate(self._sectors):
            for block in range(sector._BLOCKS_PER_SECTOR):
                print(
                    f"Sector: {sector_index} Block: {block} - {sector.read_block(block)}"
                )

    @property
    def SECTOR_COUNT(self):
        return self._SECTOR_COUNT

    @property
    def uuid(self):
        return self._uuid


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
        self._tag = MifareClassic1K()  # Instantiate the MIFARE Classic 1K tag
        self._client = mqtt.Client(client_id)  # Create an MQTT client
        self._broker = broker
        self._port = port
        self._unique_device = unique_device
        self._subscribe_topic_list = [
            f"/titanium/{self._unique_device}/tag/command/config",
            f"/titanium/{self._unique_device}/tag/command/write",
        ]
        self._publish_topic_list = [
            f"/titanium/{self._unique_device}/tag/response/read",
            f"/titanium/{self._unique_device}/tag/response/write",
        ]
        # Set up callbacks for MQTT events
        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message

        # Set up the operation mode for the reader
        # self._mode = READ #ENUM
        self._requested_block = 0
        self._requested_sector = 0

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

        if topic == f"/titanium/{self._unique_device}/tag/command/config":
            command_config = json.loads(payload)
            # self._mode = command_config.get("mode", 0) #ENUM READ
            self._requested_block = command_config.get("block", 0)
            self._requested_sector = command_config.get("sector", 0)

        elif topic == f"/titanium/{self._unique_device}/tag/command/write":
            command_write = json.loads(payload)
            command_write_data = command_write.get("data", [])
            command_write_sector = command_write.get("sector", 0)
            command_write_block = command_write.get("block", 0)

            self._write_status = self._tag.write_block(
                command_write_sector, command_write_block, command_write_data
            )

            response_write_json = {
                "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                "uuid": self._tag.uuid,
                "block": command_write_block,
                "sector": command_write_sector,
                "status": self._write_status,
            }
            response_write_topic = f"/titanium/{self._unique_device}/tag/response/write"
            payload = json.dumps(response_write_json)
            self._client.publish(response_write_topic, payload)
            print(f"Published: {payload} to topic: {response_write_topic}")

    # Function to simulate publishing messages every 1 second
    def publish_response_read(self):
        """
        Simulates publishing the data read from the MifareClassic1K tag to the MQTT broker.

        The method publishes the contents of the requested block and sector to the
        specified MQTT topic every 1 second, in response to a read request.
        """
        topic = f"/titanium/{self._unique_device}/tag/response/read"
        data = self._tag.read_block(self._requested_sector, self._requested_block)
        response_read_json = {
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "uuid": self._tag.uuid,
            "block": self._requested_block,
            "sector": self._requested_sector,
            "data": data,
        }
        payload = json.dumps(response_read_json)
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


def main():
    reader_emulator = ReaderEmulator(
        "mqtt.eclipseprojects.io", 1883, "reader_emulator", "CCDBA72F0080"
    )
    reader_emulator.connect()

    try:
        while True:
            reader_emulator.publish_response_read()
            time.sleep(1)
    except KeyboardInterrupt:
        print("Disconnected from broker.")
        reader_emulator.disconnect()


if __name__ == "__main__":
    main()
