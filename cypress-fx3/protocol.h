/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2021 Ashwin Nair <ashwin.nair@infineon.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBSIGROK_HARDWARE_CYPRESS_FX3_PROTOCOL_H
#define LIBSIGROK_HARDWARE_CYPRESS_FX3_PROTOCOL_H

#include <stdbool.h>

#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"

#define LOG_PREFIX "cypress-fx3"

#define USB_INTERFACE		0
#define USB_CONFIGURATION	1
#define NUM_TRIGGER_STAGES	4

#define MAX_RENUM_DELAY_MS	3000
#define NUM_SIMUL_TRANSFERS	16
#define MAX_EMPTY_TRANSFERS	(NUM_SIMUL_TRANSFERS * 2)

#define NUM_CHANNELS		8  // was 16 channels

#define FX3_REQUIRED_VERSION_MAJOR	1

#define MAX_8BIT_SAMPLE_RATE	SR_MHZ(24)
#define MAX_16BIT_SAMPLE_RATE	SR_MHZ(100)
#define FX3_PIB_CLOCK			SR_MHZ(400)

/* 6 delay states of up to 256 clock ticks */
#define MAX_SAMPLE_DELAY	(6 * 256)

#define DEV_CAPS_16BIT_POS	0
#define DEV_CAPS_AX_ANALOG_POS	1

#define DEV_CAPS_16BIT		(1 << DEV_CAPS_16BIT_POS)
#define DEV_CAPS_AX_ANALOG	(1 << DEV_CAPS_AX_ANALOG_POS)

/* Protocol commands */
#define CMD_GET_FW_VERSION		    (0xb0)
#define CMD_START			        (0xb1)
#define CMD_GET_REVID_VERSION		(0xb2)

#define CMD_START_FLAGS_CLK_CTL2_POS	4
#define CMD_START_FLAGS_WIDE_POS	5
#define CMD_START_FLAGS_CLK_SRC_POS	6

#define CMD_START_FLAGS_CLK_CTL2	(1 << CMD_START_FLAGS_CLK_CTL2_POS)
#define CMD_START_FLAGS_SAMPLE_8BIT	(0 << CMD_START_FLAGS_WIDE_POS)
#define CMD_START_FLAGS_SAMPLE_16BIT	(1 << CMD_START_FLAGS_WIDE_POS)

#define CMD_START_FLAGS_CLK_30MHZ	(0 << CMD_START_FLAGS_CLK_SRC_POS)
#define CMD_START_FLAGS_CLK_48MHZ	(1 << CMD_START_FLAGS_CLK_SRC_POS)
#define CMD_START_FLAGS_CLK_100MHZ	(2 << CMD_START_FLAGS_CLK_SRC_POS)


struct cypress_fx3_profile {
	uint16_t vid;
	uint16_t pid;

	const char *vendor;
	const char *model;
	const char *model_version;

	const char *firmware;

	uint32_t dev_caps;

	const char *usb_manufacturer;
	const char *usb_product;
};

struct dev_context {
	const struct cypress_fx3_profile *profile;
	GSList *enabled_analog_channels;
	/*
	 * Since we can't keep track of an Cypress-FX3 device after upgrading
	 * the firmware (it renumerates into a different device address
	 * after the upgrade) this is like a global lock. No device will open
	 * until a proper delay after the last device was upgraded.
	 */
	int64_t fw_updated;

	const uint64_t *samplerates;
	int num_samplerates;

	uint64_t cur_samplerate;
	uint64_t limit_frames;
	uint64_t limit_samples;
	uint64_t capture_ratio;

	gboolean trigger_fired;
	gboolean acq_aborted;
	gboolean sample_wide;
	struct soft_trigger_logic *stl;

	uint64_t num_frames;
	uint64_t sent_samples;
	int submitted_transfers;
	int empty_transfer_count;

	unsigned int num_transfers;
	struct libusb_transfer **transfers;
	struct sr_context *ctx;
	void (*send_data_proc)(struct sr_dev_inst *sdi,
		uint8_t *data, size_t length, size_t sample_width);
	

	float *analog_buffer;
	size_t analog_buffer_size;

	uint8_t *logic_buffer;  
	size_t logic_buffer_size;
};



struct parsed_packet {
    uint8_t channel_type;
    uint8_t channel_number;  //  uint8_t
    uint32_t timestamp;
    unsigned int num_samples;
    float *analog_samples;           // analog
    uint16_t *digital_samples; // digital  //  uint8_t

	uint16_t ts_lo;   // <-- add this
    uint16_t ts_hi;   // <-- and this
};

int fx3driver_parse_next_packet(const uint8_t *data, size_t len, struct parsed_packet *pkt);

SR_PRIV int cypress_fx3_dev_open(struct sr_dev_inst *sdi, struct sr_dev_driver *di);
SR_PRIV struct dev_context *cypress_fx3_dev_new(void);
SR_PRIV int cypress_fx3_start_acquisition(const struct sr_dev_inst *sdi);
SR_PRIV void cypress_fx3_abort_acquisition(struct dev_context *devc);

#endif
