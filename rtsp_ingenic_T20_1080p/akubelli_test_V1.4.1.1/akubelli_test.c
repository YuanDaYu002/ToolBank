#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <akubelli.h>
#include <akub_nemer.h>

#include <RTSPServer.hh>
#include <imp/imp_osd.h>
#include <imp/imp_isp.h>

#define TAG "akubelli_test"
#define IOCTL_SEND_USER_PID   0x111
#define DEV_T30_ISP     "/dev/tx-isp"

#define RAW_IMAGE_PATH        "/tmp/image.raw"
#define RAW_MD5_PATH          "/tmp/image.raw.md5"
#define YUV_IMAGE_PATH        "/tmp/image.yuv"
#define YUV_MD5_PATH          "/tmp/image.yuv.md5"
#define DEBUG_RAW_CAPTURE    0
#define DEBUG_YUV_CAPTURE    0
#define MAX_IVS_OSD_REGION      400
#define MAX_IVS_ID_OSD_REGION   100

#define FFVA_PATH "/mnt/ffvA.feat"
#define FFVB_PATH "/mnt/ffvB.feat"
#define FFVC_PATH "/mnt/ffvC.feat"

/* external funtion switch */
#define PRINT_RESULT            0
#define PRINT_ATTRIBUTE         0
#define MODIFY_CNN_USER         0
#define REMOVE_MD_FPS_PRINTING  0
#define CHECK_VERSION           0
#define FACE_RECOG              0
#define CONTROL_FACE_RECOG      0
#define CONTROL_PUSH_IMAGE      0
#define NEMER_TEST				0

unsigned char *osd_data;
IMPRgnHandle ivsRgnHandler[MAX_IVS_OSD_REGION + MAX_IVS_ID_OSD_REGION] = {INVHANDLE};
extern int drawLineRectShow(int index, int x0, int y0, int x1, int y1, int color, int dir, unsigned long long ts);
extern int drawPicRectShow(int index, int id, int x0, int y0, unsigned long long ts);
extern int host_get_isp_EV_attr(ISP_EV_Attr *ev_attr);
extern AkubelliAttr* CheckArgs(int argc, char *argv[]);
IMPRgnHandle handler;
static int osd_index = 0;
static int id_osd_index = 0;
static unsigned int last_time_osd_index = 0;
static unsigned int last_time_id_osd_index = 0;
unsigned int frame_index;

struct timeval time_val;
unsigned long long time_last;
unsigned int time_id;
static int deinit_flag = 0;

static unsigned int osd_clear_threshold = 0;

static void auto_deinit(void)
{
	int ret = -1;
	if (!deinit_flag) {
		deinit_flag++;


		ret = Akubelli_Stop();
		if (ret) {
			printf("ERROR: Akubelli_Stop failed.\n");
			return;
		}

		ret = Akubelli_DeInit();
		if (ret) {
			printf("ERROR: Akubelli_DeInit failed.\n");
			return;
		}
	}

	return;
}

static void signal_handler(int signal)
{
	switch (signal) {
	case SIGINT:  /* cutout by ctrl + c */
	case SIGTSTP: /* cutout by ctrl + z */
	case SIGKILL:
	case SIGTERM:
	default:
		auto_deinit();
		exit(0);
		break;
	}
}

static void signal_init(void)
{
	signal(SIGINT,  signal_handler);
	signal(SIGTSTP, signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGTERM, signal_handler);
}

static inline void print_mddesc(AK_MetaDatadDesc *md_desc)
{
	printf("Frame : %d\n", md_desc->frame);
	printf("\terror     : 0x%08x\n", md_desc->error);
	printf("\tts        : %llu\n", md_desc->ts);
	printf("\tobjnum    : %d\n", md_desc->objnum);
	printf("\tjpeg_size : %d\n", md_desc->jpeg_size);
	printf("\t_private  : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		   md_desc->_private[0], md_desc->_private[1], md_desc->_private[2], md_desc->_private[3],
		   md_desc->_private[4], md_desc->_private[5], md_desc->_private[6], md_desc->_private[7],
		   md_desc->_private[8], md_desc->_private[9], md_desc->_private[10], md_desc->_private[11],
		   md_desc->_private[12], md_desc->_private[13], md_desc->_private[14], md_desc->_private[15]);
}

static inline void print_objdesc(AK_ObjDesc *obj_desc)
{
	char *prefix = "\t\t";
	printf("%stype : %d\n", prefix, obj_desc->type);
}

static inline void print_faceattr(FaceAttr *face_attr, unsigned long long ts)
{
#if PRINT_RESULT
	printf("### face attr ###\n### x = %d, y = %d, width = %d, height = %d ###\n### id = %d ###\n### pitch = %d, yaw = %d, roll = %d ###\n\n",
			face_attr->base_attr.box.x, face_attr->base_attr.box.y, face_attr->base_attr.box.width, face_attr->base_attr.box.height,
			face_attr->base_attr.track_id, face_attr->pitch, face_attr->yaw, face_attr->roll);
#endif
#if PRINT_ATTRIBUTE
	char *man = "男";
	char *women = "女";
#if 0
	if (face_attr->base_attr.track_push) {
		printf("### face attr ###\n### blur = %d ###\n### yaw = %d, pitch = %d, roll = %d ###\n\n", face_attr->blur, face_attr->yaw * 57 / 100, face_attr->pitch * 57 / 100, face_attr->roll * 57 /100);
		printf("### face detailed attr ###\n### age = %d, gender = %d ###\n### post_filter_score = %d ###\n### landmark_nose_x = %d, landmark_nose_y = %d ###\n### feature_valid = %d ###\n\n",
				face_attr->age, face_attr->gender, face_attr->post_filter_score, face_attr->face_landmark.nose_tip_x, face_attr->face_landmark.nose_tip_y, face_attr->feature_valid);
	}
#else
	if (face_attr->base_attr.track_push == 1) {
		printf("### face detailed attr ###\n### gender : %s ###\n### age : %d ###\n\n",
				face_attr->gender <= 50 ? man : women, face_attr->age);
	}
#endif
#endif
	if (osd_index <= MAX_IVS_OSD_REGION) {
		BaseAttr *base_attr = &face_attr->base_attr;
		ObjBox *box = &base_attr->box;
		handler = ivsRgnHandler[osd_index++];

		/* if is track_push, skip it! */
		if ( !face_attr->base_attr.track_push ) {
			drawLineRectShow(handler, box->x, box->y, box->x + box->width, box->y + box->height, OSD_RED, OSD_REG_RECT, ts);
		}
	}
}

