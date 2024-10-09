#ifndef _ASMART_COMM_HANDLER_H_
#define _ASMART_COMM_HANDLER_H_

#include <stdint.h>
#include <string.h>
#include "usart.h"

// UART handle definition (modify according to your UART instance)
#define COMM_UART hlpuart2

// Constants for special characters
#define STX 0x02  // Start of Text
#define ETX 0x03  // End of Text

// Buffer sizes
#define RECEIVE_BUFFER_SIZE 512
#define TRANSMIT_BUFFER_SIZE 512

// Command timeout in milliseconds
#define COMMAND_TIMEOUT_MS 5000  // Adjust as needed

// Message Types
typedef enum {
    MSG_TYPE_COMMAND = 0x01,
    MSG_TYPE_RESPONSE = 0x02,
    MSG_TYPE_NOTIFICATION = 0x03,
    MSG_TYPE_ERROR = 0x04
} message_type_t;

// Command Types
typedef enum {
    COMMAND_TYPE_BEGIN_TRANSACTION = 0x10,
    COMMAND_TYPE_END_TRANSACTION = 0x11,
    // Add other command types as needed
} command_type_t;

// Command Entry Structure for Mapping Table
typedef struct {
    uint16_t sequence_number;
    uint8_t command_type;
    uint32_t timestamp;  // Time when the command was sent (ms)
} CommandEntry_t;

// Receive Handler Structure
typedef struct {
    uint8_t rxd_buffer[RECEIVE_BUFFER_SIZE];
    uint16_t rxd_buffer_size;
    uint16_t rxd_index;
    uint8_t message_ready;
} aSmart_RxHandler_t;

// Transmit Handler Structure
typedef struct {
    uint8_t txd_buffer[TRANSMIT_BUFFER_SIZE];
    uint16_t txd_length;
} aSmart_TxHandler_t;

// Response Callback Function Type
/**
 * @brief Response callback function type.
 * @param message_type Type of the message received.
 * @param command_type Type of the command or notification.
 * @param sequence_number Sequence number of the message (zero if not applicable).
 * @param payload Pointer to the payload data.
 * @param length Length of the payload data.
 */
typedef void (*ResponseCallback)(uint8_t message_type, uint8_t command_type, uint16_t sequence_number, uint8_t* payload, uint16_t length);

// Communication Handler Structure
typedef struct {
    uint16_t sequence_number;
    CommandEntry_t mapping_table[20];  // Adjust size as needed
    uint8_t mapping_table_count;
    aSmart_RxHandler_t rx_handler;
    aSmart_TxHandler_t tx_handler;
    ResponseCallback response_callback;  // Single callback for all messages
} aSmart_Comm_Handler_t;

// Function Prototypes

/**
 * @brief Initializes the communication handler.
 * @param comm_handler Pointer to the communication handler structure.
 * @param response_callback Function pointer to the response callback.
 * @retval None
 */
void asmart_comm_init(aSmart_Comm_Handler_t* comm_handler, ResponseCallback response_callback);

/**
 * @brief Handles incoming messages and timeouts. Should be called periodically.
 * @param comm_handler Pointer to the communication handler structure.
 * @retval None
 */
void asmart_comm_handler(aSmart_Comm_Handler_t* comm_handler);

/**
 * @brief Sends a command message.
 * @param comm_handler Pointer to the communication handler structure.
 * @param command_type Type of the command to send.
 * @param payload Pointer to the payload data.
 * @param payload_length Length of the payload data.
 * @retval None
 */
void asmart_comm_send_command(aSmart_Comm_Handler_t* comm_handler, uint8_t command_type, uint8_t* payload, uint16_t payload_length);

/**
 * @brief Sends a notification message.
 * @param comm_handler Pointer to the communication handler structure.
 * @param notification_type Type of the notification.
 * @param payload Pointer to the payload data.
 * @param payload_length Length of the payload data.
 * @retval None
 */
void asmart_comm_send_notification(aSmart_Comm_Handler_t* comm_handler, uint8_t notification_type, uint8_t* payload, uint16_t payload_length);

/**
 * @brief Sends a response message.
 * @param comm_handler Pointer to the communication handler structure.
 * @param sequence_number Sequence number of the original command.
 * @param command_type Type of the command being responded to.
 * @param payload Pointer to the payload data.
 * @param payload_length Length of the payload data.
 * @retval None
 */
void asmart_comm_send_response(aSmart_Comm_Handler_t* comm_handler, uint16_t sequence_number, uint8_t command_type, uint8_t* payload, uint16_t payload_length);

/**
 * @brief Sends an error message.
 * @param comm_handler Pointer to the communication handler structure.
 * @param sequence_number Sequence number of the related message (zero if not applicable).
 * @param error_code Error code to send.
 * @param payload Pointer to the payload data.
 * @param payload_length Length of the payload data.
 * @retval None
 */
void asmart_comm_send_error(aSmart_Comm_Handler_t* comm_handler, uint16_t sequence_number, uint8_t error_code, uint8_t* payload, uint16_t payload_length);

#endif // _ASMART_COMM_HANDLER_H_
