import asyncio
from bleak import BleakScanner, BleakClient
import json

DEVICE_NAME = "E3K_3"
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

def notification_handler(sender, data):
    """Handle incoming notifications."""
    try:
        # Decode the bytes to a string and parse the JSON
        json_data = json.loads(data.decode())
        print("Received data:")
        print(json.dumps(json_data, indent=2))
    except json.JSONDecodeError:
        print(f"Received non-JSON data: {data}")

async def run_ble_client(address):
    async with BleakClient(address) as client:
        print(f"Connected: {client.is_connected}")

        # Subscribe to the characteristic
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

        print("Listening for notifications, press Ctrl+C to stop...")
        
        # Keep the connection alive
        while True:
            await asyncio.sleep(1)

async def scan_and_connect():
    device = await BleakScanner.find_device_by_name(DEVICE_NAME)
    
    if device is None:
        print(f"Could not find device with name '{DEVICE_NAME}'")
        return

    print(f"Found device: {device.name}")
    print(f"Address: {device.address}")
    
    await run_ble_client(device.address)

if __name__ == "__main__":
    asyncio.run(scan_and_connect())