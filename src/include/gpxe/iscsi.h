#ifndef _GPXE_ISCSI_H
#define _GPXE_ISCSI_H

/** @file
 *
 * iSCSI protocol
 *
 */

#include <stdint.h>
#include <gpxe/tcp.h>
#include <gpxe/scsi.h>

/**
 * iSCSI segment lengths
 *
 * iSCSI uses an icky structure with one one-byte field (a dword
 * count) and one three-byte field (a byte count).  This structure,
 * and the accompanying macros, relieve some of the pain.
 */
union iscsi_segment_lengths {
	struct {
		/** The AHS length (measured in dwords) */
		uint8_t ahs_len;
		/** The data length (measured in bytes), in network
		 * byte order
		 */
		uint8_t data_len[3];
	} bytes;
	/** Ths data length (measured in bytes), in network byte
	 * order, with ahs_len as the first byte.
	 */
	uint32_t ahs_and_data_len;
};

/** The length of the additional header segment, in dwords */
#define ISCSI_AHS_LEN( segment_lengths ) \
	( (segment_lengths).bytes.ahs_len )

/** The length of the data segment, in bytes, excluding any padding */
#define ISCSI_DATA_LEN( segment_lengths ) \
	( ntohl ( (segment_lengths).ahs_and_data_len ) & 0xffffff )

/** The padding of the data segment, in bytes */
#define ISCSI_DATA_PAD_LEN( segment_lengths ) \
	( ( 0 - (segment_lengths).bytes.data_len[2] ) & 0x03 )

/** Set additional header and data segment lengths */
#define ISCSI_SET_LENGTHS( segment_lengths, ahs_len, data_len ) do {	\
	(segment_lengths).ahs_and_data_len =				\
		htonl ( data_len | ( ahs_len << 24 ) );			\
	} while ( 0 )

/**
 * iSCSI basic header segment common fields
 *
 */
struct iscsi_bhs_common {
	/** Opcode */
	uint8_t opcode;
	/** Flags */
	uint8_t flags;
	/** Fields specific to the PDU type */
	uint8_t other_a[2];
	/** Segment lengths */
	union iscsi_segment_lengths lengths;
	/** Fields specific to the PDU type */
	uint8_t other_b[8];
	/** Initiator Task Tag */
	uint32_t itt;
	/** Fields specific to the PDU type */
	uint8_t other_c[28];
};

/** Opcode mask */
#define ISCSI_OPCODE_MASK 0x3f

/** Immediate delivery */
#define ISCSI_FLAG_IMMEDIATE 0x40

/** Final PDU of a sequence */
#define ISCSI_FLAG_FINAL 0x80

/**
 * iSCSI login request basic header segment
 *
 */
struct iscsi_bhs_login_request {
	/** Opcode */
	uint8_t opcode;
	/** Flags */
	uint8_t flags;
	/** Maximum supported version number */
	uint8_t version_max;
	/** Minimum supported version number */
	uint8_t version_min;
	/** Segment lengths */
	union iscsi_segment_lengths lengths;
	/** Initiator session ID (IANA format) enterprise number and flags */
	uint32_t isid_iana_en;
	/** Initiator session ID (IANA format) qualifier */
	uint16_t isid_iana_qual;
	/** Target session identifying handle */
	uint16_t tsih;
	/** Initiator Task Tag */
	uint32_t itt;
	/** Connection ID */
	uint16_t cid;
	/** Reserved */
	uint16_t reserved_a;
	/** Command sequence number */
	uint32_t cmdsn;
	/** Expected status sequence number */
	uint32_t expstatsn;
	/** Reserved */
	uint8_t reserved_b[16];
};

/** Login request opcode */
#define ISCSI_OPCODE_LOGIN_REQUEST 0x03

/** Willingness to transition to next stage */
#define ISCSI_LOGIN_FLAG_TRANSITION 0x80

/** Key=value pairs continued in subsequent request */
#define ISCSI_LOGIN_FLAG_CONTINUE 0x40

/* Current stage values and mask */
#define ISCSI_LOGIN_CSG_MASK 0x0c
#define ISCSI_LOGIN_CSG_SECURITY_NEGOTIATION 0x00
#define ISCSI_LOGIN_CSG_OPERATIONAL_NEGOTIATION 0x04
#define ISCSI_LOGIN_CSG_FULL_FEATURE_PHASE 0x0c

/* Next stage values and mask */
#define ISCSI_LOGIN_NSG_MASK 0x03
#define ISCSI_LOGIN_NSG_SECURITY_NEGOTIATION 0x00
#define ISCSI_LOGIN_NSG_OPERATIONAL_NEGOTIATION 0x01
#define ISCSI_LOGIN_NSG_FULL_FEATURE_PHASE 0x03

/** ISID IANA format marker */
#define ISCSI_ISID_IANA 0x40000000