static inline void print_humanattr(HumanAttr *human_attr, unsigned long long ts)
{
#if PRINT_RESULT
	printf("### human attr ###\n### x = %d, y = %d, width = %d, height = %d ###\n### id = %d ###\n\n",
			human_attr->base_attr.box.x, human_attr->base_attr.box.y, human_attr->base_attr.box.width, human_attr->base_attr.box.height,
			human_attr->base_attr.track_id);
#endif
#if PRINT_ATTRIBUTE
	char *man = "男";
	char *women = "女";
	if (human_attr->base_attr.track_push == 1) {
		printf("### human detailed attr ###\n### gender = %s ###\n### age = %d ###\n### ride_bike = %d ###\n\n",
				human_attr->gender <= 50 ? man : women, human_attr->age, human_attr->ride_bike);
	}
#endif
	if (osd_index <= MAX_IVS_OSD_REGION) {
		BaseAttr *base_attr = &human_attr->base_attr;
		ObjBox *box = &base_attr->box;
		handler = ivsRgnHandler[osd_index++];

		/* if is track_push, skip it! */
		if ( !human_attr->base_attr.track_push ) {
			drawLineRectShow(handler, box->x, box->y, box->x + box->width, box->y + box->height, OSD_GREEN, OSD_REG_RECT, ts);
		}
		/* draw id */
		if (id_osd_index <= MAX_IVS_ID_OSD_REGION) {
			handler = ivsRgnHandler[MAX_IVS_OSD_REGION + id_osd_index++];
			/* if is track_push, skip it! */
			if ( !human_attr->base_attr.track_push ) {
				drawPicRectShow(handler, base_attr->track_id, box->x, box->y, ts);
			}
		}
	}
}

static inline void print_plateattr(PlateAttr *plate_attr, unsigned long long ts, obj_context_t obj_context)
{
#if PRINT_RESULT
	printf("### plate attr ###\n### x = %d, y = %d, width = %d, height = %d ###\n### id = %d ###\n\n",
			plate_attr->base_attr.box.x, plate_attr->base_attr.box.y, plate_attr->base_attr.box.width, plate_attr->base_attr.box.height,
			plate_attr->base_attr.track_id);
#endif
#if PRINT_ATTRIBUTE
#if 1
	if (plate_attr->base_attr.track_push) {
		/*printf("### plate attr ###\n### roll = %d ###\n### quality = %d ###\n\n", plate_attr->roll * 57 / 100, plate_attr->quality);*/
		printf("### plate detailed attr ###\n### color = %d, confidence = %d ###\n### text = %s, confidence = %d, tokens_cnt = %d ###\n\n", plate_attr->plate_color.color_type, plate_attr->plate_color.color_confidence,
				plate_attr->plate_text[0].str_utf8, plate_attr->plate_text[0].confidence, plate_attr->plate_text[0].tokens_cnt);
	}
#else
	if (plate_attr->base_attr.track_push == 1) {
		PlateTextpredict plate_txt_prdt;
		memset(&plate_txt_prdt, 0x0, sizeof(PlateTextpredict));
		memset(plate_txt_prdt.str_utf8, 0x0, 32);
		plate_context_t plate_ctx = Akubelli_AIEngine_Get_PlateTxtPrdt(obj_context, 0, &plate_txt_prdt);
		if (plate_ctx) {
			printf("### plate detailed attr ###\n### plate number : [%s], confidence : %d, tokens_cnt : %d  ###\n\n", plate_txt_prdt.str_utf8, plate_txt_prdt.confidence, plate_txt_prdt.tokens_cnt);
		}
	}
#endif
#endif
	if (osd_index <= MAX_IVS_OSD_REGION) {
		BaseAttr *base_attr = &plate_attr->base_attr;
		ObjBox *box = &base_attr->box;
		handler = ivsRgnHandler[osd_index++];
		/* if is track_push, skip it! */
		if ( !plate_attr->base_attr.track_push ) {
			drawLineRectShow(handler, box->x, box->y, box->x + box->width, box->y + box->height, OSD_RED, OSD_REG_RECT, ts);
		}
	}
}

