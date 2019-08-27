#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <akubelli.h>

#define ARRAY_SIZE(x)   (sizeof(x) / (sizeof((x)[0])))

AkubelliAttr default_ioattr = {
	/* imx307 normal */
	.sensor_name = IMX307,
	.input = {
		.interface = INPUT_INTERFACE_MIPI,
		.data_type = INPUT_MIPI_DATA_TYPE_RAW12,
		.image_width = 1920,
		.image_height = 1080,
		.virtual_channel_map = VIRTUAL_CHANNEL_0,
		.blanking = {
			.sony_mipi_after_invalid_pix = 16,
			.sony_mipi_front_effective_margin = {
				.virtual_channel0_front_del = 12,
				.virtual_channel1_front_del = 0,
				.virtual_channel2_front_del = 0,
				.virtual_channel3_front_del = 0,
			},
			.sony_mipi_top_effective_margin = {
				.virtual_channel0_top_del = 8,
				.virtual_channel1_top_del = 0,
				.virtual_channel2_top_del = 0,
				.virtual_channel3_top_del = 0,
			},
		},
		.config = {
			.mipi_lane = INPUT_MIPI_2_LANE,
		},
		.sensor_frame_mode = INPUT_SENSOR_FRAME_NORMAL,
		.sensor_mode = INPUT_SENSOR_NORMAL_MODE,
	},

	.output = {
		.data_type = OUTPUT_DATA_TYPE_RAW12,
		.mode = OUTPUT_PHY2PHY_MODE,
		.clk_mode = DISCRETE_TIME,
		.mipi_lane = OUTPUT_MIPI_2_LANE,
		.virtual_channel_map = VIRTUAL_CHANNEL_0,
		.timing_h_delay = 512,
		.timing_v_delay = 2048,
	},
};

AkubelliAttr array_ioattr[] = {
	/* imx307 dol */
	{
		.sensor_name = IMX307_DOL,
		.input = {
			.interface = INPUT_INTERFACE_MIPI,
			.data_type = INPUT_MIPI_DATA_TYPE_RAW10,
			.image_width = 1920,
			.image_height = 1080,
			.virtual_channel_map = VIRTUAL_CHANNEL_0 | VIRTUAL_CHANNEL_1,
			.blanking = {
				.sony_mipi_after_invalid_pix = 16,
				.sony_mipi_front_effective_margin = {
					.virtual_channel0_front_del = 16,
					.virtual_channel1_front_del = 0,
					.virtual_channel2_front_del = 0,
					.virtual_channel3_front_del = 0,
				},
				.sony_mipi_top_effective_margin = {
					.virtual_channel0_top_del = 7,
					.virtual_channel1_top_del = 26,
					.virtual_channel2_top_del = 0,
					.virtual_channel3_top_del = 0,
				},
			},
			.config = {
				.mipi_lane = INPUT_MIPI_2_LANE,
			},
			.sensor_frame_mode = INPUT_SENSOR_FRAME_WDR_2_FRAME,
			.sensor_mode = INPUT_SENSOR_DOL_NO_VC_MODE,
		},

		.output = {
			.data_type = OUTPUT_DATA_TYPE_RAW10,
			.mode = OUTPUT_BYPASS_MODE,
			.clk_mode = DISCRETE_TIME,
			.mipi_lane = OUTPUT_MIPI_2_LANE,
			.virtual_channel_map = VIRTUAL_CHANNEL_0 | VIRTUAL_CHANNEL_1,
			.timing_h_delay = 512,
			.timing_v_delay = 2048,
		},
	},
	/* imx307 normal */
	{
		.sensor_name = IMX307,
		.input = {
			.interface = INPUT_INTERFACE_MIPI,
			.data_type = INPUT_MIPI_DATA_TYPE_RAW12,
			.image_width = 1920,
			.image_height = 1080,
			.virtual_channel_map = VIRTUAL_CHANNEL_0,
			.blanking = {
				.sony_mipi_after_invalid_pix = 16,
				.sony_mipi_front_effective_margin = {
					.virtual_channel0_front_del = 12,
					.virtual_channel1_front_del = 0,
					.virtual_channel2_front_del = 0,
					.virtual_channel3_front_del = 0,
				},
				.sony_mipi_top_effective_margin = {
					.virtual_channel0_top_del = 8,
					.virtual_channel1_top_del = 0,
					.virtual_channel2_top_del = 0,
					.virtual_channel3_top_del = 0,
				},
			},
			.config = {
				.mipi_lane = INPUT_MIPI_2_LANE,
			},
			.sensor_frame_mode = INPUT_SENSOR_FRAME_NORMAL,
			.sensor_mode = INPUT_SENSOR_NORMAL_MODE,
		},

		.output = {
			.data_type = OUTPUT_DATA_TYPE_RAW12,
			.mode = OUTPUT_PHY2PHY_MODE,
			.clk_mode = DISCRETE_TIME,
			.mipi_lane = OUTPUT_MIPI_2_LANE,
			.virtual_channel_map = VIRTUAL_CHANNEL_0,
			.timing_h_delay = 512,
			.timing_v_delay = 2048,
		},
	}
};

AkubelliAttr* CheckArgs(int argc, char *argv[])
{
	int c, i;
	int option_index = 0;
	char const *option = NULL;
	int sensor_name_magic = 0;
	AkubelliAttr *search_ioattr = NULL;
	static struct option long_options[] = {
		{"st", required_argument, 0, 0},
		{0, 0, 0, 0}
	};

	if (argc == 1)
		return &default_ioattr;

	c = getopt_long_only(argc, argv, "w:h",
			long_options, &option_index);
	if (c == -1) {
		printf("getopt_long_only error!, use the imx307 normal default ioattr!\n");
		return &default_ioattr;
	}

	switch (c) {
		case 0:
			option = long_options[option_index].name;
			if (strcmp(option, "st") == 0) {
				if (strncmp(optarg, "imx385", strlen("imx385")) == 0) {
					sensor_name_magic = IMX385;
				}
				if (strncmp(optarg, "imx307", strlen("imx307")) == 0) {
					sensor_name_magic = IMX307;
				}
				if (strncmp(optarg, "ps5270", strlen("ps5270")) == 0) {
					sensor_name_magic = PS5270;
				}
				if (strncmp(optarg, "ps5280", strlen("ps5280")) == 0) {
					sensor_name_magic = PS5280;
				}
				if (strncmp(optarg, "imx307_dol", strlen("imx307_dol")) == 0) {
					sensor_name_magic = IMX307_DOL;
				}
			} else {
				sensor_name_magic = IMX307;
			}
			break;
		default:
			break;
	}

	search_ioattr = array_ioattr;
	for (i = 0; i < ARRAY_SIZE(array_ioattr); search_ioattr++, i++) {
		if (sensor_name_magic == search_ioattr->sensor_name)
			return search_ioattr;
	}

	printf("Error: do not support this sensor, type [%s], use default imx307 normal mode\n", optarg);

	return &default_ioattr;
}

