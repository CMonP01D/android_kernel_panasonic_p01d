/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <mach/qdsp5v2/snddev_icodec.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/qdsp5v2/audio_interct.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/afe.h>
#include <mach/qdsp5v2/lpa.h>
#include <mach/qdsp5v2/marimba_profile.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <linux/wakelock.h>
#include <mach/debug_mm.h>
#include <mach/rpc_pmapp.h>
#include <mach/qdsp5v2/audio_acdb_def.h>
#include <linux/slab.h>

#define SMPS_AUDIO_PLAYBACK_ID	"AUPB"
#define SMPS_AUDIO_RECORD_ID	"AURC"

/* [SIMT-lisensen-110721] for spliting audio parameters for voice call and media sound. */
typedef enum {
	/* This method is required
	*/
	SND_METHOD_VOICE = 0,   /* Use the device's voice generator                */

    /* Application-Specific Methods
    */
	SND_METHOD_KEY_BEEP,    /* Use the device's keybeep generator              */
	SND_METHOD_MESSAGE,     /* Use the path's ringer, or voice generator       */
	SND_METHOD_RING,        /* Use the device's ring generator                 */
	SND_METHOD_MIDI,        /* Use the device's Midi generator                 */
	SND_METHOD_AUX,         /* Use the device's auxiliary generator if avail.  */
	SND_METHOD_MAX,         /* Always last in the list                         */	 
} snd_method_type;

/* [SIMT-lisensen-110721] add icodec parameter calibration. */
enum acm_cmd_code_enum
{
  CMD_RESULT = 0,/**< Result of a request */
  CMD_SET_STRUCT,/**< Send a CALUNIT to other side*/
  CMD_GET_STRUCT,/**< Request a CALUNIT from other side*/
  CMD_GET_FEATURE_INFO,/**< Request information about Features enabled */
  CMD_SET_FEATURE_INFO,/**< Send information about Features enabled */
  CMD_SET_CALFILE_TO_SFS,/**< Set a CALFIL to SFS */
  CMD_SET_ADIE_REG,
  CMD_GET_ADIE_REG,
/*PMIC_CALIBRATION START*/
  CMD_CALL_PMIC,
/*PMIC_CALIBRATION END*/
};

typedef enum acm_calunit_enum_tag
{
	CALUNIT_DEV_ACTION,
	CALUNIT_DEV_VOC_VOL_LIMT,
	CALUNIT_MAX,/**< CALUNIT count*/
    CALUNIT_UNKNOWN=CALUNIT_MAX
} acm_calunit_enum;

struct acm_guid
{
  u32 x;
  u16 s1;
  u16 s2;
  u8  c[8];
} __attribute__((packed));

struct voc_filter_indices_struct
{
    s32 voc_cal_pcm_path_cfg;/**< PCM Path*/
    s32 snd_device;/**< Device type*/
    s32 snd_method;/**< Method type*/
} __attribute__((packed));

struct acm_diag_req
{
  u16 cmd;
  u8 *buf;
  u32 size;
} __attribute__((packed));

struct icodec_dev_info_t {
	char name[260];
	u32 capability;
	u32 copp_id;
	u32 acdb_id;
    u32 index;
} __attribute__((packed));

struct dev_action_group {
	u32 num;
	struct adie_codec_action_unit group[];
} __attribute__((__packed__));

struct dev_voc_vol_limt {
	s32 max;
	s32 min;
} __attribute__((__packed__));

#define ACM_GUID_SIZE                   16
#define ICODEC_IOCTL_MAGIC              'i'
#define ICODEC_GET_DEV_INFO             _IOWR(ICODEC_IOCTL_MAGIC, 0, struct icodec_dev_info_t*)
#define ICODEC_CAL_DEV                  _IOWR(ICODEC_IOCTL_MAGIC, 1, struct acm_diag_req*)
/* [SIMT-lisensen-110825]  for spliting audio parameters for voice call and media sound. */
#define ICODEC_SET_METHOD               _IOW(ICODEC_IOCTL_MAGIC, 2, int)

struct acm_guid calunitlist[CALUNIT_MAX]= { 
  // DEV ACTION
  {0xba398ec7,0x5200,0x45b7,{0x80,0x17,0x76,0xba,0xe4,0x2b,0x82,0x35}},

  // DEV_VOC_VOL_LIMT
  {0x258fc6fb,0xe73c,0x46ef,{0x95,0xea,0x5b,0x7e,0xdb,0xbd,0xf4,0x8b}},
}; 

#define SNDDEV_ICODEC_PCM_SZ 32 /* 16 bit / sample stereo mode */
#define SNDDEV_ICODEC_MUL_FACTOR 3 /* Multi by 8 Shift by 3  */
#define SNDDEV_ICODEC_CLK_RATE(freq) \
	(((freq) * (SNDDEV_ICODEC_PCM_SZ)) << (SNDDEV_ICODEC_MUL_FACTOR))

#ifdef CONFIG_DEBUG_FS
static struct adie_codec_action_unit debug_rx_actions[] =
		HANDSET_RX_8000_OSR_256;

static struct adie_codec_action_unit debug_tx_lb_actions[] = {
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_OFF },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x01, 0x01)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x01, 0x00) },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x30, 0x30)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x11, 0xfc, 0xfc)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x13, 0xfc, 0x58)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x14, 0xff, 0x65)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x15, 0xff, 0x64)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x82, 0xff, 0x5C)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_READY },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x0D, 0xF0, 0xd0)},
	{ ADIE_CODEC_ACTION_DELAY_WAIT, 0xbb8},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x14, 0x14)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x86, 0xff, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x50, 0x40)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x91, 0xFF, 0x01)}, /* Start loop back */
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_ANALOG_READY},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x10, 0x30)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x0D, 0xFF, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x14, 0x00)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_ANALOG_OFF},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x11, 0xff, 0x00)}
};

static struct adie_codec_action_unit debug_tx_actions[] =
		HANDSET_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry debug_rx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = debug_rx_actions,
		.action_sz = ARRAY_SIZE(debug_rx_actions),
	}
};

static struct adie_codec_hwsetting_entry debug_tx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = debug_tx_actions,
		.action_sz = ARRAY_SIZE(debug_tx_actions),
	}
};

static struct adie_codec_hwsetting_entry debug_tx_lb_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = debug_tx_lb_actions,
		.action_sz = ARRAY_SIZE(debug_tx_lb_actions),
	}
};

static struct adie_codec_dev_profile debug_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = debug_rx_settings,
	.setting_sz = ARRAY_SIZE(debug_rx_settings),
};

static struct adie_codec_dev_profile debug_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = debug_tx_settings,
	.setting_sz = ARRAY_SIZE(debug_tx_settings),
};

static struct adie_codec_dev_profile debug_tx_lb_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = debug_tx_lb_settings,
	.setting_sz = ARRAY_SIZE(debug_tx_lb_settings),
};
#endif /* CONFIG_DEBUG_FS */

/* Context for each internal codec sound device */
struct snddev_icodec_state {
	struct snddev_icodec_data *data;
	struct adie_codec_path *adie_path;
	u32 sample_rate;
	u32 enabled;
};

/* Global state for the driver */
struct snddev_icodec_drv_state {
	struct mutex rx_lock;
	struct mutex tx_lock;
	struct mutex lb_lock;
	u32 rx_active; /* ensure one rx device at a time */
	u32 tx_active; /* ensure one tx device at a time */
	u32 lb_active; /* ensure one lb device at a time */
	struct clk *rx_mclk;
	struct clk *rx_sclk;
	struct clk *tx_mclk;
	struct clk *tx_sclk;
	struct clk *lpa_codec_clk;
	struct clk *lpa_core_clk;
	struct clk *lpa_p_clk;
	struct lpa_drv *lpa;

