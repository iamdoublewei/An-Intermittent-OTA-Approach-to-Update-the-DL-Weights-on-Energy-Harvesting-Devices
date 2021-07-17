import asyncio
from bleak import BleakClient
import time

address = "F0:F8:F2:DA:2B:FF"
UUID = "f0001131-0451-4000-b000-000000000000"

async def run(address):
    while True:
        try:
            async with BleakClient(address) as client:
                while True:
                    await client.write_gatt_char(UUID, [0x31])
                    print(time.time())
        except Exception as e:
            print(str(time.time()) + ': Exception Caught. ')

loop = asyncio.get_event_loop()
loop.run_until_complete(run(address))