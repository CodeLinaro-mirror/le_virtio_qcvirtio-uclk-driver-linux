/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note
 */

#ifndef UCLK_IOCTL_H
#define UCLK_IOCTL_H

#include <linux/types.h>

#define UCLK_NAME_MAX_SIZE 64
#define UCLK_RATES_ARRAY_MAX_SIZE 16

/*
 * struct uclk_id - clock identifier
 *
 * struct uclk_id is deprecated but kept for backwards compatibility.
 * Addressing clocks by name is only possible reliably when each clock has an
 * unique name. There are setups in the field for which this is not the case.
 *
 * @name: [IN] target clock name
 */
struct uclk_id {
	const char name[UCLK_NAME_MAX_SIZE];
};

/*
 * struct uclk_request - clock request from userspace
 *
 * struct uclk_request is deprecated but kept for backwards compatibility
 *
 * @id: [IN] target clock identifier
 * @value: [IN/OUT] request argument/response value
 */
struct uclk_request {
	struct uclk_id id;
	__u64 value;
};
/*
 * struct uclk_info - clock information
 *
 * @id: [IN] clock identifier
 * @name: [OUT] target clock name
 * @enabled: [OUT] is clock enabled
 */
struct uclk_info {
	__u32 id;
	char name[UCLK_NAME_MAX_SIZE];
	__u8 enabled;
};

/*
 * struct uclk_rate_info - clock rates information
 *
 * @id: [IN] clock identifier
 * @num_rates: [OUT] number of clock rates
 * @rates: [OUT] array of clock rates
 */
struct uclk_rate_info {
	__u32 id;
	__u32 num_rates;
	__u64 rates[UCLK_RATES_ARRAY_MAX_SIZE];
};

/*
 * struct uclk_state - clock state change from userspace
 *
 * @id: [IN] clock identifier
 * @state: [IN] clock state
 */
struct uclk_state {
	__u32 id;
#define CLK_STATE_DISABLED 0 /* clock state is disabled */
#define CLK_STATE_ENABLED 1 /* clock state in enabled */
	__u8 state;
};

/*
 * struct uclk_rate - clock rate change from/to userspace
 *
 * @id: [IN] clock identifier
 * @rate: [IN/OUT] rate to set in clock in Hz
 */
struct uclk_rate {
	__u32 id;
	__u64 rate;
};

#define UCLKIO 'C'

/* UCLK_IOCTL_ENABLE is deprecated but kept for backwards compatibility */
#define UCLK_IOCTL_ENABLE _IOR(UCLKIO, 0x00, struct uclk_id)
/* UCLK_IOCTL_DISABLE is deprecated but kept for backwards compatibility */
#define UCLK_IOCTL_DISABLE _IOR(UCLKIO, 0x01, struct uclk_id)
/* UCLK_IOCTL_GET_RATE is deprecated but kept for backwards compatibility */
#define UCLK_IOCTL_GET_RATE _IOWR(UCLKIO, 0x02, struct uclk_request)
/* UCLK_IOCTL_SET_RATE is deprecated but kept for backwards compatibility */
#define UCLK_IOCTL_SET_RATE _IOR(UCLKIO, 0x03, struct uclk_request)
#define UCLK_IOCTL_GET_CLOCKS_COUNT _IOR(UCLKIO, 0x04, __u32)
#define UCLK_IOCTL_GET_CLOCK_INFO _IOWR(UCLKIO, 0x05, struct uclk_info)
#define UCLK_IOCTL_SET_CLOCK_STATE _IOW(UCLKIO, 0x06, struct uclk_state)
#define UCLK_IOCTL_SET_CLOCK_RATE _IOW(UCLKIO, 0x07, struct uclk_rate)
#define UCLK_IOCTL_GET_CLOCK_RATE _IOWR(UCLKIO, 0x08, struct uclk_rate)
#define UCLK_IOCTL_GET_CLOCK_RATES_INFO \
	_IOWR(UCLKIO, 0x09, struct uclk_rate_info)

#endif /* UCLK_IOCTL_H */