	struct wake_lock rx_idlelock;
	struct wake_lock tx_idlelock;
	struct wake_lock lb_idlelock;   /* [SIMT-lisensen-110721] loopback device unlock. */
	
	snd_method_type method; /* [SIMT-lisensen-110825]  for spliting audio parameters for voice call and media sound. */
};

struct icodec_dev_t {
	struct list_head list;
	struct msm_snddev_info *dev_info;
};

struct icodec_misc_dev_t {
	struct list_head list;
	struct mutex lock;
};

static struct snddev_icodec_drv_state snddev_icodec_drv;
static struct icodec_misc_dev_t icodec_misc_dev = {
    .lock = __MUTEX_INITIALIZER(icodec_misc_dev.lock),
    .list = LIST_HEAD_INIT(icodec_misc_dev.list),
};

/* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. { */
static void snddev_icodec_set_action(struct snddev_icodec_state *icodec) {
	struct adie_codec_dev_profile *profile = icodec->data->profile;  
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	struct adie_codec_hwsetting_entry *settings = profile->settings;
	int i;

    for (i = 0; i < profile->setting_sz; i++) {
        switch (drv->method) {
        case SND_METHOD_VOICE:
            if (settings->voc && settings->voc_sz) {
			    settings->actions = settings->voc;
			    settings->action_sz = settings->voc_sz;
			}
			    break;
			case SND_METHOD_MIDI:
			    if (settings->media && settings->media_sz) {
			        settings->actions = settings->media;
			        settings->action_sz = settings->media_sz;
			    }
			    break;
			default:
			    ;
			}
			settings++;
    }
}
/* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. } */

static int snddev_icodec_open_rx(struct snddev_icodec_state *icodec)
{
	int trc, err;
	int smps_mode = PMAPP_SMPS_MODE_VOTE_PWM;
	struct msm_afe_config afe_config;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	struct lpa_codec_config lpa_config;

	wake_lock(&drv->rx_idlelock);

	if ((icodec->data->acdb_id == ACDB_ID_HEADSET_SPKR_MONO) ||
		(icodec->data->acdb_id == ACDB_ID_HEADSET_SPKR_STEREO)) {
		/* Vote PMAPP_SMPS_MODE_VOTE_PFM for headset */
		smps_mode = PMAPP_SMPS_MODE_VOTE_PFM;
		MM_DBG("snddev_icodec_open_rx: PMAPP_SMPS_MODE_VOTE_PFM \n");
	} else
		MM_DBG("snddev_icodec_open_rx: PMAPP_SMPS_MODE_VOTE_PWM \n");

	/* Vote for SMPS mode*/
	err = pmapp_smps_mode_vote(SMPS_AUDIO_PLAYBACK_ID,
				PMAPP_VREG_S4, smps_mode);
	if (err != 0)
		MM_ERR("pmapp_smps_mode_vote error %d\n", err);

	/* enable MI2S RX master block */
	/* enable MI2S RX bit clock */
	trc = clk_set_rate(drv->rx_mclk,
		SNDDEV_ICODEC_CLK_RATE(icodec->sample_rate));
	if (IS_ERR_VALUE(trc))
		goto error_invalid_freq;
	clk_enable(drv->rx_mclk);
	clk_enable(drv->rx_sclk);
	/* clk_set_rate(drv->lpa_codec_clk, 1); */ /* Remove if use pcom */
	clk_enable(drv->lpa_p_clk);
	clk_enable(drv->lpa_codec_clk);
	clk_enable(drv->lpa_core_clk);

	/* Enable LPA sub system
	 */
	drv->lpa = lpa_get();
	if (!drv->lpa)
		goto error_lpa;
	lpa_config.sample_rate = icodec->sample_rate;
	lpa_config.sample_width = 16;
	lpa_config.output_interface = LPA_OUTPUT_INTF_WB_CODEC;
	lpa_config.num_channels = icodec->data->channel_mode;
	lpa_cmd_codec_config(drv->lpa, &lpa_config);

	/* Set audio interconnect reg to LPA */
	audio_interct_codec(AUDIO_INTERCT_LPA);

	/* Set MI2S */
	mi2s_set_codec_output_path((icodec->data->channel_mode == 2 ?
	MI2S_CHAN_STEREO : MI2S_CHAN_MONO_PACKED), WT_16_BIT);

	if (icodec->data->voltage_on)
		icodec->data->voltage_on();

	/* Configure ADIE */
	trc = adie_codec_open(icodec->data->profile, &icodec->adie_path);
	if (IS_ERR_VALUE(trc))
		goto error_adie;
	/* OSR default to 256, can be changed for power optimization
	 * If OSR is to be changed, need clock API for setting the divider
	 */
	adie_codec_setpath(icodec->adie_path, icodec->sample_rate, 256);
	/* Start AFE */
	afe_config.sample_rate = icodec->sample_rate / 1000;
	afe_config.channel_mode = icodec->data->channel_mode;
	afe_config.volume = AFE_VOLUME_UNITY;
	trc = afe_enable(AFE_HW_PATH_CODEC_RX, &afe_config);
	if (IS_ERR_VALUE(trc))
		goto error_afe;
	lpa_cmd_enable_codec(drv->lpa, 1);
	/* Enable ADIE */
	adie_codec_proceed_stage(icodec->adie_path, ADIE_CODEC_DIGITAL_READY);
	adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_ANALOG_READY);

	/* Enable power amplifier */
	if (icodec->data->pamp_on)
		icodec->data->pamp_on();

	icodec->enabled = 1;

	wake_unlock(&drv->rx_idlelock);
	return 0;

error_afe:
	adie_codec_close(icodec->adie_path);
	icodec->adie_path = NULL;
error_adie:
	lpa_put(drv->lpa);
error_lpa:
	clk_disable(drv->lpa_p_clk);
	clk_disable(drv->lpa_codec_clk);
	clk_disable(drv->lpa_core_clk);
	clk_disable(drv->rx_sclk);
	clk_disable(drv->rx_mclk);
error_invalid_freq:

	MM_ERR("encounter error\n");

	wake_unlock(&drv->rx_idlelock);
	return -ENODEV;
}

