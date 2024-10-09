#include "asmart_comm_handler.h"
#include "crc16.h"

/***********************************************************************************************
 *                                Communication Process Flowchart                      *
 ***********************************************************************************************
 *
 * 1. Initialization
 *    ----------------
 *    - Function: `asmart_comm_init()`
 *      - Initializes the communication handler structure (`aSmart_Comm_Handler_t`).
 *      - Sets up UART reception using `HAL_UARTEx_ReceiveToIdle_IT()`.
 *      - Assigns the response callback function provided by the application.
 *
 * 2. Sending a Command
 *    --------------------
 *    - Function: `asmart_send_command()`
 *      - Increments the internal sequence number (wraps around at 65535).
 *      - Adds the command to the mapping table (`mapping_table`) with:
 *        - Sequence number
 *        - Command type
 *        - Timestamp for timeout management.
 *      - Assembles the message by calling `assemble_message()`:
 *        - Constructs the message according to the protocol:
 *          [STX][Length][Sequence Number][Message Type][Command Type][Payload][CRC][ETX]
 *      - Transmits the message using `HAL_UART_Transmit()`.
 *
 * 3. Sending a Response
 *    ----------------------
 *    - Function: `asmart_send_response()`
 *      - Assembles a response message to a received command.
 *      - Uses the sequence number from the received command to match the response.
 *      - Calls `assemble_message()` to construct the message.
 *      - Transmits the message.
 *
 * 4. Sending a Notification
 *    -------------------------
 *    - Function: `asmart_send_notification()`
 *      - Assembles a notification message that does not expect a response.
 *      - Sequence number is set to zero.
 *      - Calls `assemble_message()` to construct the message.
 *      - Transmits the message.
 *
 * 5. Sending an Error
 *    ---------------------
 *    - Function: `asmart_send_error()`
 *      - Assembles an error message.
 *      - If the error is in response to a command, includes the sequence number of the command.
 *      - If the error is an unsolicited error notification, sequence number is set to zero.
 *      - Calls `assemble_message()` to construct the message.
 *      - Transmits the message.
 *
 * 6. Assembling the Message
 *    -------------------------
 *    - Function: `assemble_message()`
 *      - Builds the message buffer:
 *        - Starts with STX (Start of Text).
 *        - Includes the Length field (excluding STX and ETX).
 *        - Adds the Sequence Number (2 bytes, big-endian).
 *        - Adds the Message Type (e.g., COMMAND, RESPONSE, NOTIFICATION, ERROR).
 *        - Adds the Command Type or Error Code.
 *        - Appends the Payload (message data).
 *        - Calculates and appends the CRC16-CCITT checksum.
 *        - Ends with ETX (End of Text).
 *
 * 7. UART Reception
 *    -----------------
 *    - Callback: `HAL_UARTEx_RxEventCallback()`
 *      - Triggered when data is received until an idle event occurs.
 *      - Sets the `message_ready` flag in the receive handler.
 *      - Re-initiates UART reception for the next message.
 *
 * 8. Communication Handler Loop
 *    ------------------------------
 *    - Function: `asmart_comm_handler()`
 *      - Should be called periodically in the main loop.
 *      - Checks if a message is ready to be processed:
 *        - If yes, calls `process_received_message()`.
 *      - Calls `check_command_timeouts()` to handle any command timeouts.
 *
 * 9. Processing Received Messages
 *    -------------------------------
 *    - Function: `process_received_message()`
 *      - Verifies message framing (STX and ETX) and length.
 *      - Extracts the Sequence Number, Message Type, Command Type, and Payload.
 *      - Verifies the CRC16 checksum.
 *      - Depending on the Message Type:
 *        - **MSG_TYPE_COMMAND**:
 *          - Calls the application's response callback with the message details.
 *          - The application processes the command and can send a response or error using the sequence number.
 *        - **MSG_TYPE_RESPONSE**:
 *          - Finds the corresponding command in the mapping table using the Sequence Number.
 *          - If found:
 *            - Calls the application's response callback with the message details.
 *            - Removes the command from the mapping table.
 *          - If not found:
 *            - Handles unexpected responses (optional).
 *        - **MSG_TYPE_NOTIFICATION**:
 *          - Calls the application's response callback with the message details.
 *        - **MSG_TYPE_ERROR**:
 *          - Calls the application's response callback with the message details.
 *          - If the Sequence Number is non-zero, relates to a specific command.
 *          - Removes the command from the mapping table if applicable.
 *      - Resets the receive handler for the next message.
 *
 * 10. Handling Responses and Messages in Application
 *     ------------------------------------------------
 *     - Callback Function: `response_handler()`
 *       - Implemented by the application.
 *       - Receives:
 *         - Message Type
 *         - Command Type or Error Code
 *         - Sequence Number
 *         - Payload (NULL if a timeout occurred)
 *         - Payload Length
 *       - Uses a switch-case or if-else statements to handle different message types and commands.
 *       - For **MSG_TYPE_COMMAND**:
 *         - Processes the command.
 *         - Sends a response or error using `asmart_send_response()` or `asmart_send_error()`.
 *       - For **MSG_TYPE_RESPONSE**:
 *         - Processes the response to a previously sent command.
 *       - For **MSG_TYPE_NOTIFICATION**:
 *         - Handles the notification.
 *       - For **MSG_TYPE_ERROR**:
 *         - Handles the error.
 *         - If Sequence Number is non-zero, the error relates to a specific command.
 *       - Handles timeouts or errors when payload is NULL.
 *
 * 11. Checking for Command Timeouts
 *     ---------------------------------
 *     - Function: `check_command_timeouts()`
 *       - Iterates over the mapping table.
 *       - Compares the current time with the timestamp of each command.
 *       - If the time difference exceeds `COMMAND_TIMEOUT_MS`:
 *         - Calls the response callback with `MSG_TYPE_ERROR`, passing the command type and sequence number.
 *         - Passes a NULL payload and zero length to indicate a timeout.
 *         - Removes the command from the mapping table.
 *
 * 12. CRC16 Checksum Calculation
 *     ------------------------------
 *     - Function: `crc16_ccitt()`
 *       - Calculates the CRC16-CCITT checksum over the specified data.
 *       - Used for verifying message integrity.
 *
 * 13. Error Handling
 *     ----------------
 *     - Error conditions such as CRC mismatch, framing errors, or unexpected messages are handled appropriately within the library.
 *     - The application is notified via the response callback when necessary.
 *
 * 14. Bi-directional Communication Support
 *     ---------------------------------------
 *     - Both MCUs can send commands and receive responses.
 *     - Each MCU maintains its own sequence number and mapping table.
 *     - The sequence number is included in messages to match responses to commands.
 *     - Applications can send errors in response to commands or as standalone error notifications.
 *
 ***********************************************************************************************/


