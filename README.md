# Install
* [nRF Connect for desktop](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop) (Segger J-Link is automatically installed in Windows).
* nRF Connect Serial Terminal (install in nRF Connect for desktop => installed in previous step).
* [nRF Connect for mobile](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile) (for Apple only possible from iOS 16).
* [Visual Studio Code](https://code.visualstudio.com/download).
* nRF Connect for VS code extension in VS Code.
* [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools/Download) (optional).

# Run application
* Set hci_ipc on cpunet core with suited .conf file (nrf5340_cpunet_iso_broadcast-bt_ll_sw_split.conf or nrf5340_cpunet_iso_receive-bt_ll_sw_split.conf) and use sysbuild to build (parent-child is depricated).
* Set iso_broadcast or iso_receive on cpuapp with prj.conf file and overlay-bt_ll_sw_split.conf as extra config file and use sysbuild to build (parent-child is depricated).

# Files
### hci_ipc
Makes communication between host and controller interface of the Bluetooth-stack on different cores possible.
### iso_broadcast
The primary purpose of this file is to demonstrate how to set up and manage Isochronous (ISO) Channels for Bluetooth audio streaming, specifically using the Broadcast Isochronous Group (BIG) feature.
### iso_receive
It is designed to receive periodic advertising from devices, create a synchronization with those periodic advertisers, and establish a broadcast isochronous group (BIG) to handle data streams.

# nRF5340 cores
### Application core (Cortex-M33)
The Application Core is designed for running complex application logic, making it suitable for tasks requiring significant processing power. Handles the main functionality of the application, such as data processing, communication, and user interface.
### Network core (Cortex-M0+)
The Network Core is specifically optimized for handling low-level network protocols and operations, particularly those related to Bluetooth Low Energy. Handles the Bluetooth stack, managing connections, advertising, scanning, and other BLE operations.

# Documentation
* [nRF Connect SDK documentation](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/index.html)
* [nRF5340](https://docs.nordicsemi.com/category/nrf5340-category)
* [nRF5340 DK](https://docs.nordicsemi.com/bundle/ug_nrf5340_dk/page/UG/dk/intro.html)
* [nRF5340 Audio DK](https://docs.nordicsemi.com/bundle/ug_nrf5340_audio/page/UG/nrf5340_audio/intro.html)

# Courses
* [nRF Connect SDK Fundamentals](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/)
* [BLE fundamentals](https://academy.nordicsemi.com/courses/bluetooth-low-energy-fundamentals/)
* [nRF Connect SDK Intermediate](https://academy.nordicsemi.com/courses/nrf-connect-sdk-intermediate/)





