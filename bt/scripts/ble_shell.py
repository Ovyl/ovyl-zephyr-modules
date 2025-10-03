#!/usr/bin/env python3
import asyncio
import sys
import tty
import termios
import os
import argparse
from bleak import BleakScanner, BleakClient

# Nordic UART Service UUIDs
NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # Write (PC -> device)
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # Notify (device -> PC)

# Timing and retry configuration
SCAN_TIMEOUT_SECONDS = 5.0
CONNECTION_TIMEOUT_SECONDS = 10.0
MAX_CONNECTION_ATTEMPTS = 3
INITIAL_PROMPT_DELAY_SECONDS = 2.0
LOG_BUFFER_DELAY_SECONDS = 2.0
INPUT_POLL_INTERVAL_SECONDS = 0.01
RETRY_DELAY_SECONDS = 2.0

# Control characters
CTRL_C = 0x03

async def run(target_name=None, log_level="inf"):
    # Scan for devices
    device = None

    if target_name:
        # If target name provided, search for it
        print(f"🔍 Scanning for '{target_name}'...")
        devices = await BleakScanner.discover(timeout=SCAN_TIMEOUT_SECONDS)

        for d in devices:
            if d.name == target_name:
                device = d
                break

        if not device:
            # Device not found, show list
            print(f"❌ Device '{target_name}' not found\n")
            named_devices = [d for d in devices if d.name]

            if not named_devices:
                print("❌ No devices with names found")
                return

            print(f"📱 Found {len(named_devices)} device(s):\n")
            for idx, d in enumerate(named_devices):
                print(f"  {idx + 1}. {d.name} ({d.address})")

            # Get user selection
            print("\nEnter device number to connect: ", end="", flush=True)
            try:
                choice = input().strip()
                selection = int(choice) - 1

                if 0 <= selection < len(named_devices):
                    device = named_devices[selection]
                else:
                    print("❌ Invalid selection")
                    return
            except (ValueError, EOFError):
                print("❌ Invalid input")
                return
    else:
        # List all devices and let user choose
        print("🔍 Scanning for BLE devices...")
        devices = await BleakScanner.discover(timeout=SCAN_TIMEOUT_SECONDS)

        # Filter devices with names
        named_devices = [d for d in devices if d.name]

        if not named_devices:
            print("❌ No devices with names found")
            return

        print(f"\n📱 Found {len(named_devices)} device(s):\n")
        for idx, d in enumerate(named_devices):
            print(f"  {idx + 1}. {d.name} ({d.address})")

        # Get user selection
        print("\nEnter device number to connect: ", end="", flush=True)
        try:
            choice = input().strip()
            selection = int(choice) - 1

            if 0 <= selection < len(named_devices):
                device = named_devices[selection]
            else:
                print("❌ Invalid selection")
                return
        except (ValueError, EOFError):
            print("❌ Invalid input")
            return

    print(f"✅ Found {device.name} ({device.address}), connecting…")

    # Retry connection up to MAX_CONNECTION_ATTEMPTS times
    for attempt in range(MAX_CONNECTION_ATTEMPTS):
        try:
            async with BleakClient(device, timeout=CONNECTION_TIMEOUT_SECONDS) as client:
                print("🔗 Connected")

                # Services/characteristics
                svcs = client.services
                print("📡 Services discovered")
                rx_char = None
                tx_char = None
                for s in svcs:
                    for c in s.characteristics:
                        if c.uuid == NUS_RX_CHAR_UUID:
                            rx_char = c
                        elif c.uuid == NUS_TX_CHAR_UUID:
                            tx_char = c
                if not (rx_char and tx_char):
                    print("❌ NUS characteristics not found")
                    return

                # Notifications (device -> PC)
                def handle_rx(_, data: bytearray):
                    text = data.decode(errors="ignore")
                    sys.stdout.write(text)
                    sys.stdout.flush()

                await client.start_notify(tx_char, handle_rx)
                print("📡 Subscribed to TX notifications\n")

                print("Raw terminal mode - all keys passed through to device")
                print("Ctrl+C to exit\n")

                # default: write without response for speed
                with_response = False

                # Send initial CR to get shell prompt
                await client.write_gatt_char(rx_char, b"\r", response=with_response)
                await asyncio.sleep(INITIAL_PROMPT_DELAY_SECONDS)  # Longer pause before enabling logs

                # Enable debug logging with longer delay to prevent buffer overflow
                log_command = f"log enable {log_level}\r"
                await client.write_gatt_char(rx_char, log_command.encode(), response=with_response)
                await asyncio.sleep(LOG_BUFFER_DELAY_SECONDS)  # Longer pause to let BLE buffers stabilize

                # Save terminal settings and switch to raw mode
                old_settings = termios.tcgetattr(sys.stdin)
                try:
                    tty.setraw(sys.stdin.fileno())

                    # Make stdin non-blocking
                    os.set_blocking(sys.stdin.fileno(), False)

                    while True:
                        # Read any available input
                        try:
                            # Read one byte at a time in raw mode
                            char = sys.stdin.read(1)
                            if char:
                                # Check for Ctrl+C
                                if ord(char) == CTRL_C:
                                    break

                                # Send the character immediately to the device
                                await client.write_gatt_char(rx_char, char.encode() if isinstance(char, str) else bytes([char]), response=with_response)
                        except (BlockingIOError, IOError):
                            # No input available
                            pass

                        # Small delay to prevent busy-waiting
                        await asyncio.sleep(INPUT_POLL_INTERVAL_SECONDS)

                finally:
                    # Restore terminal settings
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
                    print("\n\n👋 Exiting")
                return  # Success, exit retry loop

        except (asyncio.TimeoutError, Exception) as e:
            print(f"❌ Connection attempt {attempt + 1} failed: {e}")
            if attempt < MAX_CONNECTION_ATTEMPTS - 1:  # Don't sleep after last attempt
                print(f"⏳ Retrying in {RETRY_DELAY_SECONDS} seconds...")
                await asyncio.sleep(RETRY_DELAY_SECONDS)
            else:
                print("💥 All connection attempts failed")
                return

if __name__ == "__main__":
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="BLE Shell - Connect to Nordic UART Service devices")
    parser.add_argument("--device-name", help="Device name to auto-connect (optional)")
    parser.add_argument("--log-level", default="inf", help="Log level to enable (default: inf)")
    args = parser.parse_args()

    try:
        asyncio.run(run(target_name=args.device_name, log_level=args.log_level))
    except KeyboardInterrupt:
        print("\n👋 Exiting")