static int snddev_icodec_open_tx(struct snddev_icodec_state *icodec)
{
	int trc;
	int i, err;
	struct msm_afe_config afe_config;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;;

	wake_lock(&drv->tx_idlelock);

	/* Vote for PWM mode*/
	err = pmapp_smps_mode_vote(SMPS_AUDIO_RECORD_ID,
			PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_PWM);
	if (err != 0)
		MM_ERR("pmapp_smps_mode_vote error %d\n", err);

	/* Reuse pamp_on for TX platform-specific setup  */
	if (icodec->data->pamp_on)
		icodec->data->pamp_on();

	for (i = 0; i < icodec->data->pmctl_id_sz; i++) {
		pmic_hsed_enable(icodec->data->pmctl_id[i],
			 PM_HSED_ENABLE_PWM_TCXO);
	}

	/* enable MI2S TX master block */
	/* enable MI2S TX bit clock */
	trc = clk_set_rate(drv->tx_mclk,
		SNDDEV_ICODEC_CLK_RATE(icodec->sample_rate));
	if (IS_ERR_VALUE(trc))
		goto error_invalid_freq;
	clk_enable(drv->tx_mclk);
	clk_enable(drv->tx_sclk);

	/* Set MI2S */
	mi2s_set_codec_input_path((icodec->data->channel_mode ==
				REAL_STEREO_CHANNEL_MODE ? MI2S_CHAN_STEREO :
				(icodec->data->channel_mode == 2 ?
				 MI2S_CHAN_STEREO : MI2S_CHAN_MONO_RAW)),
				WT_16_BIT);
	/* Configure ADIE */
	trc = adie_codec_open(icodec->data->profile, &icodec->adie_path);
	if (IS_ERR_VALUE(trc))
		goto error_adie;
	/* Enable ADIE */
	adie_codec_setpath(icodec->adie_path, icodec->sample_rate, 256);
	adie_codec_proceed_stage(icodec->adie_path, ADIE_CODEC_DIGITAL_READY);
	adie_codec_proceed_stage(icodec->adie_path,
	ADIE_CODEC_DIGITAL_ANALOG_READY);

	/* Start AFE */
	afe_config.sample_rate = icodec->sample_rate / 1000;
	afe_config.channel_mode = icodec->data->channel_mode;
	afe_config.volume = AFE_VOLUME_UNITY;
	trc = afe_enable(AFE_HW_PATH_CODEC_TX, &afe_config);
	if (IS_ERR_VALUE(trc))
		goto error_afe;


	icodec->enabled = 1;

	wake_unlock(&drv->tx_idlelock);
	return 0;

error_afe:
	adie_codec_close(icodec->adie_path);
	icodec->adie_path = NULL;
error_adie:
	clk_disable(drv->tx_sclk);
	clk_disable(drv->tx_mclk);
error_invalid_freq:

	/* Disable mic bias */
	for (i = 0; i < icodec->data->pmctl_id_sz; i++) {
		pmic_hsed_enable(icodec->data->pmctl_id[i],
			 PM_HSED_ENABLE_OFF);
	}

	if (icodec->data->pamp_off)
		icodec->data->pamp_off();

	MM_ERR("encounter error\n");

	wake_unlock(&drv->tx_idlelock);
	return -ENODEV;
}

static int snddev_icodec_open_lb(struct snddev_icodec_state *icodec)
{
	int trc;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	wake_lock(&drv->lb_idlelock);

	/* Configure ADIE */
	trc = adie_codec_open(icodec->data->profile, &icodec->adie_path);
	if (IS_ERR_VALUE(trc))
		goto error_adie;
	/* OSR default to 256, can be changed for power optimization
	 * If OSR is to be changed, need clock API for setting the divider
	 */
	adie_codec_setpath(icodec->adie_path, icodec->sample_rate, 256);
	/* Enable ADIE */
	adie_codec_proceed_stage(icodec->adie_path, ADIE_CODEC_DIGITAL_READY);
	adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_ANALOG_READY);
	afe_loopback(1);
	icodec->enabled = 1;

	wake_unlock(&drv->lb_idlelock);
	return 0;

error_adie:

	MM_ERR("encounter error\n");

	wake_unlock(&drv->lb_idlelock);
	return -ENODEV;
}

static int snddev_icodec_close_rx(struct snddev_icodec_state *icodec)
{
	int err;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	wake_lock(&drv->rx_idlelock);

	/* Remove the vote for SMPS mode*/
	err = pmapp_smps_mode_vote(SMPS_AUDIO_PLAYBACK_ID,
			PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_DONTCARE);
	if (err != 0)
		MM_ERR("pmapp_smps_mode_vote error %d\n", err);

	/* Disable power amplifier */
	if (icodec->data->pamp_off)
		icodec->data->pamp_off();

	/* Disable ADIE */
	adie_codec_proceed_stage(icodec->adie_path, ADIE_CODEC_DIGITAL_OFF);
	adie_codec_close(icodec->adie_path);
	icodec->adie_path = NULL;

	afe_disable(AFE_HW_PATH_CODEC_RX);

	if (icodec->data->voltage_off)
		icodec->data->voltage_off();

	/* Disable LPA Sub system */
	lpa_cmd_enable_codec(drv->lpa, 0);
	lpa_put(drv->lpa);

	/* Disable LPA clocks */
	clk_disable(drv->lpa_p_clk);
	clk_disable(drv->lpa_codec_clk);
	clk_disable(drv->lpa_core_clk);

	/* Disable MI2S RX master block */
	/* Disable MI2S RX bit clock */
	clk_disable(drv->rx_sclk);
	clk_disable(drv->rx_mclk);

	icodec->enabled = 0;

	wake_unlock(&drv->rx_idlelock);
	return 0;
}

static int snddev_icodec_close_tx(struct snddev_icodec_state *icodec)
{
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	int i, err;

	wake_lock(&drv->tx_idlelock);

	/* Remove the vote for SMPS mode*/
	err = pmapp_smps_mode_vote(SMPS_AUDIO_RECORD_ID,
			PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_DONTCARE);
	if (err != 0)
		MM_ERR("pmapp_smps_mode_vote error %d\n", err);

	afe_disable(AFE_HW_PATH_CODEC_TX);

	/* Disable ADIE */
	adie_codec_proceed_stage(icodec->adie_path, ADIE_CODEC_DIGITAL_OFF);
	adie_codec_close(icodec->adie_path);
	icodec->adie_path = NULL;

	/* Disable MI2S TX master block */
	/* Disable MI2S TX bit clock */
	clk_disable(drv->tx_sclk);
	clk_disable(drv->tx_mclk);

	/* Disable mic bias */
	for (i = 0; i < icodec->data->pmctl_id_sz; i++) {
		pmic_hsed_enable(icodec->data->pmctl_id[i],
			 PM_HSED_ENABLE_OFF);
	}

	/* Reuse pamp_off for TX platform-specific setup  */
	if (icodec->data->pamp_off)
		icodec->data->pamp_off();

	icodec->enabled = 0;

	wake_unlock(&drv->tx_idlelock);
	return 0;
}

static int snddev_icodec_close_lb(struct snddev_icodec_state *icodec)
{
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	wake_lock(&drv->lb_idlelock);

	afe_loopback(0);
	/* Disable ADIE */
	adie_codec_proceed_stage(icodec->adie_path, ADIE_CODEC_DIGITAL_OFF);
	adie_codec_close(icodec->adie_path);
	icodec->adie_path = NULL;
	icodec->enabled = 0;

	wake_unlock(&drv->lb_idlelock);
	return 0;
}

static int snddev_icodec_set_device_volume_impl(
		struct msm_snddev_info *dev_info, u32 volume)
{
	struct snddev_icodec_state *icodec;

	int rc = 0;

	icodec = dev_info->private_data;

	if (icodec->data->dev_vol_type & SNDDEV_DEV_VOL_DIGITAL) {

		rc = adie_codec_set_device_digital_volume(icodec->adie_path,
				icodec->data->channel_mode ==
						REAL_STEREO_CHANNEL_MODE ?
					2 : icodec->data->channel_mode, volume);
		if (rc < 0) {
			MM_ERR("unable to set_device_digital_volume for"
				"%s volume in percentage = %u\n",
				dev_info->name, volume);
			return rc;
		}

	} else if (icodec->data->dev_vol_type & SNDDEV_DEV_VOL_ANALOG) {
		rc = adie_codec_set_device_analog_volume(icodec->adie_path,
				icodec->data->channel_mode ==
						REAL_STEREO_CHANNEL_MODE ?
					2 : icodec->data->channel_mode, volume);
		if (rc < 0) {
			MM_ERR("unable to set_device_analog_volume for"
				"%s volume in percentage = %u\n",
				dev_info->name, volume);
			return rc;
		}
	}
	else {
		MM_ERR("Invalid device volume control\n");
		return -EPERM;
	}
	return rc;
}

