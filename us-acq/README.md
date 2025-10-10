*** File struct ***
us-acq/
├── Makefile               # Windows-compatible build configuration
├── include/               # Header files (.h)
│   └── USBuilder.h        # Class definition for device communication
├── src/                   # Source files (.cpp)
│   ├── main.cpp           # Entry point, orchestrates acquisition flow
│   └── USBuilder.cpp      # Implements USBuilder class methods
├── build/                 # Auto-created during compilation (.o files)
├── data/                  # Generated A-scan .csv files (created at runtime)
└── us_acq.exe             # Compiled executable (Windows)

*** Build Instructions *** 
This project uses a simple GNU Makefile designed for Windows (MinGW) environments.
You’ll need a C++17-compatible compiler (e.g. MinGW g++) and make.

This will:

    - Create a /build directory (if missing)
    - Compile all .cpp files in src/ into .o files
    - Link them into a single executable: us_acq.exe

run:
    1. mingw32-make clean -- cleans/removed any us-acq.exe files
    2. mingw32-make --- create a new us-acq.exe



*** Running the Program ***

After successful compilation:
    us_acq.exe

The program will:

    - Attempt to connect to COM4 (you can change this in main.cpp).
    - Query and print the firmware version from the connected device.
    - Request an A-scan of 512 samples (8-bit each).
    - Save the collected samples into a CSV file under /data/, automatically timestamped (e.g., data/sample_2025-xx-xx_xx-xx-xx.csv).

Example console output:
    Connecting to US-Builder
    Connecting to US-Builder successfully
    GOT Firmware: 1
    FILE NAME: sample_2025-10-10_02-26-58.csv Located: "C:\path\to\us-acq\data"
    Data saved to "C:\path\to\us-acq\data\sample_2025-10-10_02-26-58.csv"
    Disconnected from US-Builder successfully



*** Code Overview ***
main.cpp

    - The entry point of the program.
    - It orchestrates the data acquisition flow:
    - Initializes a USBuilder instance on a given COM port    
    - Connects to the device
    - Requests firmware info
    - Requests and logs a single A-scan (512 samples)
    - Saves data to /data/ as a CSV
    - Gracefully disconnects
    - In short, it’s the “driver” for the acquisition process.


USBuilder.cpp / USBuilder.h

    - Connecting to a COM port
    - Sending commands to the ultrasound device
    - Receiving firmware and A-scan data
    - Writing captured samples to a CSV file with timestamping and directory management

| Method               | Description                                                                                |
| -------------------- | ------------------------------------------------------------------------------------------ |
| `connect()`          | Opens and configures the COM port for 115200 baud, 8N1 communication                       |
| `disconnect()`       | Safely closes the port                                                                     |
| `writeAll()`         | Sends an entire command buffer to the device                                               |
| `readExact()`        | Reads an exact number of bytes, handling timeouts                                          |
| `requestFirmware()`  | Sends a firmware version request and reads the single-byte firmware ID                     |
| `requestAscan8bit()` | Requests an 8-bit A-scan of `n` points (e.g. 512 samples)                                  |
| `writeCSV()`         | Saves collected samples into `data/sample_<timestamp>.csv`, creating the folder if missing |


*** Code Notes ***

1. To use a different COM port, modify this line in main.cpp:
    - USBuilder dev("\\\\.\\COM4");

2. To change the number of A-scan samples, update:
    - dev.requestAscan8bit(512, samples);

3. To store files in a different directory, edit the path in writeCSV().