static inline void print_vehicleattr(VehicleAttr *vehicle_attr, unsigned long long ts)
{
#if PRINT_RESULT
	printf("### vehicle attr ###\n### x = %d, y = %d, width = %d, height = %d ###\n### id = %d ###\n\n",
			vehicle_attr->base_attr.box.x, vehicle_attr->base_attr.box.y, vehicle_attr->base_attr.box.width,
			vehicle_attr->base_attr.box.height, vehicle_attr->base_attr.track_id);
#endif
#if PRINT_ATTRIBUTE
	if (vehicle_attr->base_attr.track_push == 1) {
		char *color = NULL;
		char *type = NULL;
		switch (vehicle_attr->veh_color_array[0].vehicle_color) {
		case VEH_BLUE:
			color = "蓝色";
			break;
		case VEH_YELLOW:
			color = "黄色";
			break;
		case VEH_BLACK:
			color = "黑色";
			break;
		case VEH_WHITE:
			color = "白色";
			break;
		case VEH_GREEN:
			color = "绿色";
			break;
		case VEH_RED:
			color = "红色";
			break;
		case VEH_GRAY:
			color = "灰色";
			break;
		case VEH_PURPLE:
			color = "紫色";
			break;
		case VEH_PINK:
			color = "粉色";
			break;
		case VEH_BROWN:
			color = "棕色";
			break;
		case VEH_CYAN:
			color = "蓝绿色";
			break;
		case VEH_COLORFUL:
			color = "多彩色";
			break;
		default:
			color = "未知";
			break;
		}
		switch (vehicle_attr->veh_type_array[0].vehicle_type) {
		case VEH_CAR:
			type = "轿车";
			break;
		case VEH_SUV:
			type = "SUV";
			break;
		case VEH_MICROBUS:
			type = "客车";
			break;
		case VEH_MINIBUS:
			type = "面包车";
			break;
		case VEH_BUS:
			type = "大巴车";
			break;
		case VEH_PICKUP:
			type = "皮卡";
			break;
		case VEH_TRUCK:
			type = "卡车";
			break;
		default:
			type = "未知";
			break;
		}
		printf("### Vehicle detailed attr ###\n### color : [%s] , confidence : %d ###\n### type : [%s], confidence = %d ###\n### trademark : [%s], confidence = %d ###\n### facing = %d, confidence = %d ###\n\n",
				color, vehicle_attr->veh_color_array[0].color_confidence,
				type, vehicle_attr->veh_type_array[0].type_confidence,
				vehicle_attr->trademark_utf8, vehicle_attr->trademark_utf8_confidence,
				vehicle_attr->vehicle_facing, vehicle_attr->vehicle_facing_confidence);

	}
#endif
	if (osd_index <= MAX_IVS_OSD_REGION) {
		BaseAttr *base_attr = &vehicle_attr->base_attr;
		ObjBox *box = &base_attr->box;
		handler = ivsRgnHandler[osd_index++];
		/* if is track_push, skip it! */
		if ( !vehicle_attr->base_attr.track_push ) {
			drawLineRectShow(handler, box->x, box->y, box->x + box->width, box->y + box->height, OSD_GREEN, OSD_REG_RECT, ts);
		}
		/* draw id */
		if (id_osd_index <= MAX_IVS_ID_OSD_REGION) {
			handler = ivsRgnHandler[MAX_IVS_OSD_REGION + id_osd_index++];
			/* if is track_push, skip it! */
			if ( !vehicle_attr->base_attr.track_push ) {
				drawPicRectShow(handler, base_attr->track_id, box->x, box->y, ts);
			}
		}
	}
}