static int snddev_icodec_close(struct msm_snddev_info *dev_info)
{
	int rc = 0;
	struct snddev_icodec_state *icodec;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	if (!dev_info) {
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;

	if (icodec->data->capability & SNDDEV_CAP_RX) {
		mutex_lock(&drv->rx_lock);
		if (!drv->rx_active) {
			mutex_unlock(&drv->rx_lock);
			rc = -EPERM;
			goto error;
		}
		rc = snddev_icodec_close_rx(icodec);
		if (!IS_ERR_VALUE(rc))
			drv->rx_active = 0;
		mutex_unlock(&drv->rx_lock);
	} 
	else if (icodec->data->capability & SNDDEV_CAP_TX) {
		mutex_lock(&drv->tx_lock);
		if (!drv->tx_active) {
			mutex_unlock(&drv->tx_lock);
			rc = -EPERM;
			goto error;
		}
		rc = snddev_icodec_close_tx(icodec);
		if (!IS_ERR_VALUE(rc))
			drv->tx_active = 0;
		mutex_unlock(&drv->tx_lock);
	}
	else if (icodec->data->capability & SNDDEV_CAP_LB) {
		mutex_lock(&drv->lb_lock);
		if (!drv->lb_active) {
			mutex_unlock(&drv->lb_lock);
			rc = -EPERM;
			goto error;
		}
		rc = snddev_icodec_close_lb(icodec);
		if (!IS_ERR_VALUE(rc))
			drv->lb_active = 0;
		mutex_unlock(&drv->lb_lock);
	}

error:
	return rc;
}

static int snddev_icodec_open(struct msm_snddev_info *dev_info)
{
	int rc = 0;
	struct snddev_icodec_state *icodec;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	if (!dev_info) {
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;

	if (icodec->data->capability & SNDDEV_CAP_RX) {
		mutex_lock(&drv->rx_lock);
		if (drv->rx_active) {
			mutex_unlock(&drv->rx_lock);
			rc = -EBUSY;
			goto error;
		}
		/* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. */
		snddev_icodec_set_action(icodec);  
		rc = snddev_icodec_open_rx(icodec);

		if (!IS_ERR_VALUE(rc)) {
			drv->rx_active = 1;
			if ((icodec->data->dev_vol_type & (
				SNDDEV_DEV_VOL_DIGITAL |
				SNDDEV_DEV_VOL_ANALOG)))
				rc = snddev_icodec_set_device_volume_impl(
						dev_info, dev_info->dev_volume);
				if (IS_ERR_VALUE(rc)) {
					MM_ERR("Failed to set device volume"
						" impl for rx device\n");
					snddev_icodec_close(dev_info);
					mutex_unlock(&drv->rx_lock);
					goto error;
				}
		}
		mutex_unlock(&drv->rx_lock);
	}
	else if (icodec->data->capability & SNDDEV_CAP_TX){
		mutex_lock(&drv->tx_lock);
		if (drv->tx_active) {
			mutex_unlock(&drv->tx_lock);
			rc = -EBUSY;
			goto error;
		}
		/* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. */
		snddev_icodec_set_action(icodec);  
		rc = snddev_icodec_open_tx(icodec);

		if (!IS_ERR_VALUE(rc)) {
			drv->tx_active = 1;
			if ((icodec->data->dev_vol_type & (
				SNDDEV_DEV_VOL_DIGITAL |
				SNDDEV_DEV_VOL_ANALOG)))
				rc = snddev_icodec_set_device_volume_impl(
						dev_info, dev_info->dev_volume);
				if (IS_ERR_VALUE(rc)) {
					MM_ERR("Failed to set device volume"
						" impl for tx device\n");
					snddev_icodec_close(dev_info);
					mutex_unlock(&drv->tx_lock);
					goto error;
				}
		}
		mutex_unlock(&drv->tx_lock);
	}
	else if (icodec->data->capability & SNDDEV_CAP_LB){
		mutex_lock(&drv->lb_lock);
		if (drv->lb_active) {
			mutex_unlock(&drv->lb_lock);
			rc = -EBUSY;
			goto error;
		}
		/* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. */
		snddev_icodec_set_action(icodec); 
		rc = snddev_icodec_open_lb(icodec);
		if (!IS_ERR_VALUE(rc))
			drv->lb_active = 1;
		mutex_unlock(&drv->lb_lock);
	}
error:
	return rc;
}

static int snddev_icodec_check_freq(u32 req_freq)
{
	int rc = -EINVAL;

	if ((req_freq != 0) && (req_freq >= 8000) && (req_freq <= 48000)) {
		if ((req_freq == 8000) || (req_freq == 11025) ||
			(req_freq == 12000) || (req_freq == 16000) ||
			(req_freq == 22050) || (req_freq == 24000) ||
			(req_freq == 32000) || (req_freq == 44100) ||
			(req_freq == 48000)) {
				rc = 0;
		} else
			MM_INFO("Unsupported Frequency:%d\n", req_freq);
		}
		return rc;
}

static int snddev_icodec_set_freq(struct msm_snddev_info *dev_info, u32 rate)
{
	int rc;
	struct snddev_icodec_state *icodec;

	if (!dev_info) {
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;
	if (adie_codec_freq_supported(icodec->data->profile, rate) != 0) {
		rc = -EINVAL;
		goto error;
	} else {
		if (snddev_icodec_check_freq(rate) != 0) {
			rc = -EINVAL;
			goto error;
		} else
			icodec->sample_rate = rate;
	}

	if (icodec->enabled) {
		snddev_icodec_close(dev_info);
		snddev_icodec_open(dev_info);
	}

	return icodec->sample_rate;

error:
	return rc;
}

static int snddev_icodec_enable_sidetone(struct msm_snddev_info *dev_info,
	u32 enable)
{
	int rc = 0;
	struct snddev_icodec_state *icodec;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	if (!dev_info) {
		MM_ERR("invalid dev_info\n");
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;

	if (icodec->data->capability & SNDDEV_CAP_RX) {
		mutex_lock(&drv->rx_lock);
		if (!drv->rx_active || !dev_info->opened) {
			MM_ERR("dev not active\n");
			rc = -EPERM;
			mutex_unlock(&drv->rx_lock);
			goto error;
		}
		rc = adie_codec_enable_sidetone(icodec->adie_path, enable);
		mutex_unlock(&drv->rx_lock);
	} else {
		rc = -EINVAL;
		MM_ERR("rx device only\n");
	}

error:
	return rc;

}

int snddev_icodec_set_device_volume(struct msm_snddev_info *dev_info,
		u32 volume)
{
	struct snddev_icodec_state *icodec;
	struct mutex *lock;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	int rc = -EPERM;

	if (!dev_info) {
		MM_INFO("device not intilized.\n");
		return  -EINVAL;
	}

	icodec = dev_info->private_data;

	if (!(icodec->data->dev_vol_type & (SNDDEV_DEV_VOL_DIGITAL
				| SNDDEV_DEV_VOL_ANALOG))) {

		MM_INFO("device %s does not support device volume "
				"control.", dev_info->name);
		return -EPERM;
	}
	dev_info->dev_volume =  volume;

	if (icodec->data->capability & SNDDEV_CAP_RX)
		lock = &drv->rx_lock;
	else
		lock = &drv->tx_lock;

	mutex_lock(lock);

	rc = snddev_icodec_set_device_volume_impl(dev_info,
			dev_info->dev_volume);
	mutex_unlock(lock);
	return rc;
}

static int snddev_icodec_probe(struct platform_device *pdev)
{
	int rc = 0, i;
	struct snddev_icodec_data *pdata;
	struct msm_snddev_info *dev_info;
	struct snddev_icodec_state *icodec;
    struct icodec_dev_t *icodec_dev;

	if (!pdev || !pdev->dev.platform_data) {
		printk(KERN_ALERT "Invalid caller \n");
		rc = -1;
		goto error;
	}
	pdata = pdev->dev.platform_data;
	if ((pdata->capability & SNDDEV_CAP_RX) &&
	   (pdata->capability & SNDDEV_CAP_TX)) {
		MM_ERR("invalid device data either RX or TX\n");
		goto error;
	}
	icodec = kzalloc(sizeof(struct snddev_icodec_state), GFP_KERNEL);
	if (!icodec) {
		rc = -ENOMEM;
		goto error;
	}
	dev_info = kmalloc(sizeof(struct msm_snddev_info), GFP_KERNEL);
	if (!dev_info) {
		kfree(icodec);
		rc = -ENOMEM;
		goto error;
	}

	dev_info->name = pdata->name;
	dev_info->copp_id = pdata->copp_id;
	dev_info->acdb_id = pdata->acdb_id;
	dev_info->private_data = (void *) icodec;
	dev_info->dev_ops.open = snddev_icodec_open;
	dev_info->dev_ops.close = snddev_icodec_close;
	dev_info->dev_ops.set_freq = snddev_icodec_set_freq;
	dev_info->dev_ops.set_device_volume = snddev_icodec_set_device_volume;
	dev_info->capability = pdata->capability;
	dev_info->opened = 0;
	msm_snddev_register(dev_info);
	icodec->data = pdata;
	icodec->sample_rate = pdata->default_sample_rate;
	dev_info->sample_rate = pdata->default_sample_rate;
	if (pdata->capability & SNDDEV_CAP_RX) {
		for (i = 0; i < VOC_RX_VOL_ARRAY_NUM; i++) {
			dev_info->max_voc_rx_vol[i] =
				pdata->max_voice_rx_vol[i];
			dev_info->min_voc_rx_vol[i] =
				pdata->min_voice_rx_vol[i];
		}
		/*sidetone is enabled only for  the device which
		property set for side tone*/
		if (pdata->property & SIDE_TONE_MASK)
			dev_info->dev_ops.enable_sidetone =
				snddev_icodec_enable_sidetone;
		else
			dev_info->dev_ops.enable_sidetone = NULL;
	} else {
		dev_info->dev_ops.enable_sidetone = NULL;
	}

	icodec_dev = kmalloc(sizeof(struct icodec_dev_t), GFP_KERNEL);
	if (icodec_dev) {
        list_add_tail(&icodec_dev->list, &icodec_misc_dev.list);
        icodec_dev->dev_info = dev_info;
	}
        
error:
	return rc;
}

static int snddev_icodec_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver snddev_icodec_driver = {
  .probe = snddev_icodec_probe,
  .remove = snddev_icodec_remove,
  .driver = { .name = "snddev_icodec" }
};

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_sdev_dent;
static struct dentry *debugfs_afelb;
static struct dentry *debugfs_adielb;
static struct adie_codec_path *debugfs_rx_adie;
static struct adie_codec_path *debugfs_tx_adie;

static int snddev_icodec_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	MM_INFO("snddev_icodec: debug intf %s\n", (char *) file->private_data);
	return 0;
}

static void debugfs_adie_loopback(u32 loop)
{
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	if (loop) {

		/* enable MI2S RX master block */
		/* enable MI2S RX bit clock */
		clk_set_rate(drv->rx_mclk,
			SNDDEV_ICODEC_CLK_RATE(8000));
		clk_enable(drv->rx_mclk);
		clk_enable(drv->rx_sclk);

		MM_INFO("configure ADIE RX path\n");
		/* Configure ADIE */
		adie_codec_open(&debug_rx_profile, &debugfs_rx_adie);
		adie_codec_setpath(debugfs_rx_adie, 8000, 256);
		adie_codec_proceed_stage(debugfs_rx_adie,
		ADIE_CODEC_DIGITAL_ANALOG_READY);

		MM_INFO("Enable Handset Mic bias\n");
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_PWM_TCXO);
		/* enable MI2S TX master block */
		/* enable MI2S TX bit clock */
		clk_set_rate(drv->tx_mclk,
			SNDDEV_ICODEC_CLK_RATE(8000));
		clk_enable(drv->tx_mclk);
		clk_enable(drv->tx_sclk);

		MM_INFO("configure ADIE TX path\n");
		/* Configure ADIE */
		adie_codec_open(&debug_tx_lb_profile, &debugfs_tx_adie);
		adie_codec_setpath(debugfs_tx_adie, 8000, 256);
		adie_codec_proceed_stage(debugfs_tx_adie,
		ADIE_CODEC_DIGITAL_ANALOG_READY);
	} else {
		/* Disable ADIE */
		adie_codec_proceed_stage(debugfs_rx_adie,
		ADIE_CODEC_DIGITAL_OFF);
		adie_codec_close(debugfs_rx_adie);
		adie_codec_proceed_stage(debugfs_tx_adie,
		ADIE_CODEC_DIGITAL_OFF);
		adie_codec_close(debugfs_tx_adie);

		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_OFF);

		/* Disable MI2S RX master block */
		/* Disable MI2S RX bit clock */
		clk_disable(drv->rx_sclk);
		clk_disable(drv->rx_mclk);

		/* Disable MI2S TX master block */
		/* Disable MI2S TX bit clock */
		clk_disable(drv->tx_sclk);
		clk_disable(drv->tx_mclk);
	}
}

static void debugfs_afe_loopback(u32 loop)
{
	int trc;
	struct msm_afe_config afe_config;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	struct lpa_codec_config lpa_config;

	if (loop) {
		/* Vote for SMPS mode*/
		pmapp_smps_mode_vote(SMPS_AUDIO_PLAYBACK_ID,
				PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_PWM);

		/* enable MI2S RX master block */
		/* enable MI2S RX bit clock */
		trc = clk_set_rate(drv->rx_mclk,
		SNDDEV_ICODEC_CLK_RATE(8000));
		if (IS_ERR_VALUE(trc))
			MM_ERR("failed to set clk rate\n");
		clk_enable(drv->rx_mclk);
		clk_enable(drv->rx_sclk);
		clk_enable(drv->lpa_p_clk);
		clk_enable(drv->lpa_codec_clk);
		clk_enable(drv->lpa_core_clk);
		/* Enable LPA sub system
		 */
		drv->lpa = lpa_get();
		if (!drv->lpa)
			MM_ERR("failed to enable lpa\n");
		lpa_config.sample_rate = 8000;
		lpa_config.sample_width = 16;
		lpa_config.output_interface = LPA_OUTPUT_INTF_WB_CODEC;
		lpa_config.num_channels = 1;
		lpa_cmd_codec_config(drv->lpa, &lpa_config);
		/* Set audio interconnect reg to LPA */
		audio_interct_codec(AUDIO_INTERCT_LPA);
		mi2s_set_codec_output_path(MI2S_CHAN_MONO_PACKED, WT_16_BIT);
		MM_INFO("configure ADIE RX path\n");
		/* Configure ADIE */
		adie_codec_open(&debug_rx_profile, &debugfs_rx_adie);
		adie_codec_setpath(debugfs_rx_adie, 8000, 256);
		lpa_cmd_enable_codec(drv->lpa, 1);

		/* Start AFE for RX */
		afe_config.sample_rate = 0x8;
		afe_config.channel_mode = 1;
		afe_config.volume = AFE_VOLUME_UNITY;
		MM_INFO("enable afe\n");
		trc = afe_enable(AFE_HW_PATH_CODEC_RX, &afe_config);
		if (IS_ERR_VALUE(trc))
			MM_ERR("fail to enable afe RX\n");
		adie_codec_proceed_stage(debugfs_rx_adie,
		ADIE_CODEC_DIGITAL_READY);
		adie_codec_proceed_stage(debugfs_rx_adie,
		ADIE_CODEC_DIGITAL_ANALOG_READY);

		/* Vote for PWM mode*/
		pmapp_smps_mode_vote(SMPS_AUDIO_RECORD_ID,
			PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_PWM);

		MM_INFO("Enable Handset Mic bias\n");
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_PWM_TCXO);

		/* enable MI2S TX master block */
		/* enable MI2S TX bit clock */
		clk_set_rate(drv->tx_mclk,
			SNDDEV_ICODEC_CLK_RATE(8000));
		clk_enable(drv->tx_mclk);
		clk_enable(drv->tx_sclk);
		/* Set MI2S */
		mi2s_set_codec_input_path(MI2S_CHAN_MONO_PACKED, WT_16_BIT);
		MM_INFO("configure ADIE TX path\n");
		/* Configure ADIE */
		adie_codec_open(&debug_tx_profile, &debugfs_tx_adie);
		adie_codec_setpath(debugfs_tx_adie, 8000, 256);
		adie_codec_proceed_stage(debugfs_tx_adie,
		ADIE_CODEC_DIGITAL_READY);
		adie_codec_proceed_stage(debugfs_tx_adie,
		ADIE_CODEC_DIGITAL_ANALOG_READY);
		/* Start AFE for TX */
		afe_config.sample_rate = 0x8;
		afe_config.channel_mode = 1;
		afe_config.volume = AFE_VOLUME_UNITY;
		trc = afe_enable(AFE_HW_PATH_CODEC_TX, &afe_config);
		if (IS_ERR_VALUE(trc))
			MM_ERR("failed to enable AFE TX\n");
		/* Set the volume level to non unity, to avoid
		   loopback effect */
		afe_device_volume_ctrl(AFE_HW_PATH_CODEC_RX, 0x0500);

		/* enable afe loopback */
		afe_loopback(1);
		MM_INFO("AFE loopback enabled\n");
	} else {
		/* disable afe loopback */
		afe_loopback(0);
		/* Remove the vote for SMPS mode*/
		pmapp_smps_mode_vote(SMPS_AUDIO_PLAYBACK_ID,
			PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_DONTCARE);

		/* Disable ADIE */
		adie_codec_proceed_stage(debugfs_rx_adie,
		ADIE_CODEC_DIGITAL_OFF);
		adie_codec_close(debugfs_rx_adie);
		/* Disable AFE for RX */
		afe_disable(AFE_HW_PATH_CODEC_RX);

		/* Disable LPA Sub system */
		lpa_cmd_enable_codec(drv->lpa, 0);
		lpa_put(drv->lpa);

		/* Disable LPA clocks */
		clk_disable(drv->lpa_p_clk);
		clk_disable(drv->lpa_codec_clk);
		clk_disable(drv->lpa_core_clk);

		/* Disable MI2S RX master block */
		/* Disable MI2S RX bit clock */
		clk_disable(drv->rx_sclk);
		clk_disable(drv->rx_mclk);

		pmapp_smps_mode_vote(SMPS_AUDIO_RECORD_ID,
			PMAPP_VREG_S4, PMAPP_SMPS_MODE_VOTE_DONTCARE);

		/* Disable AFE for TX */
		afe_disable(AFE_HW_PATH_CODEC_TX);

		/* Disable ADIE */
		adie_codec_proceed_stage(debugfs_tx_adie,
		ADIE_CODEC_DIGITAL_OFF);
		adie_codec_close(debugfs_tx_adie);
		/* Disable MI2S TX master block */
		/* Disable MI2S TX bit clock */
		clk_disable(drv->tx_sclk);
		clk_disable(drv->tx_mclk);
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_OFF);
		MM_INFO("AFE loopback disabled\n");
	}
}

