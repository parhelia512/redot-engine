/**************************************************************************/
/*  video_stream_theora.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             REDOT ENGINE                               */
/*                        https://redotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2024-present Redot Engine contributors                   */
/*                                          (see REDOT_AUTHORS.md)        */
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/os/thread.h"
#include "scene/resources/video_stream.h"

#include <theora/theoradec.h>
#include <vorbis/codec.h>

class ImageTexture;

class VideoStreamPlaybackTheora : public VideoStreamPlayback {
	GDCLASS(VideoStreamPlaybackTheora, VideoStreamPlayback);

	Image::Format format = Image::Format::FORMAT_L8;
	Vector<uint8_t> frame_data;
	int frames_pending = 0;
	Ref<FileAccess> file;
	String file_name;
	Point2i size;
	Rect2i region;

	float *audio_buffer = nullptr;
	int audio_buffer_size = 0;
	int audio_ptr_start = 0;
	int audio_ptr_end = 0;

	int buffer_data();
	int queue_page(ogg_page *page);
	int read_page(ogg_page *page);
	int feed_pages();
	double get_page_time(ogg_page *page);
	int64_t seek_streams(double p_time, int64_t &video_granulepos, int64_t &audio_granulepos);
	void find_streams(th_setup_info *&ts);
	void read_headers(th_setup_info *&ts);
	void video_write(th_ycbcr_buffer yuv);
	double get_time() const;

	bool theora_eos = false;
	bool vorbis_eos = false;

	ogg_sync_state oy;
	ogg_stream_state vo;
	ogg_stream_state to;
	th_info ti;
	th_comment tc;
	th_dec_ctx *td = nullptr;
	vorbis_info vi = {};
	vorbis_dsp_state vd;
	vorbis_block vb;
	vorbis_comment vc;
	th_pixel_fmt px_fmt;
	double frame_duration = 0;
	double stream_length = 0;
	int64_t stream_data_offset = 0;
	int64_t stream_data_size = 0;

	int pp_level_max = 0;
	int pp_level = 0;
	int pp_inc = 0;

	bool playing = false;
	bool paused = false;

	bool dup_frame = false;
	bool has_video = false;
	bool has_audio = false;
	bool video_ready = false;
	bool video_done = false;
	bool audio_done = false;

	double time = 0;
	double next_frame_time = 0;
	double current_frame_time = 0;
	double delay_compensation = 0;

	Ref<ImageTexture> texture;

	int audio_track = 0;

protected:
	void clear();

	_FORCE_INLINE_ bool send_audio() {
		if (audio_ptr_end > 0) {
			int mixed = mix_callback(mix_udata, &audio_buffer[audio_ptr_start * vi.channels], audio_ptr_end - audio_ptr_start);
			audio_ptr_start += mixed;
			if (audio_ptr_start == audio_ptr_end) {
				audio_ptr_start = 0;
				audio_ptr_end = 0;
			} else {
				return false;
			}
		}
		return true;
	}

public:
	virtual void play() override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual void set_paused(bool p_paused) override;
	virtual bool is_paused() const override;

	virtual double get_length() const override;

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	void set_file(const String &p_file);

	virtual Ref<Texture2D> get_texture() const override;
	virtual void update(double p_delta) override;

	virtual int get_channels() const override;
	virtual int get_mix_rate() const override;

	virtual void set_audio_track(int p_idx) override;

	VideoStreamPlaybackTheora();
	~VideoStreamPlaybackTheora();
};

class VideoStreamTheora : public VideoStream {
	GDCLASS(VideoStreamTheora, VideoStream);

protected:
	static void _bind_methods();

public:
	Ref<VideoStreamPlayback> instantiate_playback() override {
		Ref<VideoStreamPlaybackTheora> pb = memnew(VideoStreamPlaybackTheora);
		pb->set_audio_track(audio_track);
		pb->set_file(file);
		return pb;
	}

	void set_audio_track(int p_track) override { audio_track = p_track; }

	VideoStreamTheora() { audio_track = 0; }
};

class ResourceFormatLoaderTheora : public ResourceFormatLoader {
public:
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
};