/* Internal function prototypes */

/**
 * @brief Assembles and prepares a message for transmission.
 * @param comm_handler Pointer to the communication handler structure.
 * @param msg_type Type of the message (Command, Response, Notification, Error).
 * @param seq_num Sequence number of the message.
 * @param cmd_type Command or notification type.
 * @param payload Pointer to the payload data.
 * @param payload_length Length of the payload data.
 * @retval None
 */
 
 
typedef struct{
	uint8_t* buffer;
	uint16_t length;
	uint16_t index;
	uint16_t msg_length;
	uint16_t seq_num;
	 uint8_t msg_type;
	uint8_t cmd_type;
	uint16_t received_crc;
	uint16_t calculated_crc;
}aMessage_Struct_t;


 
 aSmart_Comm_Handler_t* ptr_handler;
 
static void assemble_message(aSmart_Comm_Handler_t* comm_handler, uint8_t msg_type, uint16_t seq_num, uint8_t cmd_type, uint8_t* payload, uint16_t payload_length);

/**
 * @brief Processes a received message.
 * @param comm_handler Pointer to the communication handler structure.
 * @retval None
 */
static void process_received_message(aSmart_Comm_Handler_t* comm_handler);

/**
 * @brief Adds a command to the mapping table for tracking.
 * @param comm_handler Pointer to the communication handler structure.
 * @param seq_num Sequence number of the command.
 * @param cmd_type Type of the command.
 * @retval None
 */
static void add_command_to_mapping_table(aSmart_Comm_Handler_t* comm_handler, uint16_t seq_num, uint8_t cmd_type);

