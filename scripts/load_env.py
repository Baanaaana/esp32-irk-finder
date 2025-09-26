"""
PlatformIO build script to load environment variables from .env file
This script runs before the build process and loads variables from .env
"""

import os
from pathlib import Path

# Import PlatformIO environment
Import("env")

def load_dotenv():
    """Load variables from .env file into environment"""
    # Find project root directory
    project_dir = Path(env['PROJECT_DIR'])
    env_file = project_dir / '.env'

    # Check if .env file exists
    if not env_file.exists():
        print("Warning: .env file not found. Using default values from config.h")
        return

    # Parse .env file
    with open(env_file, 'r') as f:
        for line in f:
            # Skip comments and empty lines
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            # Split key=value pairs
            if '=' in line:
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip()

                # Remove quotes if present
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1]
                elif value.startswith("'") and value.endswith("'"):
                    value = value[1:-1]

                # Set as build flag
                # For string values, add quotes
                if key in ['WIFI_SSID', 'WIFI_PASSWORD', 'AP_SSID', 'AP_PASSWORD', 'BLE_DEVICE_NAME']:
                    env.Append(CPPDEFINES=[(key, f'\\"{value}\\"')])
                # For numeric/boolean values, no quotes
                else:
                    # Convert true/false to 1/0 for C++
                    if value.lower() == 'true':
                        value = '1'
                    elif value.lower() == 'false':
                        value = '0'
                    env.Append(CPPDEFINES=[(key, value)])

                print(f"Loaded from .env: {key}")

# Load environment variables
load_dotenv()