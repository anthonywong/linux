/* linux/sound/soc/msm/qsd8k-pcm.c
 *
 * Copyright (c) 2009-2010 Code Aurora Forum. All rights reserved.
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <asm/dma.h>
#include <linux/dma-mapping.h>

#include "qsdv2-pcm.h"
static void snd_qsd_timer(unsigned long data);

static void snd_qsd_timer(unsigned long data)
{
	struct qsd_audio *prtd = (struct qsd_audio *)data;

	if (!prtd->enabled)
		return;
	prtd->timerintcnt++;
	snd_pcm_period_elapsed(prtd->substream);
	if (prtd->enabled)
		add_timer(&prtd->timer);
}

static int rc = 1;

#define SND_DRIVER        "snd_qsd"
#define MAX_PCM_DEVICES	SNDRV_CARDS
#define MAX_PCM_SUBSTREAMS 1

struct snd_qsd {
	struct snd_card *card;
	struct snd_pcm *pcm;
};

struct qsd_ctl qsd_glb_ctl;
EXPORT_SYMBOL(qsd_glb_ctl);
struct audio_locks the_locks;
EXPORT_SYMBOL(the_locks);


static struct snd_pcm_hardware qsd_pcm_playback_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED,
	.formats = USE_FORMATS,
	.rates = USE_RATE,
	.rate_min = USE_RATE_MIN,
	.rate_max = USE_RATE_MAX,
	.channels_min = USE_CHANNELS_MIN,
	.channels_max = USE_CHANNELS_MAX,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = MAX_PERIOD_SIZE,
	.periods_min = USE_PERIODS_MIN,
	.periods_max = USE_PERIODS_MAX,
	.fifo_size = 0,
};

static struct snd_pcm_hardware qsd_pcm_capture_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED,
	.formats = USE_FORMATS,
	.rates = USE_RATE,
	.rate_min = USE_RATE_MIN,
	.rate_max = USE_RATE_MAX,
	.channels_min = USE_CHANNELS_MIN,
	.channels_max = USE_CHANNELS_MAX,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = MAX_PERIOD_SIZE,
	.periods_min = USE_PERIODS_MIN,
	.periods_max = USE_PERIODS_MAX,
	.fifo_size = 0,
};

int qsd_audio_volume_update(struct qsd_audio *prtd)
{
	int rc = 0;

	return rc;
}

void qsd_pcm_playback_eos_cb(void *data)
{
	struct qsd_audio *prtd = data;

	prtd->eos_ack = 1;
	wake_up(&the_locks.eos_wait);
}

void qsd_pcm_playback_buf_done_cb(void *data)
{
	struct qsd_audio *prtd = data;

	prtd->intcnt++;
	prtd->pcm_irq_pos += prtd->pcm_count;
}

void qsd_pcm_capture_eos_cb(void *data)
{
}

void qsd_pcm_capture_buf_done_cb(void *data)
{
}

static int qsd_pcm_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;
	unsigned long expiry = 0;

	prtd->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	prtd->pcm_count = snd_pcm_lib_period_bytes(substream);
	prtd->out_sample_rate = runtime->rate;
	prtd->out_channel_mode = runtime->channels;
	prtd->pcm_irq_pos = 0;
	prtd->pcm_buf_pos = 0;
	atomic_set(&prtd->copy_count, 0);

	if (prtd->enabled)
		return 0;

	prtd->dsp_handle = q6audio_open_pcm(prtd->pcm_count,
					prtd->out_sample_rate,
					prtd->mode_channel_mode,
					AUDIO_FLAG_WRITE);
	if (!prtd->dsp_handle)
		return -EAGAIN;

	prtd->dsp_handle->playback_eos_cb = qsd_pcm_playback_eos_cb;
	prtd->dsp_handle->playback_eos_data = prtd;

	prtd->dsp_handle->playback_buf_done_cb = qsd_pcm_playback_buf_done_cb;
	prtd->dsp_handle->playback_buf_done_data = prtd;

	prtd->enabled = 1;
	expiry = ((unsigned long)((prtd->pcm_count * 1000)
		/(runtime->rate * runtime->channels * 2)));
	prtd->timer.expires = jiffies + msecs_to_jiffies(expiry);
	setup_timer(&prtd->timer, snd_qsd_timer, (unsigned long)prtd);
	add_timer(&prtd->timer);

	return rc;
}

static int qsd_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t
qsd_pcm_playback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;

	if (prtd->pcm_irq_pos >= prtd->pcm_size)
		prtd->pcm_irq_pos = 0;
	return bytes_to_frames(runtime, (prtd->pcm_irq_pos));
}

static int qsd_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd;
	int ret = 0;

	prtd = kzalloc(sizeof(struct qsd_audio), GFP_KERNEL);
	if (prtd == NULL) {
		ret = -ENOMEM;
		return ret;
	}
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		printk(KERN_INFO "Stream = SNDRV_PCM_STREAM_PLAYBACK\n");
		runtime->hw = qsd_pcm_playback_hardware;
		prtd->dir = SNDRV_PCM_STREAM_PLAYBACK;
	} else {
		printk(KERN_INFO "Stream = SNDRV_PCM_STREAM_CAPTURE\n");
		runtime->hw = qsd_pcm_capture_hardware;
		prtd->dir = SNDRV_PCM_STREAM_CAPTURE;
	}
	prtd->substream = substream;

	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		kfree(prtd);
		return ret;
	}

	runtime->private_data = prtd;

	prtd->enabled = 0;

	return 0;
}

static int qsd_pcm_playback_copy(struct snd_pcm_substream *substream, int a,
				 snd_pcm_uframes_t hwoff, void __user *buf,
				 snd_pcm_uframes_t frames)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;
	struct audio_client *ac = prtd->dsp_handle;
	struct audio_buffer *ab;
	int fbytes = 0;
	size_t xfer;
	int count;
	int rc;

	fbytes = frames_to_bytes(runtime, frames);
	count = fbytes;

	while (count > 0) {
		ab = ac->buf + ac->cpu_buf;

		if (ab->used)
			wait_event(ac->wait, (ab->used == 0));

		xfer = count;
		if (xfer > ab->size)
			xfer = ab->size;

		if (copy_from_user(ab->data, buf, xfer))
			return -EFAULT;

		buf += xfer;
		count -= xfer;

		ab->used = 1;
		ab->actual_size = xfer;
		q6audio_write(ac, ab);
		ac->cpu_buf ^= 1;
	}

	prtd->pcm_buf_pos += fbytes;

	prtd->buffer_cnt++;

	mutex_lock(&the_locks.mixer_lock);
	if (qsd_glb_ctl.update) {
		rc = qsd_audio_volume_update(prtd);
		qsd_glb_ctl.update = 0;
	}
	mutex_unlock(&the_locks.mixer_lock);

	return 0;
}

static int qsd_pcm_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;
	int ret = 0;

	if (prtd->enabled) {
		q6audio_command(prtd->dsp_handle,
			ADSP_AUDIO_IOCTL_CMD_STREAM_EOS);

		q6audio_close(prtd->dsp_handle);

		ret = wait_event_interruptible(the_locks.eos_wait,
					prtd->eos_ack);

		if (!prtd->eos_ack)
			pr_err("EOS Failed\n");
	}

	prtd->enabled = 0;
	del_timer_sync(&prtd->timer);
	prtd->eos_ack = 0;

	/*
	 * TODO: Deregister the async callback handler.
	 */
	kfree(prtd);

	return ret;
}