/**
 * @brief Finds a command in the mapping table using the sequence number.
 * @param comm_handler Pointer to the communication handler structure.
 * @param seq_num Sequence number to search for.
 * @retval Pointer to the command entry if found, NULL otherwise.
 */
static CommandEntry_t* find_command_in_mapping_table(aSmart_Comm_Handler_t* comm_handler, uint16_t seq_num);

/**
 * @brief Removes a command from the mapping table.
 * @param comm_handler Pointer to the communication handler structure.
 * @param seq_num Sequence number of the command to remove.
 * @retval None
 */
static void remove_command_from_mapping_table(aSmart_Comm_Handler_t* comm_handler, uint16_t seq_num);

/**
 * @brief Checks for command timeouts and handles them.
 * @param comm_handler Pointer to the communication handler structure.
 * @retval None
 */
static void check_command_timeouts(aSmart_Comm_Handler_t* comm_handler);

/* Function implementations */

void asmart_comm_init(aSmart_Comm_Handler_t* comm_handler, ResponseCallback response_callback){
		ptr_handler = comm_handler;
    comm_handler->rx_handler.rxd_buffer_size = RECEIVE_BUFFER_SIZE;
    comm_handler->rx_handler.rxd_index = 0;
    comm_handler->rx_handler.message_ready = 0;
    comm_handler->sequence_number = 0;
    comm_handler->mapping_table_count = 0;
    comm_handler->response_callback = response_callback;
    memset(comm_handler->mapping_table, 0, sizeof(comm_handler->mapping_table));

    /* Hardware dependent configuration */
    HAL_UARTEx_ReceiveToIdle_IT(&COMM_UART, comm_handler->rx_handler.rxd_buffer, comm_handler->rx_handler.rxd_buffer_size);
}

void asmart_comm_handler(aSmart_Comm_Handler_t* comm_handler){
    if(comm_handler->rx_handler.message_ready){
        /* Parse and process the message */
        process_received_message(comm_handler);
        comm_handler->rx_handler.message_ready = 0;
        comm_handler->rx_handler.rxd_index = 0;
    }
    /* Check for command timeouts */
    check_command_timeouts(comm_handler);
}

void asmart_comm_send_command(aSmart_Comm_Handler_t* comm_handler, uint8_t command_type, uint8_t* payload, uint16_t payload_length){
    /* Increment and wrap sequence number */
    comm_handler->sequence_number = (comm_handler->sequence_number + 1) % 65536;

    /* Add to mapping table */
    add_command_to_mapping_table(comm_handler, comm_handler->sequence_number, command_type);

    /* Assemble message */
    assemble_message(comm_handler, MSG_TYPE_COMMAND, comm_handler->sequence_number, command_type, payload, payload_length);

    /* Transmit message */
    HAL_UART_Transmit(&COMM_UART, comm_handler->tx_handler.txd_buffer, comm_handler->tx_handler.txd_length, HAL_MAX_DELAY);
}

void asmart_comm_send_notification(aSmart_Comm_Handler_t* comm_handler, uint8_t notification_type, uint8_t* payload, uint16_t payload_length){
    /* Notifications do not require sequence numbers; set to zero */
    /* Assemble message */
    assemble_message(comm_handler, MSG_TYPE_NOTIFICATION, 0, notification_type, payload, payload_length);

    /* Transmit message */
    HAL_UART_Transmit(&COMM_UART, comm_handler->tx_handler.txd_buffer, comm_handler->tx_handler.txd_length, HAL_MAX_DELAY);
}

void asmart_comm_send_response(aSmart_Comm_Handler_t* comm_handler, uint16_t sequence_number, uint8_t command_type, uint8_t* payload, uint16_t payload_length){
    /* Assemble message */
    assemble_message(comm_handler, MSG_TYPE_RESPONSE, sequence_number, command_type, payload, payload_length);

    /* Transmit message */
    HAL_UART_Transmit(&COMM_UART, comm_handler->tx_handler.txd_buffer, comm_handler->tx_handler.txd_length, HAL_MAX_DELAY);
}

void asmart_comm_send_error(aSmart_Comm_Handler_t* comm_handler, uint16_t sequence_number, uint8_t error_code, uint8_t* payload, uint16_t payload_length){
    /* Assemble message */
    assemble_message(comm_handler, MSG_TYPE_ERROR, sequence_number, error_code, payload, payload_length);

    /* Transmit message */
    HAL_UART_Transmit(&COMM_UART, comm_handler->tx_handler.txd_buffer, comm_handler->tx_handler.txd_length, HAL_MAX_DELAY);
}