void* metadata_thread(void *arg)
{
	int i, j, ret;
	unsigned long long ts = 0;

	printf("INFO(%s): Akubelli_AIEngine_Create_Channel ...\n", TAG);
	ch_context_t md_channel = Akubelli_AIEngine_Create_Channel(MD_NOBLOCK);
	if (!md_channel) {
		printf("ERROR(%s): Akubelli_AIEngine_Create_Channel failed!\n", TAG);
	}
	printf("INFO(%s): Akubelli_AIEngine_Create_Channel OK\n", TAG);

#if DEBUG_RAW_CAPTURE
	ret = Akubelli_Raw_Image_Capture(RAW_IMAGE_PATH, RAW_MD5_PATH);
	if (ret) {
		printf("ERROR: Akubelli_Raw_Image_Capture failed!\n");
	}
	printf("Capture raw image OK !\n");
#endif

#if DEBUG_YUV_CAPTURE
	printf("Start capture yuv image ...\n");
	ret = Akubelli_YUV_Image_Capture(YUV_IMAGE_PATH, YUV_MD5_PATH);
	if (ret) {
		printf("ERROR: Akubelli_YUV_Image_Capture failed!\n");
	}
	printf("Capture raw image OK !\n");
#endif

#if REMOVE_MD_FPS_PRINTING
	printf("Start remove MD fps printing ...\n");
	ret = Akubelli_Control_Fps_Printing(0);
	if (ret) {
		printf("ERROR: Akubelli_Control_Fps_Printing failed!\n");
	}
	printf("Remove MD fps OK !\n");
#endif

	while (1) {
		AK_MetaDatadDesc md_desc;
		md_context_t md_context = Akubelli_AIEngine_Request_MD(md_channel, &md_desc);

		if (md_context) {
#ifdef MD_TIME_COST_DEBUG
			{ /* DEBUG: used for time debug */
				struct timeval cur_time;
				uint64_t t0, t1, t2, t3, t4, t5;
				static int i = 1, index = 1;
				static uint64_t avg = 0, min = -1, max = 0, sum = 0;
				uint64_t *time_store = (uint64_t *)(&(md_desc.reserved));
				AK_MetaDatadDesc *desc = &md_desc;
				gettimeofday(&cur_time, NULL);
				t0 = desc->ts;
				t1 = time_store[0];
				t2 = time_store[1];
				t3 = time_store[2];
				t4 = time_store[3];
				min = (t4 - t0) < min ? (t4 - t0) : min;
				index = (t4 - t0) > max ? i : index;
				max = (t4 - t0) > max ? (t4 - t0) : max;
				sum += t4 -t0;
				avg = sum / i++;
				t5 = cur_time.tv_sec * 1000000LL + cur_time.tv_usec;
				printf("MD time const debug : avg = %llums, min = %llums, max(%d) = %llums\n"
					   " item: [frame done] [analysis done] [trans ready] [received done] [app request] [current time]\n"
					   " time(us):      %llu          %llu           %llu           %llu          %llu          %llu      \n"
					   " keep(ms):      %llu          %llu           %llu           %llu          %llu          %llu      \n",
					   avg / 1000, min / 1000, index, max / 1000,
					   t0, t1, t2, t3, t4, t5,
					   (t0 - t0) / 1000, (t1 - t0) / 1000, (t2 - t1) / 1000, (t3 - t2) / 1000, (t4 - t3) / 1000, (t5 - t4) / 1000);
			}
#endif /* MD_TIME_COST_DEBUG */
			osd_index = 0;
			id_osd_index = 0;
			/*struct timeval sys_time;*/
			/*gettimeofday(&sys_time, NULL);*/
			ts = md_desc.ts;
			/*printf("md ts = %llu. sys ts = %llu.\n", md_desc.ts / 1000, (unsigned long long)sys_time.tv_sec * 1000 + sys_time.tv_usec / 1000);*/
			/*print_mddesc(&md_desc);*/

			if (md_desc.objnum > 0) {
					osd_clear_threshold = 0;
#if PRINT_RESULT
				printf("##### %u frame result #####\n", frame_index++);
#endif
			} else {
				osd_clear_threshold++;

				/* clear remaining osd region */
				if (osd_clear_threshold >= 25) {
					osd_clear_threshold = 0;
					for (i = 0; i < MAX_IVS_OSD_REGION + MAX_IVS_ID_OSD_REGION; i++) {
						handler = ivsRgnHandler[i];
						IMP_OSD_ShowRgn(handler, 0 , 0);
					}
				}

			}
			for (j = 0; j < md_desc.objnum; j++) {
				AK_ObjDesc obj_desc;
				obj_context_t obj_context = Akubelli_AIEngine_Get_Object(md_context, j, &obj_desc);
				if (obj_context) {
					/*print_objdesc(&obj_desc);*/
					PlateTextpredict plate_txt_prdt;
					memset(&plate_txt_prdt, 0x0, sizeof(PlateTextpredict));

					if (obj_desc.type == OBJ_TYPE_FACE) {
						FaceAttr face_attr;
						ret = Akubelli_AIEngine_Get_FaceAttr(obj_context, &face_attr);
						if (ret) {
							printf("ERROR(%s): Akubelli_AIEngine_Get_FaceAttr failed!\n", TAG);
						}
						print_faceattr(&face_attr, ts);
						if (face_attr.cap_image.image_valid) {
							printf("Face: have image, image size: [%d] Bytes\n", face_attr.cap_image.image_size);
							image_context_t img = Akubelli_AIEngine_Get_ObjImage(obj_context);
							if (0) {
								FILE *face_fp = NULL;
								char face_fname[32] = {0};
								sprintf(face_fname, "/tmp/fc_face_img_%d.jpeg", \
										face_attr.cap_image.image_size);
								face_fp = fopen(face_fname, "w+");
								if (!face_fp) {
									printf("open %s error!\n", face_fname);
								}
								fwrite((void *)img, face_attr.cap_image.image_size, 1, face_fp);
								fclose(face_fp);
							}
						}

						if (face_attr.groups_cnt > 0) {
							TokenScore *tokensre = Akubelli_AIEngine_Get_FaceGToken(obj_context, 0, 0);
							if (tokensre != NULL) {
								printf("TokenScore -> grpct: %d\n", face_attr.groups_cnt);
								printf("TokenScore -> token: %d\n", tokensre->token);
								printf("TokenScore -> score: %f\n", tokensre->score);
							}
						}
					} else if (obj_desc.type == OBJ_TYPE_HUMAN) {
						HumanAttr human_attr;
						ret = Akubelli_AIEngine_Get_HumanAttr(obj_context, &human_attr);
						if (ret) {
							printf("ERROR(%s): Akubelli_AIEngine_Get_HumanAttr failed!\n", TAG);
						}
						print_humanattr(&human_attr, ts);

						if (human_attr.cap_image.image_valid) {
							printf("Human: have image, image size: [%d] Bytes\n", human_attr.cap_image.image_size);
							image_context_t img = Akubelli_AIEngine_Get_ObjImage(obj_context);
							if (0) {
								FILE *human_fp = NULL;
								char human_fname[32] = {0};
								sprintf(human_fname, "/tmp/fc_human_img_%d.jpeg", \
										human_attr.cap_image.image_size);
								human_fp = fopen(human_fname, "w+");
								if (!human_fp) {
									printf("open %s error!\n", human_fname);
								}
								fwrite((void *)img, human_attr.cap_image.image_size, 1, human_fp);
								fclose(human_fp);
							}
						}
					} else if (obj_desc.type == OBJ_TYPE_PLATE) {
						PlateAttr plate_attr;
						ret = Akubelli_AIEngine_Get_PlateAttr(obj_context, &plate_attr);
						if (ret) {
							printf("ERROR(%s): Akubelli_AIEngine_Get_PlateAttr failed!\n", TAG);
						}
						print_plateattr(&plate_attr, ts, obj_context);
						if (plate_attr.cap_image.image_valid) {
							printf("Plate: have image, image size: [%d] Bytes\n", plate_attr.cap_image.image_size);
							image_context_t img = Akubelli_AIEngine_Get_ObjImage(obj_context);
							if (0) {
								FILE *plate_fp = NULL;
								char plate_fname[32] = {0};
								sprintf(plate_fname, "/tmp/fc_plate_img_%d.jpeg", \
										plate_attr.cap_image.image_size);
								plate_fp = fopen(plate_fname, "w+");
								if (!plate_fp) {
									printf("open %s error!\n", plate_fname);
								}
								fwrite((void *)img, plate_attr.cap_image.image_size, 1, plate_fp);
								fclose(plate_fp);
							}
						}
					} else if (obj_desc.type == OBJ_TYPE_VEHICLE) {
						VehicleAttr vehicle_attr;
						ret = Akubelli_AIEngine_Get_VehicleAttr(obj_context, &vehicle_attr);
						if (ret) {
							printf("ERROR(%s): Akubelli_AIEngine_Get_VehicleAttr failed!\n", TAG);
						}
						print_vehicleattr(&vehicle_attr, ts);

						if (vehicle_attr.cap_image.image_valid) {
							printf("Vehicle: have image, image size: [%d] Bytes\n", vehicle_attr.cap_image.image_size);
							image_context_t img = Akubelli_AIEngine_Get_ObjImage(obj_context);
							if (0) {
								FILE *vehicle_fp = NULL;
								char vehicle_fname[32] = {0};
								sprintf(vehicle_fname, "/tmp/fc_vehicle_img_%d.jpeg", \
										vehicle_attr.cap_image.image_size);
								vehicle_fp = fopen(vehicle_fname, "w+");
								if (!vehicle_fp) {
									printf("open %s error!\n", vehicle_fname);
								}
								fwrite((void *)img, vehicle_attr.cap_image.image_size, 1, vehicle_fp);
								fclose(vehicle_fp);
							}
						}
					} else {
						printf("ERROR(%s): Unknown object type [%d]!\n", TAG, obj_desc.type);
					}

					Akubelli_AIEngine_Free_Object(obj_context);
				}
			}
			Akubelli_AIEngine_Release_MD(md_channel, md_context);

			if (last_time_osd_index > osd_index) {
				for (i = osd_index; i < MAX_IVS_OSD_REGION; i++) {
					handler = ivsRgnHandler[i];
					IMP_OSD_ShowRgn(handler, 0 , 0);
				}
			}

			if (last_time_id_osd_index > id_osd_index) {
				for (i = id_osd_index; i < MAX_IVS_ID_OSD_REGION; i++) {
					handler = ivsRgnHandler[MAX_IVS_OSD_REGION + i];
					IMP_OSD_ShowRgn(handler, 0 , 0);
				}
			}

			last_time_osd_index = osd_index;
			last_time_id_osd_index = id_osd_index;

		} else {
			usleep(40 * 1000);
		}
	}

	printf("INFO(%s): Akubelli_AIEngine_Destroy_Channel ...\n", TAG);
	ret = Akubelli_AIEngine_Destroy_Channel(md_channel);
	if (ret) {
		printf("ERROR(%s): Akubelli_AIEngine_Destroy_Channel failed!\n", TAG);
		/*return -1;*/
	}
	printf("INFO(%s): Akubelli_AIEngine_Destroy_Channel OK\n", TAG);

	/*return 0;*/
	pthread_exit(NULL);
}