/** Fen Systems Ltd. IANA enterprise number
 *
 * Permission is hereby granted to use Fen Systems Ltd.'s IANA
 * enterprise number with this iSCSI implementation.
 */
#define IANA_EN_FEN_SYSTEMS 10019

/**
 * iSCSI login response basic header segment
 *
 */
struct iscsi_bhs_login_response {
	/** Opcode */
	uint8_t opcode;
	/** Flags */
	uint8_t flags;
	/** Maximum supported version number */
	uint8_t version_max;
	/** Minimum supported version number */
	uint8_t version_min;
	/** Segment lengths */
	union iscsi_segment_lengths lengths;
	/** Initiator session ID (IANA format) enterprise number and flags */
	uint32_t isid_iana_en;
	/** Initiator session ID (IANA format) qualifier */
	uint16_t isid_iana_qual;
	/** Target session identifying handle */
	uint16_t tsih;
	/** Initiator Task Tag */
	uint32_t itt;
	/** Reserved */
	uint32_t reserved_a;
	/** Status sequence number */
	uint32_t statsn;
	/** Expected command sequence number */
	uint32_t expcmdsn;
	/** Maximum command sequence number */
	uint32_t maxcmdsn;
	/** Status class */
	uint8_t status_class;
	/** Status detail */
	uint8_t status_detail;
	/** Reserved */
	uint8_t reserved_b[10];
};

/** Login response opcode */
#define ISCSI_OPCODE_LOGIN_RESPONSE 0x23

/**
 * iSCSI SCSI command basic header segment
 *
 */
struct iscsi_bhs_scsi_command {
	/** Opcode */
	uint8_t opcode;
	/** Flags */
	uint8_t flags;
	/** Reserved */
	uint16_t reserved_a;
	/** Segment lengths */
	union iscsi_segment_lengths lengths;
	/** SCSI Logical Unit Number */
	uint8_t lun[8];
	/** Initiator Task Tag */
	uint32_t itt;
	/** Expected data transfer length */
	uint32_t exp_len;
	/** Command sequence number */
	uint32_t cmdsn;
	/** Expected status sequence number */
	uint32_t expstatsn;
	/** SCSI Command Descriptor Block (CDB) */
	union scsi_cdb cdb;
};

/** SCSI command opcode */
#define ISCSI_OPCODE_SCSI_COMMAND 0x01

/** Command will read data */
#define ISCSI_COMMAND_FLAG_READ 0x40

/** Command will write data */
#define ISCSI_COMMAND_FLAG_WRITE 0x20

/* Task attributes */
#define ISCSI_COMMAND_ATTR_UNTAGGED 0x00
#define ISCSI_COMMAND_ATTR_SIMPLE 0x01
#define ISCSI_COMMAND_ATTR_ORDERED 0x02
#define ISCSI_COMMAND_ATTR_HEAD_OF_QUEUE 0x03
#define ISCSI_COMMAND_ATTR_ACA 0x04

/**
 * iSCSI SCSI response basic header segment
 *
 */
struct iscsi_bhs_scsi_response {
	/** Opcode */
	uint8_t opcode;
	/** Flags */
	uint8_t flags;
	/** Response code */
	uint8_t response;
	/** SCSI status code */
	uint8_t status;
	/** Segment lengths */
	union iscsi_segment_lengths lengths;
	/** Reserved */
	uint8_t reserved_a[8];
	/** Initiator Task Tag */
	uint32_t itt;
	/** SNACK tag */
	uint32_t snack;
	/** Status sequence number */
	uint32_t statsn;
	/** Expected command sequence number */
	uint32_t expcmdsn;
	/** Maximum command sequence number */
	uint32_t maxcmdsn;
	/** Expected data sequence number */
	uint32_t expdatasn;
	/** Reserved */
	uint8_t reserved_b[8];
};

/** SCSI response opcode */
#define ISCSI_OPCODE_SCSI_RESPONSE 0x21

/** SCSI command completed at target */
#define ISCSI_RESPONSE_COMMAND_COMPLETE 0x00

/** SCSI target failure */
#define ISCSI_RESPONSE_TARGET_FAILURE 0x01

/**
 * iSCSI data in basic header segment
 *
 */
struct iscsi_bhs_data_in {
	/** Opcode */
	uint8_t opcode;
	/** Flags */
	uint8_t flags;
	/** Reserved */
	uint8_t reserved_a;
	/** SCSI status code */
	uint8_t status;
	/** Segment lengths */
	union iscsi_segment_lengths lengths;
	/** Logical Unit Number */
	uint8_t lun[8];
	/** Initiator Task Tag */
	uint32_t itt;
	/** Target Transfer Tag */
	uint32_t ttt;
	/** Status sequence number */
	uint32_t statsn;
	/** Expected command sequence number */
	uint32_t expcmdsn;
	/** Maximum command sequence number */
	uint32_t maxcmdsn;
	/** Data sequence number */
	uint32_t datasn;
	/** Buffer offset */
	uint32_t offset;
	/** Residual count */
	uint32_t residual_count;
};