static int qsd_pcm_capture_copy(struct snd_pcm_substream *substream, int a,
				 snd_pcm_uframes_t hwoff, void __user *buf,
				 snd_pcm_uframes_t frames)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;
	struct audio_client *ac = prtd->dsp_handle;
	struct audio_buffer *ab;
	int fbytes = 0;
	size_t xfer = 0;
	int count;
	int rc = 0;

	/* pr_err("+%s:\n", __func__); */
	fbytes = frames_to_bytes(runtime, frames);
	count = fbytes;

	while (count > 0) {
		ab = ac->buf + ac->cpu_buf;

		if (ab->used)
			wait_event(ac->wait, (ab->used == 0));

		xfer = count;
		if (xfer > ab->size)
			xfer = ab->size;

		if (copy_to_user(buf, ab->data, xfer))
			return -EFAULT;

		buf += xfer;
		count -= xfer;

		ab->used = 1;
		q6audio_read(ac, ab);
		ac->cpu_buf ^= 1;
	}

	prtd->pcm_buf_pos += fbytes;
	if (xfer < fbytes)
		return -EIO;

	/* pr_err("+%s:\n", __func__); */

	return rc;
}

static snd_pcm_uframes_t
qsd_pcm_capture_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;

	return bytes_to_frames(runtime, (prtd->pcm_irq_pos));
}

static int qsd_pcm_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;

	prtd->enabled = 0;
	del_timer_sync(&prtd->timer);

	/*
	 * TODO: Deregister the async callback handler.
	 */

	if (prtd->enabled)
		q6audio_close(prtd->dsp_handle);

	kfree(prtd);

	return 0;
}