static ssize_t snddev_icodec_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *lb_str = filp->private_data;
	char cmd;

	if (get_user(cmd, ubuf))
		return -EFAULT;

	MM_INFO("%s %c\n", lb_str, cmd);

	if (!strcmp(lb_str, "adie_loopback")) {
		switch (cmd) {
		case '1':
			debugfs_adie_loopback(1);
			break;
		case '0':
			debugfs_adie_loopback(0);
			break;
		}
	} else if (!strcmp(lb_str, "afe_loopback")) {
		switch (cmd) {
		case '1':
			debugfs_afe_loopback(1);
			break;
		case '0':
			debugfs_afe_loopback(0);
			break;
		}
	}

	return cnt;
}

static const struct file_operations snddev_icodec_debug_fops = {
	.open = snddev_icodec_debug_open,
	.write = snddev_icodec_debug_write
};
#endif

void dump_mem(const char *name, unsigned long addr,
		     unsigned long size) 
 {
    unsigned long i, end;
    char str[16*3 + 1];

    end = addr + size;
	printk("%s(0x%08lx to 0x%08lx size %ld)\n", name, addr, end, size);
    
    while (addr < end) {
		printk("%04lx:", addr & 0xffff);
        memset(str, 0, sizeof(str));
        for (i = 0; i < 16; i++) {
			sprintf(str + i * 3, " %02x", *(uint8_t*)addr);
            addr++;
            if (addr >= end) {
                break;
            }
        }
		printk("%s\n", str);
    }
}

