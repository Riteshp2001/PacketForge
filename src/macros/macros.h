/**
 * @file macros.h
 * @brief Centralized definitions for application constants and command data.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 *
 * @description
 * This file contains string and byte sequence constants used for
 * both packet transmission and UI default values.
 */

#ifndef MACROS_H
#define MACROS_H

// --- Application Constants ---
/** Default window title and connection settings */
#define APP_TITLE "PacketForge"
#define APP_VERSION "1.0.1"
#define DEFAULT_BAUD_RATE "115200"
#define DEFAULT_IP "127.0.0.1"

#define OFFSET_ONE 1
#define MAX_INTERVAL_MS 999999
#define DEFAULT_INTERVAL_MS 1000

// --- Packet Commands (Strings) ---
#define CMD_PING        "PING"
#define CMD_HELLO       "HELLO WORLD"
#define CMD_STATUS      "STATUS?"
#define CMD_RESET       "RESET_DEVICE"
#define CMD_ACK         "ACK"
#define CMD_NACK        "NACK"
#define CMD_START       "START_OP"
#define CMD_STOP        "STOP_OP"

// --- Hex/Binary Test Patterns (String format for conversion) ---
/**
 * Hex string patterns.
 * These are detected by the macro handler and converted to binary bytes
 * before sending (e.g., "DE AD" -> [0xDE, 0xAD]).
 */
#define SEQ_TEST_1      "01 02 03 04 05"
#define SEQ_DEADBEEF    "DE AD BE EF"
#define SEQ_MAGIC       "AA 55 AA 55"
#define SEQ_ALL_FL      "FF FF FF FF"

// --- Macro Mappings (for GUI buttons) ---
/**
 * Mappings from Button ID to Data Macro.
 * Update these to change what each button sends.
 */
#define MACRO_1_DATA    CMD_PING
#define MACRO_2_DATA    CMD_STATUS
#define MACRO_3_DATA    CMD_RESET
#define MACRO_4_DATA    CMD_ACK
#define MACRO_5_DATA    CMD_NACK
#define MACRO_6_DATA    CMD_HELLO
#define MACRO_7_DATA    SEQ_TEST_1
#define MACRO_8_DATA    SEQ_DEADBEEF
#define MACRO_9_DATA    SEQ_MAGIC
#define MACRO_10_DATA   CMD_START
#define MACRO_11_DATA   CMD_STOP
#define MACRO_12_DATA   SEQ_ALL_FL

#endif // MACROS_H