static int qsd_pcm_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct qsd_audio *prtd = runtime->private_data;
	struct adsp_audio_standard_format *fmt;
	struct adsp_open_command rpc;
	int rc = 0;
	unsigned long expiry = 0;

	prtd->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	prtd->pcm_count = snd_pcm_lib_period_bytes(substream);
	prtd->out_sample_rate = runtime->rate;
	prtd->out_channel_mode = runtime->channels;
	prtd->pcm_irq_pos = 0;
	prtd->pcm_buf_pos = 0;

	if (prtd->enabled)
		return 0;

	prtd->dsp_handle = q6audio_open_pcm(prtd->pcm_count,
						prtd->out_sample_rate,
						prtd->out_channel_mode,
						AUDIO_FLAG_READ);
	if (!prtd->dsp_handle)
		return -EAGAIN;


	prtd->dsp_handle->capture_eos_cb = qsd_pcm_capture_eos_cb;
	prtd->dsp_handle->capture_eos_data = prtd;

	prtd->dsp_handle->capture_buf_done_cb = qsd_pcm_capture_buf_done_cb;
	prtd->dsp_handle->capture_buf_done_data = prtd;


	prtd->enabled = 1;
	expiry = ((unsigned long)((prtd->pcm_count * 1000)
		/(runtime->rate * runtime->channels * 2)));
	prtd->timer.expires = jiffies + msecs_to_jiffies(expiry);
	setup_timer(&prtd->timer, snd_qsd_timer, (unsigned long)prtd);
	add_timer(&prtd->timer);

	return rc;
}


static int qsd_pcm_copy(struct snd_pcm_substream *substream, int a,
			snd_pcm_uframes_t hwoff, void __user *buf,
			snd_pcm_uframes_t frames)
{
	int ret = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = qsd_pcm_playback_copy(substream, a, hwoff, buf, frames);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = qsd_pcm_capture_copy(substream, a, hwoff, buf, frames);
	return ret;
}

static int qsd_pcm_close(struct snd_pcm_substream *substream)
{
	int ret = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = qsd_pcm_playback_close(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = qsd_pcm_capture_close(substream);
	return ret;
}
static int qsd_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = qsd_pcm_playback_prepare(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = qsd_pcm_capture_prepare(substream);
	return ret;
}

static snd_pcm_uframes_t qsd_pcm_pointer(struct snd_pcm_substream *substream)
{
	snd_pcm_uframes_t ret = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = qsd_pcm_playback_pointer(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = qsd_pcm_capture_pointer(substream);
	return ret;
}

int qsd_pcm_hw_params(struct snd_pcm_substream *substream,
		      struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (substream->pcm->device & 1) {
		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	}
	return 0;
}

struct snd_pcm_ops qsd_pcm_ops = {
	.open = qsd_pcm_open,
	.copy = qsd_pcm_copy,
	.hw_params = qsd_pcm_hw_params,
	.close = qsd_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.prepare = qsd_pcm_prepare,
	.trigger = qsd_pcm_trigger,
	.pointer = qsd_pcm_pointer,
};
EXPORT_SYMBOL_GPL(qsd_pcm_ops);

static int qsd_pcm_remove(struct platform_device *devptr)
{
	struct snd_soc_device *socdev = platform_get_drvdata(devptr);
	snd_soc_free_pcms(socdev);
	kfree(socdev->card->codec);
	platform_set_drvdata(devptr, NULL);
	return 0;
}

static int qsd_pcm_new(struct snd_card *card,
			struct snd_soc_dai *codec_dai,
			struct snd_pcm *pcm)
{
	int ret;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	ret = snd_pcm_new_stream(pcm, SNDRV_PCM_STREAM_PLAYBACK,
				PLAYBACK_STREAMS);
	if (ret)
		return ret;
	ret = snd_pcm_new_stream(pcm, SNDRV_PCM_STREAM_CAPTURE,
				CAPTURE_STREAMS);
	if (ret)
		return ret;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &qsd_pcm_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &qsd_pcm_ops);
	return ret;
}

struct snd_soc_platform qsd_soc_platform = {
	.name		= "qsd-audio",
	.remove         = qsd_pcm_remove,
	.pcm_ops 	= &qsd_pcm_ops,
	.pcm_new	= qsd_pcm_new,
};
EXPORT_SYMBOL(qsd_soc_platform);

static int __init qsd_soc_platform_init(void)
{
	return snd_soc_register_platform(&qsd_soc_platform);
}
module_init(qsd_soc_platform_init);

static void __exit qsd_soc_platform_exit(void)
{
	snd_soc_unregister_platform(&qsd_soc_platform);
}
module_exit(qsd_soc_platform_exit);

MODULE_DESCRIPTION("PCM module platform driver");
MODULE_LICENSE("GPL v2");