static int icodec_get_indices(
    struct voc_filter_indices_struct *indices,
    uint8_t *buf,
    uint32_t size)
{
    int rc;
    do {
        if (size < sizeof(struct voc_filter_indices_struct)) {
    		MM_ERR("get voc_filter_indices_struct: invalid size\n");
    		rc = -EINVAL;
            break;
        }
    	memcpy(indices, buf, sizeof(struct voc_filter_indices_struct));

        rc = sizeof(struct voc_filter_indices_struct);
    }while(0);

    return rc;
}

static int icodec_get_guid(
    struct acm_guid *guid,
    uint8_t *buf,
    uint32_t size)
{
    int rc;
    do {
        if (size < sizeof(struct acm_guid)) {
    		MM_ERR("get acm_guid: invalid size\n");
    		rc = -EINVAL;
            break;
        }
    	memcpy(guid, buf, sizeof(struct acm_guid));

        rc = sizeof(struct acm_guid);
    }while(0);

    return rc;
}

static acm_calunit_enum icodec_get_cal_unit(struct acm_guid *guid) {
  /*initialize calunit*/
  acm_calunit_enum calunit = CALUNIT_UNKNOWN;
  int i=0;
  if ( guid == NULL )
  {
    return calunit;
  }
  /*try to match with calunit list we have*/
  for(i=0;i<CALUNIT_MAX;i++)
  {
    if( memcmp((void *)&calunitlist[i],(void *)guid,ACM_GUID_SIZE) == 0)
    {
      /*found. break here.*/
      calunit = (acm_calunit_enum) i;
      break;
    }
  }
  return calunit;
}