/* Internal function implementations */

static void assemble_message(aSmart_Comm_Handler_t* comm_handler, uint8_t msg_type, uint16_t seq_num, uint8_t cmd_type, uint8_t* payload, uint16_t payload_length) {
    uint8_t* buffer = comm_handler->tx_handler.txd_buffer;
    uint16_t index = 0;

    /* STX */
    buffer[index++] = STX;

    /* Placeholder for Length */
    index += 2;

    /* Sequence Number (Big Endian) */
    buffer[index++] = (seq_num >> 8) & 0xFF;
    buffer[index++] = seq_num & 0xFF;

    /* Message Type */
    buffer[index++] = msg_type;

    /* Command Type */
    buffer[index++] = cmd_type;

    /* Payload */
    memcpy(&buffer[index], payload, payload_length);
    index += payload_length;

    /* Calculate Length (excluding STX and ETX) */
    uint16_t msg_length = index - 1;  /* Exclude STX */

    /* Fill in Length (Big Endian) */
    buffer[1] = (msg_length >> 8) & 0xFF;
    buffer[2] = msg_length & 0xFF;

    /* Calculate CRC */
    uint16_t crc = crc16(&buffer[1], msg_length);

    /* Append CRC (Big Endian) */
    buffer[index++] = (crc >> 8) & 0xFF;
    buffer[index++] = crc & 0xFF;

    /* ETX */
    buffer[index++] = ETX;

    /* Total message length */
    comm_handler->tx_handler.txd_length = index;
}

static void process_received_message(aSmart_Comm_Handler_t* comm_handler) {
		aMessage_Struct_t parsing_msg;
	
    parsing_msg.buffer = comm_handler->rx_handler.rxd_buffer;
    parsing_msg.length = comm_handler->rx_handler.rxd_index;
    parsing_msg.index = 0;

    /* Check STX and ETX */
    if (parsing_msg.buffer[0] != STX || parsing_msg.buffer[parsing_msg.length - 1] != ETX) {
        /* Invalid framing */
        return;
    }

    /* Extract Length */
    parsing_msg.msg_length = (parsing_msg.buffer[1] << 8) | parsing_msg.buffer[2];

    /* Verify Length */
    if (parsing_msg.msg_length != (parsing_msg.length - 4)) {
        /* Length mismatch */
        return;
    }

    parsing_msg.index = 3;  /* Move past STX and Length */

    /* Extract Sequence Number */
    parsing_msg.seq_num = (parsing_msg.buffer[parsing_msg.index++] << 8);
    parsing_msg.seq_num |= parsing_msg.buffer[parsing_msg.index++];

    /* Extract Message Type */
    parsing_msg.msg_type = parsing_msg.buffer[parsing_msg.index++];

    /* Extract Command Type */
    parsing_msg.cmd_type = parsing_msg.buffer[parsing_msg.index++];

    /* Extract Payload */
    uint16_t payload_length = parsing_msg.msg_length - 6;  /* Exclude Sequence Number (2 bytes), Message Type (1 byte), Command Type (1 byte) */
    uint8_t payload[256];
    memcpy(payload, &parsing_msg.buffer[parsing_msg.index], payload_length);
    payload[payload_length] = '\0';

    /* Verify CRC */
    parsing_msg.received_crc = (parsing_msg.buffer[parsing_msg.length - 3] << 8) |parsing_msg. buffer[parsing_msg.length - 2];
    parsing_msg.calculated_crc = crc16(&parsing_msg.buffer[1], parsing_msg.msg_length);

    if (parsing_msg.received_crc != parsing_msg.calculated_crc) {
        /* CRC mismatch */
        return;
    }

    /* Process Message */
    if (parsing_msg.msg_type == MSG_TYPE_RESPONSE) {
        /* Find command in mapping table */
        CommandEntry_t* entry = find_command_in_mapping_table(comm_handler, parsing_msg.seq_num);
        if (entry != NULL) {
            /* Match found, call the response callback */
            if (comm_handler->response_callback) {
                comm_handler->response_callback(parsing_msg.msg_type, entry->command_type, parsing_msg.seq_num, payload, payload_length);
            }
            /* Remove from mapping table */
            remove_command_from_mapping_table(comm_handler, parsing_msg.seq_num);
        } 
				else {
            /* Sequence number not found */
            /* Handle unexpected response */
        }
    } 	
		
		else if (parsing_msg.msg_type == MSG_TYPE_COMMAND) {
        /* Handle incoming commands */
        if (comm_handler->response_callback) {
            comm_handler->response_callback(parsing_msg.msg_type, parsing_msg.cmd_type, parsing_msg.seq_num, payload, payload_length);
        }
        /* The application can now send a response or error using the sequence number */
    } 
		
		else if (parsing_msg.msg_type == MSG_TYPE_NOTIFICATION || parsing_msg.msg_type == MSG_TYPE_ERROR) {
        /* Handle notifications and errors */
        if (comm_handler->response_callback) {
            comm_handler->response_callback(parsing_msg.msg_type, parsing_msg.cmd_type, parsing_msg.seq_num, payload, payload_length);
        }
        /* For errors, if sequence number is non-zero, it relates to a command */
        if (parsing_msg.msg_type == MSG_TYPE_ERROR && parsing_msg.seq_num != 0) {
            /* Remove related command from mapping table if exists */
            remove_command_from_mapping_table(comm_handler, parsing_msg.seq_num);
        }
    }
}