/** Data in opcode */
#define ISCSI_OPCODE_DATA_IN 0x25

/** Data requires acknowledgement */
#define ISCSI_DATA_FLAG_ACKNOWLEDGE 0x40

/** Data overflow occurred */
#define ISCSI_DATA_FLAG_OVERFLOW 0x04

/** Data underflow occurred */
#define ISCSI_DATA_FLAG_UNDERFLOW 0x02

/** SCSI status code and verflow/underflow flags are valid */
#define ISCSI_DATA_FLAG_STATUS 0x01

/**
 * An iSCSI basic header segment
 */
union iscsi_bhs {
	struct iscsi_bhs_common common;
	struct iscsi_bhs_login_request login_request;
	struct iscsi_bhs_login_response login_response;
	struct iscsi_bhs_scsi_command scsi_command;
	struct iscsi_bhs_scsi_response scsi_response;
	struct iscsi_bhs_data_in data_in;
	unsigned char bytes[ sizeof ( struct iscsi_bhs_common ) ];
};

/** State */
enum iscsi_state {
	/** In the process of logging in */
	ISCSI_STATE_FAILED = -1,
	ISCSI_STATE_NOT_CONNECTED = 0,
	ISCSI_STATE_IDLE,
	ISCSI_STATE_LOGGING_IN,
	ISCSI_STATE_READING_DATA,
};

/** State of an iSCSI TX engine */
enum iscsi_tx_state {
	/** Nothing to send */
	ISCSI_TX_IDLE = 0,
	/** Sending the basic header segment */
	ISCSI_TX_BHS,
	/** Sending the additional header segment */
	ISCSI_TX_AHS,
	/** Sending the data segment */
	ISCSI_TX_DATA,
	/** Sending the data segment padding */
	ISCSI_TX_DATA_PADDING,
};

/** State of an iSCSI RX engine */
enum iscsi_rx_state {
	/** Receiving the basic header segment */
	ISCSI_RX_BHS = 0,
	/** Receiving the additional header segment */
	ISCSI_RX_AHS,
	/** Receiving the data segment */
	ISCSI_RX_DATA,
	/** Receiving the data segment padding */
	ISCSI_RX_DATA_PADDING,
};

/** An iSCSI session */
struct iscsi_session {
	/** TCP connection for this session */
	struct tcp_connection tcp;

	/** Initiator IQN */
	const char *initiator;
	/** Target IQN */
	const char *target;

	/** Block size in bytes */
	size_t block_size;
	/** Starting block number of the current data transfer */
	unsigned long block_start;
	/** Block count of the current data transfer */
	unsigned long block_count;
	/** Block read callback function
	 *
	 * Note that this may be called several times, since it is
	 * called per-packet rather than per-block.
	 */
	void ( * block_read_callback ) ( void *private, const void *data,
					 unsigned long offset, size_t len );
	/** Block read callback private data
	 *
	 * This is passed to block_read_callback()
	 */
	void *block_read_private;

	/** State of the session */
	enum iscsi_state state;
	/** Target session identifying handle
	 *
	 * This is assigned by the target when we first log in, and
	 * must be reused on subsequent login attempts.
	 */
	uint16_t tsih;

	/** Initiator task tag
	 *
	 * This is the tag of the current command.  It is incremented
	 * whenever a final response PDU is received.
	 */
	uint32_t itt;
	/** Command sequence number
	 *
	 * This is the sequence number of the current command, used to
	 * fill out the CmdSN field in iSCSI request PDUs.  It is
	 * updated with the value of the ExpCmdSN field whenever we
	 * receive an iSCSI response PDU containing such a field.
	 */
	uint32_t cmdsn;
	/** Status sequence number
	 *
	 * This is the most recent status sequence number present in
	 * the StatSN field of an iSCSI response PDU containing such a
	 * field.  Whenever we send an iSCSI request PDU, we fill out
	 * the ExpStatSN field with this value plus one.
	 */
	uint32_t statsn;
	
	/** Basic header segment for current TX PDU */
	union iscsi_bhs tx_bhs;
	/** State of the TX engine */
	enum iscsi_tx_state tx_state;
	/** Byte offset within the current TX state */
	size_t tx_offset;

	/** Basic header segment for current RX PDU */
	union iscsi_bhs rx_bhs;
	/** State of the RX engine */
	enum iscsi_rx_state rx_state;
	/** Byte offset within the current RX state */
	size_t rx_offset;
};

static inline int iscsi_busy ( struct iscsi_session *iscsi ) {
	return ( iscsi->state > ISCSI_STATE_IDLE );
}

static inline int iscsi_error ( struct iscsi_session *iscsi ) {
	return ( iscsi->state == ISCSI_STATE_FAILED );
}

#endif /* _GPXE_ISCSI_H */