int get_dev_act_data(struct voc_filter_indices_struct* indices, char* buf, int size) {
    int rc = 0;
    struct dev_action_group *action_group;
    struct icodec_dev_t *dev;
	struct snddev_icodec_state *icodec = NULL;
    struct adie_codec_hwsetting_entry *hwsetting;
    struct adie_codec_action_unit *actions = NULL;
    u32 action_sz = 0;
    action_group = (struct dev_action_group *)buf;

    do {
        if (!action_group) {
        	MM_ERR("invalid action_group\n");
            rc = -EINVAL;
            break;
        }
        if (size < sizeof(struct dev_action_group)) {
        	MM_ERR("invalid size\n");
            rc = -EINVAL;            
            break;
        }
        size -= sizeof(struct dev_action_group);
        
        list_for_each_entry(dev, &icodec_misc_dev.list, list) {
            if (indices->snd_device == dev->dev_info->acdb_id) {
                icodec = (struct snddev_icodec_state*)dev->dev_info->private_data;
                break;
            }
        }
        if (!icodec) {
        	MM_ERR("unknow dev\n");
            rc = -EINVAL;            
            break;
        }
        
        hwsetting = icodec->data->profile->settings;
        /* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. { */
        switch (indices->snd_method) {
            case SND_METHOD_VOICE:
                actions = hwsetting->voc;
                action_sz = hwsetting->voc_sz;
	            break;
            case SND_METHOD_MIDI:
                actions = hwsetting->media;
                action_sz = hwsetting->media_sz;
                break;
            }
        if (!actions || !action_sz) {
            actions = hwsetting->actions;
            action_sz = hwsetting->action_sz;
        }
        /* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. } */
        if (size) {
            action_group->num = size / sizeof(struct adie_codec_action_unit);
            size = action_group->num * sizeof(struct adie_codec_action_unit);
            memcpy(action_group->group, actions, size);
        }
        else {
            action_group->num = action_sz;
        }
        
    } while(0);

	MM_INFO("rc:%d\n", rc);
    return rc;
}

int set_dev_act_data(struct voc_filter_indices_struct* indices, char* buf, int size)
{
    int rc = 0;
    u32 num;
    struct dev_action_group *action_group;
    struct icodec_dev_t *dev;
	struct snddev_icodec_state *icodec = NULL;
    struct adie_codec_hwsetting_entry *hwsetting;
    struct adie_codec_action_unit *actions = NULL;
    u32 action_sz = 0;
    action_group = (struct dev_action_group *)buf;

    do {
        if (!action_group) {
        	MM_ERR("invalid action_group\n");
            rc = -EINVAL;
            break;
        }
        if (size < sizeof(struct dev_action_group)) {
        	MM_ERR("invalid size\n");
            rc = -EINVAL;            
            break;
        }
        size -= sizeof(struct dev_action_group);
        
        list_for_each_entry(dev, &icodec_misc_dev.list, list) {
            if (indices->snd_device == dev->dev_info->acdb_id) {
                icodec = (struct snddev_icodec_state*)dev->dev_info->private_data;
                break;
            }
        }
        if (!icodec) {
        	MM_ERR("unknow dev\n");
            rc = -EINVAL;            
            break;
        }
        
        hwsetting = icodec->data->profile->settings;
        /* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. { */
        switch (indices->snd_method) {
            case SND_METHOD_VOICE:
                actions = hwsetting->voc;
                action_sz = hwsetting->voc_sz;
                break;
            case SND_METHOD_MIDI:
                actions = hwsetting->media;
                action_sz = hwsetting->media_sz;
                break;
            }
        if (!actions || !action_sz) {
            actions = hwsetting->actions;
            action_sz = hwsetting->action_sz;
        }
        /* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. } */
        if (size) {
            num = size / sizeof(struct adie_codec_action_unit);
            num = (num < action_group->num ? num : action_group->num);
            num = (num < action_sz ? num : action_sz);
            size = num * sizeof(struct adie_codec_action_unit);
            memcpy(actions, action_group->group, size);
            //hwsetting->action_sz = num;
        }
        else {
        	MM_ERR("not more data\n");
            rc = -EINVAL;
        }
        
    } while(0);

	MM_INFO("rc:%d\n", rc);
    return rc;
}

int get_dev_voc_vol_limt_data(struct voc_filter_indices_struct* indices, char* buf, int size)
{
    int rc = 0;
    struct icodec_dev_t *dev;
	struct msm_snddev_info *dev_info = NULL;    
	struct dev_voc_vol_limt *limt = (struct dev_voc_vol_limt*)buf;

    do {
        if (size < sizeof(struct dev_voc_vol_limt)) {
        	MM_ERR("invalid size\n");
            rc = -EINVAL;
            break;
        }
        
        list_for_each_entry(dev, &icodec_misc_dev.list, list) {
            if (indices->snd_device == dev->dev_info->acdb_id) {
                dev_info = dev->dev_info;
                break;
            }
        }
        if (!dev_info) {
        	MM_ERR("unknow dev\n");
            rc = -EINVAL;
            break;
        }
        indices->voc_cal_pcm_path_cfg = indices->voc_cal_pcm_path_cfg % 2;
        limt->max = dev_info->max_voc_rx_vol[indices->voc_cal_pcm_path_cfg];
        limt->min = dev_info->min_voc_rx_vol[indices->voc_cal_pcm_path_cfg];
    }while(0);

	MM_INFO("rc:%d\n", rc);
    return rc;
}

int set_dev_voc_vol_limt_data(struct voc_filter_indices_struct* indices, char* buf, int size)
{
    int rc = 0;
    struct icodec_dev_t *dev;
	struct msm_snddev_info *dev_info = NULL;    
	struct dev_voc_vol_limt *limt = (struct dev_voc_vol_limt*)buf;

    do {
        if (size < sizeof(struct dev_voc_vol_limt)) {
        	MM_ERR("invalid size\n");
            rc = -EINVAL;
            break;
        }
        
        list_for_each_entry(dev, &icodec_misc_dev.list, list) {
            if (indices->snd_device == dev->dev_info->acdb_id) {
                dev_info = dev->dev_info;
                break;
            }
        }
        if (!dev_info) {
        	MM_ERR("unknow dev\n");
            rc = -EINVAL;
            break;
        }
        indices->voc_cal_pcm_path_cfg = indices->voc_cal_pcm_path_cfg % 2;
        dev_info->max_voc_rx_vol[indices->voc_cal_pcm_path_cfg] = limt->max;
        dev_info->min_voc_rx_vol[indices->voc_cal_pcm_path_cfg] = limt->min;
    }while(0);

	MM_INFO("rc:%d\n", rc);
    return rc;
}

typedef int (*get_data_func_list_t)(struct voc_filter_indices_struct*, char*, int);
get_data_func_list_t get_data_func_list[] = {
	get_dev_act_data,
	get_dev_voc_vol_limt_data,
};

typedef int (*set_data_func_list_t)(struct voc_filter_indices_struct*, char*, int);
set_data_func_list_t set_data_func_list[] = {
	set_dev_act_data,
	set_dev_voc_vol_limt_data,
};