static void add_command_to_mapping_table(aSmart_Comm_Handler_t* comm_handler, uint16_t seq_num, uint8_t cmd_type) {
    if (comm_handler->mapping_table_count < (sizeof(comm_handler->mapping_table) / sizeof(comm_handler->mapping_table[0]))) {
        CommandEntry_t* entry = &comm_handler->mapping_table[comm_handler->mapping_table_count++];
        entry->sequence_number = seq_num;
        entry->command_type = cmd_type;
        entry->timestamp = HAL_GetTick();
    } else {
        /* Handle mapping table full */
    }
}

static CommandEntry_t* find_command_in_mapping_table(aSmart_Comm_Handler_t* comm_handler, uint16_t seq_num) {
    for (uint8_t i = 0; i < comm_handler->mapping_table_count; i++) {
        if (comm_handler->mapping_table[i].sequence_number == seq_num) {
            return &comm_handler->mapping_table[i];
        }
    }
    return NULL;
}

static void remove_command_from_mapping_table(aSmart_Comm_Handler_t* comm_handler, uint16_t seq_num) {
    for (uint8_t i = 0; i < comm_handler->mapping_table_count; i++) {
        if (comm_handler->mapping_table[i].sequence_number == seq_num) {
            /* Shift entries to fill the gap */
            for (uint8_t j = i; j < comm_handler->mapping_table_count - 1; j++) {
                comm_handler->mapping_table[j] = comm_handler->mapping_table[j + 1];
            }
            comm_handler->mapping_table_count--;
            break;
        }
    }
}

static void check_command_timeouts(aSmart_Comm_Handler_t* comm_handler) {
    uint32_t current_time = HAL_GetTick();
    for (uint8_t i = 0; i < comm_handler->mapping_table_count; ) {
        CommandEntry_t* entry = &comm_handler->mapping_table[i];
        if (current_time - entry->timestamp > COMMAND_TIMEOUT_MS) {
            /* Handle timeout */
            if (comm_handler->response_callback) {
                /* Indicate timeout by passing NULL payload */
                comm_handler->response_callback(MSG_TYPE_ERROR, entry->command_type, entry->sequence_number, NULL, 0);
            }
            /* Remove the command from the mapping table */
            remove_command_from_mapping_table(comm_handler, entry->sequence_number);
        } 
				else {
            i++;  /* Only increment if no removal occurred */
        }
    }
}

/* UART receive callback function */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == COMM_UART.Instance) {
        ptr_handler->rx_handler.rxd_index = Size;
        ptr_handler->rx_handler.message_ready = 1;

        /* Re-initiate the reception for the next message */
        HAL_UARTEx_ReceiveToIdle_IT(&COMM_UART, ptr_handler->rx_handler.rxd_buffer, ptr_handler->rx_handler.rxd_buffer_size);
    }
}



