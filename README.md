# aSmart Communication Library

## Overview
The **aSmart Communication Library** is designed to handle UART-based communication between microcontrollers (MCUs) using a custom protocol. It supports bi-directional communication with sequence number-based command and response management. The library is built to assemble messages, handle UART reception, and manage command timeouts efficiently.

## Features
- Bi-directional communication between MCUs.
- Sequence number-based command and response handling.
- Error and notification message support.
- CRC16-CCITT checksum for message integrity.
- Timeout management for command processing.
- Modular architecture with application-defined callbacks.

## Communication Flow
1. **Initialization**
   - Function: `asmart_comm_init()`
   - Sets up the communication handler and UART reception.

2. **Sending a Command**
   - Function: `asmart_send_command()`
   - Assembles and sends a command message with sequence number management.

3. **Sending a Response**
   - Function: `asmart_send_response()`
   - Constructs and sends a response message based on a received command.

4. **Sending a Notification**
   - Function: `asmart_send_notification()`
   - Sends a notification message without expecting a response.

5. **Sending an Error**
   - Function: `asmart_send_error()`
   - Sends an error message, optionally tied to a specific command.

6. **Assembling the Message**
   - Function: `assemble_message()`
   - Constructs messages with the format: `[STX][Length][Sequence Number][Message Type][Command Type][Payload][CRC][ETX]`.

7. **UART Reception**
   - Callback: `HAL_UARTEx_RxEventCallback()`
   - Triggered when data is received, setting the `message_ready` flag for processing.

8. **Communication Handler Loop**
   - Function: `asmart_comm_handler()`
   - Checks for ready messages and command timeouts, processing them accordingly.

9. **Processing Received Messages**
   - Function: `process_received_message()`
   - Verifies message structure, extracts data, and calls the application's response callback.

10. **Handling Responses and Messages in Application**
    - Callback Function: `response_handler()`
    - The application implements this function to handle different message types, including commands, responses, notifications, and errors.

11. **Checking for Command Timeouts**
    - Function: `check_command_timeouts()`
    - Manages timeouts for sent commands and calls the response callback in case of timeouts.

12. **CRC16 Checksum Calculation**
    - Function: `crc16_ccitt()`
    - Verifies message integrity using CRC16-CCITT checksum.

## Error Handling
The library handles error conditions such as CRC mismatches, framing errors, and unexpected messages. The application is notified via the response callback whenever necessary.

## Bi-Directional Communication Support
Both MCUs can send commands and receive responses. Each MCU maintains its own sequence number and mapping table to track sent commands. Errors can be sent in response to commands or as standalone notifications.

## Installation
To use the **aSmart Communication Library** in your project:
1. Clone the repository:
   ```bash
   git clone https://github.com/your-repo-link.git
2. Include the Library Files in Your Project

### Usage
1. Initialize the communication handler using `asmart_comm_init()`.
2. Define your response callback in the application.
3. Use `asmart_send_command()`, `asmart_send_response()`, `asmart_send_notification()`, or `asmart_send_error()` to communicate between MCUs.
4. Regularly call `asmart_comm_handler()` in the main loop to process messages and check for timeouts.

### License
This project is licensed under the MIT License.

### Contributions
Feel free to submit issues, fork the repository, and send pull requests to improve the library.

### Contact
For questions or support, please open an issue in the GitHub repository.