static long icodec_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int i, rc = 0;
    uint8_t *pointer, *buf = 0;
    uint32_t size;
    struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
    struct icodec_dev_info_t dev_info;
    struct icodec_dev_t *dev;
    struct acm_diag_req req;
    struct voc_filter_indices_struct indices;
    struct acm_guid guid;
    snd_method_type method;
    acm_calunit_enum calunit;
    
    mutex_lock(&icodec_misc_dev.lock);
    
	switch (cmd) {
    case ICODEC_GET_DEV_INFO:
		MM_INFO("ICODEC_GET_DEV_INFO\n");
		if (copy_from_user(&dev_info, 
                (void __user *)arg, sizeof(struct icodec_dev_info_t))) {
			MM_ERR("get arg: invalid pointer\n");
			rc = -EFAULT;
            break;
		}
        
        dev_info.name[0] = 0;
        dev_info.capability = 0xFFFFFFFF;
        dev_info.copp_id = 0xFFFFFFFF;
        dev_info.acdb_id = 0xFFFFFFFF;

        i = 0;
        list_for_each_entry(dev, &icodec_misc_dev.list, list) {
            if (i == dev_info.index) {
                strcpy(dev_info.name, dev->dev_info->name);
                dev_info.acdb_id = dev->dev_info->acdb_id;
                dev_info.copp_id = dev->dev_info->copp_id;
                dev_info.capability = dev->dev_info->capability;
            }
            i++;
        }

		if (copy_to_user((void __user *)arg,
                &dev_info,
                sizeof(struct icodec_dev_info_t))) {
			MM_ERR("set arg: invalid pointer\n");
			rc = -EFAULT;
            break;
		}
        break;
    case ICODEC_CAL_DEV:
		MM_INFO("ICODEC_CAL_DEV_ACT\n");
		if (copy_from_user(&req, 
                (void __user *)arg, sizeof(struct acm_diag_req))) {
			MM_ERR("get arg: invalid pointer\n");
			rc = -EFAULT;
            break;
		}
        size = req.size;
    	buf = kmalloc(size, GFP_KERNEL);
        if (!buf) {
			rc = -ENOMEM;
            break;
        }
    	if (copy_from_user(buf, 
                (void __user *)req.buf, req.size)) {
    		MM_ERR("get buf: invalid pointer\n");
    		rc = -EFAULT;
            break;
    	}
        pointer = buf;
        dump_mem("CAL_DATA", (unsigned long)pointer, size);
        rc = icodec_get_guid(&guid, pointer, size);
        if (rc < 0) {
            break;
        }
        pointer += rc;
        size-= rc;
        rc = icodec_get_indices(&indices, pointer, size);
        if (rc < 0) {
            break;
        }
        pointer += rc;
        size-= rc;
        calunit = icodec_get_cal_unit(&guid);
		MM_INFO("calunit:%d\n", calunit);
        if (calunit < CALUNIT_MAX) {
    		MM_INFO("cmd:%d\n", req.cmd);
            switch (req.cmd) {
            case CMD_SET_STRUCT:
                rc = set_data_func_list[calunit](&indices, pointer, size);
                break;
            case CMD_GET_STRUCT:
                rc = get_data_func_list[calunit](&indices, pointer, size);
                if (!rc) {
            		if (copy_to_user((void __user *)req.buf,
                            buf,
                            req.size)) {
            			MM_ERR("set req buf: invalid buf pointer\n");
            			rc = -EFAULT;
                        break;
            		}            
                }
                break;
            default:
        		MM_ERR("unknown req command\n");
        		rc = -EINVAL;
            }
        }
        else {
            rc = -EINVAL;
        }
        break;
        /* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. { */
        case ICODEC_SET_METHOD:
            MM_INFO("ICODEC_SET_METHOD\n");
            rc = -EINVAL;
            method = (snd_method_type)arg;
            if (method < SND_METHOD_MAX) {
                drv->method = method;
                rc = 0;
            }
            break;
        /* [SIMT-lisensen-110825] for spliting audio parameters for voice call and media sound. } */
	default:
		MM_ERR("unknown ioctl command\n");
		rc = -EINVAL;
		break;
	}
    if (buf) {
        kfree(buf);
    }
    mutex_unlock(&icodec_misc_dev.lock);
	MM_INFO("rc:%d\n", rc);
	return rc;
}

static struct file_operations icodec_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= icodec_ioctl,
};

struct miscdevice icodec_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "icodec",
	.fops	= &icodec_fops,
};

static int __init snddev_icodec_init(void)
{
	s32 rc;
	struct snddev_icodec_drv_state *icodec_drv = &snddev_icodec_drv;

	rc = misc_register(&icodec_misc);
	if (rc)
		return rc;

	rc = platform_driver_register(&snddev_icodec_driver);
	if (IS_ERR_VALUE(rc))
		goto error_platform_driver;
	icodec_drv->rx_mclk = clk_get(NULL, "mi2s_codec_rx_m_clk");
	if (IS_ERR(icodec_drv->rx_mclk))
		goto error_rx_mclk;
	icodec_drv->rx_sclk = clk_get(NULL, "mi2s_codec_rx_s_clk");
	if (IS_ERR(icodec_drv->rx_sclk))
		goto error_rx_sclk;
	icodec_drv->tx_mclk = clk_get(NULL, "mi2s_codec_tx_m_clk");
	if (IS_ERR(icodec_drv->tx_mclk))
		goto error_tx_mclk;
	icodec_drv->tx_sclk = clk_get(NULL, "mi2s_codec_tx_s_clk");
	if (IS_ERR(icodec_drv->tx_sclk))
		goto error_tx_sclk;
	icodec_drv->lpa_codec_clk = clk_get(NULL, "lpa_codec_clk");
	if (IS_ERR(icodec_drv->lpa_codec_clk))
		goto error_lpa_codec_clk;
	icodec_drv->lpa_core_clk = clk_get(NULL, "lpa_core_clk");
	if (IS_ERR(icodec_drv->lpa_core_clk))
		goto error_lpa_core_clk;
	icodec_drv->lpa_p_clk = clk_get(NULL, "lpa_pclk");
	if (IS_ERR(icodec_drv->lpa_p_clk))
		goto error_lpa_p_clk;

#ifdef CONFIG_DEBUG_FS
	debugfs_sdev_dent = debugfs_create_dir("snddev_icodec", 0);
	if (debugfs_sdev_dent) {
		debugfs_afelb = debugfs_create_file("afe_loopback",
		S_IFREG | S_IWUGO, debugfs_sdev_dent,
		(void *) "afe_loopback", &snddev_icodec_debug_fops);
		debugfs_adielb = debugfs_create_file("adie_loopback",
		S_IFREG | S_IWUGO, debugfs_sdev_dent,
		(void *) "adie_loopback", &snddev_icodec_debug_fops);
	}
#endif
	mutex_init(&icodec_drv->rx_lock);
	mutex_init(&icodec_drv->tx_lock);
	mutex_init(&icodec_drv->lb_lock);
	icodec_drv->rx_active = 0;
	icodec_drv->tx_active = 0;
	icodec_drv->lb_active = 0;
	icodec_drv->lpa = NULL;
	wake_lock_init(&icodec_drv->tx_idlelock, WAKE_LOCK_IDLE,
			"snddev_tx_idle");
	wake_lock_init(&icodec_drv->rx_idlelock, WAKE_LOCK_IDLE,
			"snddev_rx_idle");
	wake_lock_init(&icodec_drv->lb_idlelock, WAKE_LOCK_IDLE,
			"snddev_lb_idle");
	return 0;

error_lpa_p_clk:
	clk_put(icodec_drv->lpa_core_clk);
error_lpa_core_clk:
	clk_put(icodec_drv->lpa_codec_clk);
error_lpa_codec_clk:
	clk_put(icodec_drv->tx_sclk);
error_tx_sclk:
	clk_put(icodec_drv->tx_mclk);
error_tx_mclk:
	clk_put(icodec_drv->rx_sclk);
error_rx_sclk:
	clk_put(icodec_drv->rx_mclk);
error_rx_mclk:
	platform_driver_unregister(&snddev_icodec_driver);
error_platform_driver:

	MM_ERR("encounter error\n");
	return -ENODEV;
}

static void __exit snddev_icodec_exit(void)
{
	struct snddev_icodec_drv_state *icodec_drv = &snddev_icodec_drv;

#ifdef CONFIG_DEBUG_FS
	if (debugfs_afelb)
		debugfs_remove(debugfs_afelb);
	if (debugfs_adielb)
		debugfs_remove(debugfs_adielb);
	if (debugfs_sdev_dent)
		debugfs_remove(debugfs_sdev_dent);
#endif
	platform_driver_unregister(&snddev_icodec_driver);

	clk_put(icodec_drv->rx_sclk);
	clk_put(icodec_drv->rx_mclk);
	clk_put(icodec_drv->tx_sclk);
	clk_put(icodec_drv->tx_mclk);
	return;
}

module_init(snddev_icodec_init);
module_exit(snddev_icodec_exit);

MODULE_DESCRIPTION("ICodec Sound Device driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
