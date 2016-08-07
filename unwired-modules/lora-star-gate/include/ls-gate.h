/*
 * Copyright (C) 2016 Unwired Devices
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    
 * @ingroup     
 * @brief       
 * @{
 * @file		ls-gate.h
 * @brief       LoRa-Star gateway device definitions
 * @author      Eugene Ponomarev
 */
#ifndef UNWIRED_MODULES_LORA_STAR_INCLUDE_LS_H_
#define UNWIRED_MODULES_LORA_STAR_INCLUDE_LS_H_

#include "ls-frame-fifo.h"
#include "ls-mac-types.h"
#include "ls-crypto.h"
#include "ls-gate-device-list.h"

#include "sx1276.h"

/**
 * @brief Ping timeout in seconds.
 */
#define LS_PING_TIMEOUT_S 3

/**
 * @brief Ping counter increment period in microseconds.
 */
#define LS_PING_TIMEOUT (1e6 * LS_PING_TIMEOUT_S)

/**
 * @brief Difference in ping count. If the node skipped LS_MAX_PING_DIFFERENCE pings it considered as dead and would be kicked from the network
 */
#define LS_MAX_PING_DIFFERENCE (60 * 1)

#define LS_TX_DELAY_MIN_MS 100
#define LS_TX_DELAY_MAX_MS 1000

// TODO: check LoRaWAN
#define LS_RX2_DR LS_DR3
#define LS_RX2_CH 0

/**
 * @brief RSSI of the channel considered free.
 */
#define LS_CHANNEL_FREE_RSSI -100

// TODO: optimize these values to reduce memory consumption
#define LS_SX1276_LISTENER_STACKSIZE	(2 * THREAD_STACKSIZE_DEFAULT)

/**
 * @brief LoRa-Star stack status.
 */
typedef enum {
	LS_GATE_SLEEP = 0,

	LS_GATE_TRANSMITTING,
	LS_GATE_LISTENING,

	LS_GATE_FAULT,
} ls_gate_status_t;

typedef enum {
	LS_GATE_PING = 0,
} ls_gate_tim_cmd_t;

/**
 * @brief LoRa-Star stack initialization result.
 *
 */
typedef enum {
	LS_INIT_E_SX1276_THREAD = 1,		/**< Unable to start sx1276 event handler thread */
	LS_INIT_E_TIM_THREAD = 2,			/**< Unable to start timeout handler thread */
	LS_GATE_E_NODEV = 3,				/**< Unable to send frame - device with address specified is not joined */
	LS_E_PQ_OVERFLOW = 4,				/**< Unable to queue frame for sending - queue is overflowed */

	LS_GATE_OK,							/**< Initialized successfully */
} ls_gate_init_status_t;

/**
 * @brief LoRa-Star stack settings.
 *
 * Could be stored in non-volatile memory
 */
typedef struct {
	uint64_t gate_id;			/**< Unique node ID */
	uint8_t *join_key;			/**< Join MIC key */
} ls_gate_settings_t;

/**
 * @brief Holds internal channel-related data such as transceiver handler, thread stack, etc.
 */
typedef struct {
	sx1276_t *sx1276;	/**< Transceiver instance for this channel */
	void *gate;			/**< Gate instance pointer */

    char sx1276_listener_thread_stack[LS_SX1276_LISTENER_STACKSIZE]; /**< SX1276 events listener thread stack */
    msg_t sx1276_event_queue[16];
} ls_channel_internal_t;

/**
 * @brief Holds channel-related information.
 *
 * One sx1276 transceiver per channel
 */
typedef struct {
	ls_datarate_t dr;	/**< Data rate for this channel */
	ls_channel_t ch;	/**< Channel number */

	ls_channel_internal_t _internal;	/**< Internal channel-specific data */
} ls_gate_channel_t;

#define LS_TIM_HANDLER_STACKSIZE		(1 * THREAD_STACKSIZE_DEFAULT)
#define LS_TIM_MSG_QUEUE_SIZE 10

/**
 * @brief Lora-Star gate stack internal data.
 */
typedef struct {
    uint32_t ping_count;			/**< Ping count, increments every PING_TIMEOUT us */
    xtimer_t ping_timer;			/**< Timer for periodic ping count increment */

    /* Timeout message handler data */
    char tim_thread_stack[LS_TIM_HANDLER_STACKSIZE];
	kernel_pid_t tim_thread_pid;
	msg_t tim_msg_queue[LS_TIM_MSG_QUEUE_SIZE];

	xtimer_t pending_timer;		/**< Timer for serving pending frames */
} ls_gate_internal_t;

/**
 * @briefLoRa-Star gate stack state.
 */
typedef struct {
	ls_gate_settings_t settings;		/**< Network settings, could be stored in NVRAM */
	ls_gate_status_t status;			/**< Current LS stack status */

	ls_gate_channel_t *channels;		/**< Array of channels used by this gate */
	size_t num_channels;				/**< Number of channels available */

	/* Callback functions */
	bool (*accept_node_join_cb)(uint64_t dev_id, uint64_t app_id);
	uint32_t (*node_joined_cb) (ls_gate_node_t *node);
	void (*node_kicked_cb) (ls_gate_node_t *node);
	void (*app_data_received_cb) (ls_gate_node_t *node, ls_gate_channel_t *ch, uint8_t *buf, size_t bufsize);

	void (*app_data_ack_cb)(ls_gate_node_t *node, ls_gate_channel_t *ch);
	void (*link_ok_cb)(ls_gate_node_t *node, ls_gate_channel_t *ch);

	ls_gate_devices_t devices;		/**< Devices list */

	ls_gate_internal_t _internal;
} ls_gate_t;

/**
 * @brief Initializes the internal gate structures, channels, transceivers, start listening threads.
 */
int ls_gate_init(ls_gate_t *ls);

/**
 * @brief Sends an answer to the node in channel assigned to the node.
 *
 * If the node has class A, then the frame will be queued until the node hasn't become available
 * If the node has class B, then the frame will be send as soon as possible
 */
int ls_gate_send_to(ls_gate_t *ls, ls_addr_t devaddr, uint8_t *buf, size_t bufsize);

/**
 * @brief Broadcasts a packet to all nodes and channels.
 */
int ls_gate_broadcast(ls_gate_t *ls, uint8_t *buf, size_t bufsize);

/**
 * @brief Puts gate into sleep mode.
 */
void ls_gate_sleep(ls_gate_t *ls);

#endif /* UNWIRED_MODULES_LORA_STAR_INCLUDE_LS_H_ */