#if FACE_RECOG
static unsigned int req_cnt = 0;
static MegRequest meg_request;
static MGBuildGroupReply reply;
static MGDeleteTokenReply *deltoken_reply = NULL;
static BuildGroupRequest *build_group_request = NULL;
static char face_token[128] = {0};

void *face_reg(void *arg)
{
	int ret = -1;
	int id = -1;
	int i = 0;
	FILE *imgfp = NULL;
	char *face_img_data = NULL;

	meg_request.face_request.encoded_image.blob = (char *)malloc(MAX_ENCODE_IMAGE_SIZE);
	if (!meg_request.face_request.encoded_image.blob) {
		printf("malloc blob error!\n");
		goto error;
	}
	face_img_data = meg_request.face_request.encoded_image.blob;
	memset(face_img_data, 0x0, MAX_ENCODE_IMAGE_SIZE);

	imgfp = fopen("/mnt/fr0001.jpg", "r");
	if (!imgfp) {
		printf("fopen %s error.[%s]\n", "/mnt/fr0001.jpg", strerror(errno));
		goto error1;
	}

	ret = fseek(imgfp, 0, SEEK_END);
	if (ret) {
		printf("fseek SEEK_END error. [%s]\n", strerror(errno));
		goto error2;
	}

	long imgsize = ftell(imgfp);
	if (imgsize == -1) {
		printf("ftell error. [%s]\n", strerror(errno));
		goto error3;
	}

	if (imgsize > MAX_ENCODE_IMAGE_SIZE) {
		printf("image toooooo big, max limit size is 128KB\n");
		goto error3;
	}

	ret = fseek(imgfp, 0, SEEK_SET);
	if (ret) {
		printf("fseek SEEK_SET error. [%s]\n", strerror(errno));
		goto error4;
	}

	unsigned int read_size = 0;
	read_size = fread(face_img_data, 1, imgsize, imgfp);
	if (read_size != imgsize) {
		printf("image read error, read %d bytes, need %ld bytes\n", read_size, imgsize);
		goto error5;
	}
/*
 * MAX_TOKENS __MUST__ <= 15000
 * __BUT__ if MAX_TOKENS Tooooo Big, will wast memory
 */
#define MAX_TOKENS 1000
	deltoken_reply = (MGDeleteTokenReply *)malloc(sizeof(MGDeleteTokenReply) + sizeof(int) * MAX_TOKENS);
	if (!deltoken_reply) {
		printf("malloc deltoken_reply error!\n");
		goto error6;
	}

	sleep(5);
	printf("Start Import Face Image To T02...\n");
	for (i = 0; i < 10; i++) {
		printf("Put [%d] image to T02 FR Library\n", i);

		FaceReply face_reply;
		memset(&face_reply, 0x0, sizeof(FaceReply));

		meg_request.face_request.image_encode = true;
		meg_request.face_request.encoded_image.blob_size = imgsize;
		meg_request.face_request.encoded_image.encode_format = RIEF_JPG;
		meg_request.face_request.recognition_request.token_request.add_new_token = true;
		meg_request.face_request.attribute_request.predict_landmark = true;
		meg_request.request_id = req_cnt;
		meg_request.rtype = MRT_FACE_REQUEST;

		printf("\tSend FaceRequest to T02\n");
		ret = Akubelli_AIEngine_MGReq_FaceFequest(&meg_request, &face_reply, &id);
		if (ret) {
			printf("Akubelli_AIEngine_MGReq_FaceFequest error!\n");
		}

		printf("\tReply Token: %d\n", face_reply.face_recognition_reply.token_reply.token);

		if (face_reply.face_recognition_reply.token_reply.face_quality == RFQ_GOOD) {
			//save token
			face_token[req_cnt] = face_reply.face_recognition_reply.token_reply.token;
			printf("Face add success!\n");
		} else {
			printf("Face Quality not enough: [%d]\n",
					face_reply.face_recognition_reply.token_reply.face_quality);
		}

		req_cnt++;

		sleep(2);
	}

	{
		int idx = 0, goft = 0;
		memset(&reply, 0x0, sizeof(MGBuildGroupReply));

#define MAX_GRP_CNT	5
#define MAX_PGRP_ROKEN_CNT 3000
		if (!build_group_request) {
			build_group_request = (BuildGroupRequest *)malloc(sizeof(BuildGroupRequest) + MAX_GRP_CNT*(sizeof(BuildGroup) + MAX_PGRP_ROKEN_CNT*sizeof(int)));
			if (!build_group_request) {
				printf("malloc build_group_request error!\n");
				return NULL;
			}
		}
		build_group_request->groups_cnt = 2;
		goft += sizeof(BuildGroupRequest);

		printf("Has [%d] Token Will be InLib\n", req_cnt);
		//Fill Group0
		BuildGroup *bgrp0 = (BuildGroup *)((char *)build_group_request + goft);
		bgrp0->group_id = 1;
		bgrp0->top_k = 1;
		bgrp0->stop_recog_cond.score_threshold = 75;
		bgrp0->tokens_cnt = req_cnt/2;
		for (idx = 0; idx < bgrp0->tokens_cnt; idx++) {
			bgrp0->tokens[idx] = face_token[idx];
		}

		goft += (sizeof(BuildGroup) + (sizeof(int)*(bgrp0->tokens_cnt)));

		//Fill Group1
		BuildGroup *bgrp1 = (BuildGroup *)((char *)build_group_request + goft);
		bgrp1->group_id = 2;
		bgrp1->top_k = 1;
		bgrp1->stop_recog_cond.score_threshold = 75;
		bgrp1->tokens_cnt = req_cnt/2;
		for (idx = 0; idx < bgrp1->tokens_cnt; idx++) {
			bgrp1->tokens[idx] = face_token[bgrp1->tokens_cnt + idx];
		}

		ret = Akubelli_AIEngine_MGReq_BuildGroupRequest(build_group_request, &reply, &id);
		if (ret) {
			printf("Akubelli_AIEngine_MGReq_BuildGroupRequest error!\n");
		}
	}

	printf("Token InLib OK, Start Delete Token...\n");

	//delete token
	{
		memset(deltoken_reply, 0x0, sizeof(MGDeleteTokenReply) + sizeof(int)*MAX_TOKENS);

		meg_request.rtype = MRT_DELETE_TOKEN_REQUEST;
		meg_request.delete_token_request.delete_tokens_cnt = 2;
		for (i = 0; i < meg_request.delete_token_request.delete_tokens_cnt; i++) {
			meg_request.delete_token_request.delete_tokens[i] = 5+i;
		}

		Akubelli_AIEngine_MGReq_DeleteTokenRequest(&meg_request, deltoken_reply, &id);

		switch (deltoken_reply->status) {
			case DDFT_UNKNOWN:
				printf("DeleteTokens Status: [%s]\n", "DDFT_UNKNOWN"); break;
			case DDFT_SUCCESS:
				printf("DeleteTokens Status: [%s]\n", "DDFT_SUCCESS"); break;
			case DDFT_FAILED:
				printf("DeleteTokens Status: [%s]\n", "DDFT_FAILED"); break;
			default:
				printf("DeleteTokens Status: [%s]\n", "Default DDFT_UNKNOWN"); break;
		}

		printf("present_tokens_cnt: [%d]\n", deltoken_reply->present_tokens_cnt);
		for (i = 0; i < deltoken_reply->present_tokens_cnt; i++) {
			printf("PresentTokens: [%d]\n", deltoken_reply->present_tokens[i]);
		}
	}

	printf("Query Tokens..........\n");
	//query token
	{
		memset(deltoken_reply, 0x0, sizeof(MGDeleteTokenReply) + sizeof(int)*MAX_TOKENS);

		meg_request.rtype = MRT_DELETE_TOKEN_REQUEST;
		meg_request.delete_token_request.delete_tokens_cnt = 0;
		meg_request.delete_token_request.delete_tokens[0] = 0;

		Akubelli_AIEngine_MGReq_DeleteTokenRequest(&meg_request, deltoken_reply, &id);

		switch (deltoken_reply->status) {
			case DDFT_UNKNOWN:
				printf("DeleteTokens Status: [%s]\n", "DDFT_UNKNOWN"); break;
			case DDFT_SUCCESS:
				printf("DeleteTokens Status: [%s]\n", "DDFT_SUCCESS"); break;
			case DDFT_FAILED:
				printf("DeleteTokens Status: [%s]\n", "DDFT_FAILED"); break;
			default:
				printf("DeleteTokens Status: [%s]\n", "Default DDFT_UNKNOWN"); break;
		}

		printf("Query present_tokens_cnt: [%d]\n", deltoken_reply->present_tokens_cnt);
		for (i = 0; i < deltoken_reply->present_tokens_cnt; i++) {
			printf("Query PresentTokens: [%d]\n", deltoken_reply->present_tokens[i]);
		}
	}

error6:
error5:
error4:
error3:
error2:
	fclose(imgfp);
	imgfp = NULL;
error1:
	free(face_img_data);
error:
	return NULL;
}
#endif

static sem_t event_sem;
static void sig_handler_event(int sig)
{
	sem_post(&event_sem);
}

static void* isp_event_handler(void *arg)
{
	ISP_EV_Attr ev_attr;
	int ret;

	while(1) {
		sem_wait(&event_sem);
		ret = host_get_isp_EV_attr(&ev_attr);
		if (ret) {
		printf("Host get isp EV attr failed this time\n");
		}
		/*printf("ev = 0x%08x, sensor_again = 0x%08x, isp_dgain = 0x%08x\n", \*/
				/*ev_attr.ev, ev_attr.sensor_again, ev_attr.isp_dgain);*/
		Akubelli_Calibration_EV_Attr(ev_attr);
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int ret;
	int isp_fd;
	int akubelli_test_pid;
#if CHECK_VERSION
	unsigned int version;
#endif
	struct sigaction usr_action;
	sigset_t block_mask;
	pthread_t metadata_osd_thread;
	pthread_t isp_event_thread;
	AkubelliAttr *search_ioattr = NULL;
#if FACE_RECOG
	pthread_t face_recog_td;
#endif

	if (!(strcmp(argv[1], "help"))) {
		printf("Usage:\n");
		printf("     ./akubelli_test --st=imx307\n");

		return 0;
	}

	search_ioattr = CheckArgs(argc, argv);

	printf("INFO(%s): Akubelli_Init ...\n", TAG);
	ret = Akubelli_Init(search_ioattr);
	if (ret) {
		printf("ERROR(%s): Akubelli_Init failed!\n", TAG);
		exit(1);
	}
	printf("INFO(%s): Akubelli_Init OK\n", TAG);

#if CHECK_VERSION
	ret = Akubelli_Check_T02_Version(&version);
	if (ret) {
		printf("ERROR(%s): Check T02 version failed.\n", TAG);
		return -1;
	}
	printf("####### T02 firmware version is 0x%08x #######\n", version);

	version = 0;
	ret = Akubelli_Check_SDK_Version(&version);
	if (ret) {
		printf("ERROR(%s): Check SDK version failed.\n", TAG);
		return -1;
	}
	printf("####### SDK version is 0x%08x #######\n", version);
#endif

#if MODIFY_CNN_USER
	/* read the original cmd params */
	uint32_t cmd_have = 0, cmd_value = 0;
	uint32_t set_value = 100;
	int i = 0;

	for (i = 0; i < 4; i++) {
		ret = Akubelli_Get_Cnn_Params(i, &cmd_have, &cmd_value);
		if (ret) {
			printf("Akubelli_Get_Cnn_Params failed.\n");
			return -1;
		}
		if (cmd_have) {
			printf("Before: %d %u.\n", i, cmd_value);
		} else {
			printf("Before: No find.\n");
		}

#if 1
		/* Add test */
		if (((cmd_value != set_value) && (cmd_have)) || (!cmd_have)) {
			ret = Akubelli_Set_Cnn_Params(CNN_OPERATING_CMD_ADD, i, set_value);
			if (ret) {
				printf("ERROR: Akubelli_Set_Cnn_Params failed.\n");
				return -1;
			}
		}
#endif

#if 0
		/* Delete test */
		if (cmd_have) {
			ret = Akubelli_Set_Cnn_Params(CNN_OPERATING_CMD_DELETE, i, 0);
			if (ret) {
				printf("ERROR: Akubelli_Set_Cnn_Params failed.\n");
				return -1;
			}
		}
#endif

		/* Read the final cmd params */
		ret = Akubelli_Get_Cnn_Params(i, &cmd_have, &cmd_value);
		if (ret) {
			printf("Akubelli_Get_Cnn_Params failed.\n");
			return -1;
		}
		if (cmd_have) {
			printf("AFTER: %d %u.\n", i, cmd_value);
		} else {
			printf("AFTER: No find.\n");
		}
	}

	for (i = 128; i < 132; i++) {
		ret = Akubelli_Get_Cnn_Params(i, &cmd_have, &cmd_value);
		if (ret) {
			printf("Akubelli_Get_Cnn_Params failed.\n");
			return -1;
		}
		if (cmd_have) {
			printf("Before: %d %u.\n", i, cmd_value);
		} else {
			printf("Before: No find.\n");
		}

#if 1
		/* Add test */
		if (!cmd_have) {
			ret = Akubelli_Set_Cnn_Params(CNN_OPERATING_CMD_ADD, i, 0);
			if (ret) {
				printf("ERROR: Akubelli_Set_Cnn_Params failed.\n");
				return -1;
			}
		}
#endif

#if 0
		/* Delete test */
		if (cmd_have) {
			ret = Akubelli_Set_Cnn_Params(CNN_OPERATING_CMD_DELETE, i, 0);
			if (ret) {
				printf("ERROR: Akubelli_Set_Cnn_Params failed.\n");
				return -1;
			}
		}
#endif

		/* read the final cmd params */
		ret = Akubelli_Get_Cnn_Params(i, &cmd_have, &cmd_value);
		if (ret) {
			printf("Akubelli_Get_Cnn_Params failed.\n");
			return -1;
		}
		if (cmd_have) {
			printf("AFTER: %d %u.\n", i, cmd_value);
		} else {
			printf("AFTER: No find.\n");
		}
	}

	/* reboot T02 */
	Akubelli_Hardware_Reset();

	/* Init T02 again */
	Akubelli_DeInit();
	printf("INFO(%s): Akubelli_Init ...\n", TAG);
	ret = Akubelli_Init(search_ioattr);
	if (ret) {
		printf("ERROR(%s): Akubelli_Init failed!\n", TAG);
		exit(1);
	}
	printf("INFO(%s): Akubelli_Init OK\n", TAG);
#endif

#if CONTROL_FACE_RECOG
	printf("INFO(%s): Start to control face recog switch.\n", TAG);
	ret = Akubelli_Control_Recog_Switch(CNN_FACE_RECOG_OPEN);
	if (ret) {
		printf("ERROR(%s): Akubelli_Control_Recog_Switch failed.\n");
		exit(1);
	}
	printf("INFO(%s): Akubelli_Control_Recog_Switch OK\n", TAG);
#endif

#if CONTROL_PUSH_IMAGE
	printf("INFO(%s): Start to control push image switch.\n", TAG);
	ret = Akubelli_Control_Push_Image_Switch(CNN_OPEN_PUSH_IMAGE);
	if (ret) {
		printf("ERROR(%s): Akubelli_Control_Push_Image_Switch failed.\n");
		exit(1);
	}
	printf("INFO(%s): Akubelli_Control_Push_Image_Switch OK\n", TAG);
#endif

	printf("INFO(%s): Akubelli_Start ...\n", TAG);
	ret = Akubelli_Start();
	if (ret) {
		printf("ERROR(%s): Akubelli_Start failed!\n", TAG);
		exit(1);
	}
	printf("INFO(%s): Akubelli_Start OK\n", TAG);

	printf("INFO(%s): Akubelli_Timer Start ...\n", TAG);
	ret = Akubelli_Time_Sync(1, 5);
	if (ret) {
		printf("ERROR(%s): Akubelli_Timer Start failed!\n", TAG);
		exit(1);
	}
	printf("INFO(%s): Akubelli_Timer Start OK\n", TAG);

	printf("INFO(%s): Rtsp server Start ...\n", TAG);
	ret = rtsp_server_start();
	if (ret) {
		printf("rtsp_server_start failed!\n");
		return -1;
	}
	printf("INFO(%s): Rtsp server Start OK\n", TAG);

	ret = pthread_create(&metadata_osd_thread, NULL, metadata_thread, NULL);
	if (ret) {
		printf("pthread_create metadata_osd_thread error\n");
		return -1;
	}

#if FACE_RECOG
	ret = pthread_create(&face_recog_td, NULL, face_reg, NULL);
	if (ret) {
		printf("pthread_create face_recog_td error\n");
		return -1;
	}
#endif

#if NEMER_TEST
	double score = 0.0;
	FaceScoreConfig fscfg;

	FILE *ffvA = NULL;
	FILE *ffvB = NULL;
	FILE *ffvC = NULL;

	unsigned int readsize = 0;
	unsigned int ffvASize = 0;
	unsigned int ffvBSize = 0;
	unsigned int ffvCSize = 0;

	char A[512] = {0};
	char B[512] = {0};
	char C[512] = {0};

	ffvA = fopen(FFVA_PATH, "r");
	if (!ffvA) {
		printf("fopen %s error!\n", FFVA_PATH);
	}

	ffvB = fopen(FFVB_PATH, "r");
	if (!ffvB) {
		printf("fopen %s error!\n", FFVB_PATH);
	}

	ffvC = fopen(FFVC_PATH, "r");
	if (!ffvC) {
		printf("fopen %s error!\n", FFVC_PATH);
	}

	fseek(ffvA, 0L, SEEK_END);
	ffvASize = ftell(ffvA);
	fseek(ffvA, 0L, SEEK_SET);

	fseek(ffvB, 0L, SEEK_END);
	ffvBSize = ftell(ffvB);
	fseek(ffvB, 0L, SEEK_SET);

	fseek(ffvC, 0L, SEEK_END);
	ffvCSize = ftell(ffvC);
	fseek(ffvC, 0L, SEEK_SET);

	printf("ffvASize: [%d]bytes, ffvBSize: [%d]bytes, ffvCSize: [%d]bytes\n", ffvASize, ffvBSize, ffvCSize);

	readsize = fread((void *)A, ffvASize, 1, ffvA);
	if (readsize != 1) {
		printf("read %s error!\n", FFVA_PATH);
	}

	readsize = fread((void *)B, ffvBSize, 1, ffvB);
	if (readsize != 1) {
		printf("read %s error!\n", FFVB_PATH);
	}

	readsize = fread((void *)C, ffvCSize, 1, ffvC);
	if (readsize != 1) {
		printf("read %s error!\n", FFVC_PATH);
	}

	fscfg = Akub_nemer_get_face_score_cfg("frostnova");
	if (fscfg.feat_size == 0) {
		printf("[ERROR] get face score config error!\n");
	}

	score = Akub_nemer_get_face_score(A, B, fscfg);
	printf("A-B: %lf\n", score);
	score = Akub_nemer_get_face_score(A, C, fscfg);
	printf("A-C: %lf\n", score);
	score = Akub_nemer_get_face_score(B, C, fscfg);
	printf("B-C: %lf\n", score);

	fclose(ffvA);
	fclose(ffvB);
	fclose(ffvC);
#endif

	/* Adjust T02 ISP effect by T30 ISP */
	ret = pthread_create(&isp_event_thread, NULL, isp_event_handler, NULL);
	if (ret) {
		printf("pthread_create isp_event_thread error\n");
		return -1;
	}

	sem_init(&event_sem, 0, 0);
	sigfillset(&block_mask);
	usr_action.sa_handler = sig_handler_event;
	usr_action.sa_mask = block_mask;
	usr_action.sa_flags = SA_NODEFER;
	sigaction(SIGRTMIN+1, &usr_action, NULL);
	akubelli_test_pid = getpid();

	isp_fd = open(DEV_T30_ISP, O_RDWR);
	if (isp_fd < 0) {
		printf("open dev isp failed\n");
		return -1;
	}

	ret = ioctl(isp_fd, IOCTL_SEND_USER_PID, &akubelli_test_pid);
	if (ret) {
		printf("ioctl send akubelli_test_pid failed\n");
		return -1;
	}

	/* set signal handler */
	signal_init();

	/* set autoexit */
	atexit(auto_deinit);

	/* loop */
	for(;;){
		sleep(30);
	}

	return 0;
}
