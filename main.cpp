#include "../vendor/sokol/sokol_app.h"
#include "../vendor/sokol/sokol_gfx.h"
#include "../vendor/sokol/sokol_time.h"
#include "../vendor/sokol/sokol_audio.h"
#include "../vendor/sokol/sokol_glue.h"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui-fonts/fonts.h" // https://github.com/abdes/asap_app_imgui/tree/master/main/src/ui/fonts
#include "../vendor/imgui-fonts/material_design_icons.h" 
#include "../vendor/sokol/util/sokol_imgui.h"

#include "app/App.hpp"
#include "app/Platform.hpp"
#include "engine/core/Log.hpp"

#include <thread>

extern unsigned char app_icon_256_data[256 * 256 * 4 + 1];

constexpr char TAG[] = "main";

static sns::App app;
static int64_t app_last_render_checkpoint = 0;

static sg_image app_font_image;
static bool app_font_initialized;
static sg_pass_action pass_action;

static bool audio_initialized;
static std::vector<sns::PlatformEvent> platform_events;
static std::mutex platform_events_mutex;


static void destroyFonts() {
	auto& io = ImGui::GetIO();
	io.Fonts->Clear();

	if (app_font_initialized)
		sg_destroy_image(app_font_image);

	app_font_initialized = false;
}

static void rebuildFonts() {
	destroyFonts();

	// configure Dear ImGui with our own embedded font
	auto& io = ImGui::GetIO();

	ImFontConfig text_font_cfg;
	text_font_cfg.FontDataOwnedByAtlas = false;
	text_font_cfg.OversampleH = 2;
	text_font_cfg.OversampleV = 2;
	text_font_cfg.RasterizerMultiply = 1.5f;

	float size_pixels = 20.0f;
	ImFont* roboto = io.Fonts->AddFontFromMemoryCompressedTTF(
		asap::debug::ui::Fonts::ROBOTO_LIGHT_COMPRESSED_DATA,
		asap::debug::ui::Fonts::ROBOTO_LIGHT_COMPRESSED_SIZE,
		size_pixels, &text_font_cfg);


	ImFontConfig icons_font_cfg;
	icons_font_cfg.FontDataOwnedByAtlas = false;
	icons_font_cfg.OversampleH = 2;
	icons_font_cfg.OversampleV = 2;
	icons_font_cfg.RasterizerMultiply = 1.5f;
	icons_font_cfg.GlyphOffset.y = 1.5f;
	icons_font_cfg.MergeMode = true;

	const ImWchar icons_ranges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 }; // Will not be copied by AddFont* so keep in scope.
	ImFont* icons = io.Fonts->AddFontFromMemoryCompressedTTF(
		asap::debug::ui::Fonts::MATERIAL_DESIGN_ICONS_COMPRESSED_DATA,
		asap::debug::ui::Fonts::MATERIAL_DESIGN_ICONS_COMPRESSED_SIZE,
		size_pixels, &icons_font_cfg, icons_ranges);

	io.Fonts->Build();



	// create font texture for the custom font
	unsigned char* font_pixels;
	int font_width, font_height;
	io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
	sg_image_desc img_desc = { };
	img_desc.width = font_width;
	img_desc.height = font_height;
	img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
	img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
	img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
	img_desc.min_filter = SG_FILTER_LINEAR;
	img_desc.mag_filter = SG_FILTER_LINEAR;
	img_desc.data.subimage[0][0].ptr = font_pixels;
	img_desc.data.subimage[0][0].size = size_t(font_width) * size_t(font_height) * 4;
	app_font_image = sg_make_image(&img_desc);

	io.Fonts->TexID = (ImTextureID)(uintptr_t)app_font_image.id;
}

static void audio_callback(float* buffer, int num_frames, int num_channels) {
	app.engine().fill(buffer, num_frames, num_channels);
}

static void destroyAudioBackend() {
	if (audio_initialized) 
		saudio_shutdown();
	
	audio_initialized = false;
}

void restartAudioBackend() {
	destroyAudioBackend();

	sns::Log::d(TAG, "Starting audio...");

	//
	// Audio
	//
	saudio_desc audio_des{};
	audio_des.sample_rate = sns::SampleRate;
	audio_des.num_channels = 1;
	audio_des.stream_cb = audio_callback;
	audio_des.buffer_frames = app.configuration().audio_buffer_size;
	saudio_setup(audio_des);

	assert(sns::SampleRate == saudio_sample_rate());
	assert(1 == saudio_channels());
	audio_initialized = true;
}

static void init(void) {
	app_font_initialized = false;
	audio_initialized = false;

	// setup sokol-gfx and sokol-time
	sg_desc desc = { };
	desc.context = sapp_sgcontext();
	sg_setup(&desc);

	// setup sokol-imgui, but provide our own font
	simgui_desc_t simgui_desc = { };
	simgui_desc.no_default_font = true;
	//simgui_desc.ini_filename = "settings_imgui.ini";
	simgui_setup(&simgui_desc);

	rebuildFonts();

	// initial clear color
	pass_action.colors[0].action = SG_ACTION_CLEAR;
	pass_action.colors[0].value = { 0.11f, 0.11f, 0.11f, 1.0f };


	//
	// app
	//
	app.initialize();

	restartAudioBackend();

	sns::platformRegisterCallback([](sns::PlatformEvent event){
		std::unique_lock<std::mutex> lock(platform_events_mutex);
		platform_events.push_back(event);
	});
}

static void dispatchPlatformEvent(sns::PlatformEvent const event) {
	if (event == sns::PlatformEvent::Wakeup) {
		sns::Log::d(TAG, "Power wakeup");
		restartAudioBackend();
	}
}

static void dispatchSokolEvent(sapp_event const* event) {
	simgui_handle_event(event);
	
	//if (event->type == sapp_event_type::SAPP_EVENTTYPE_MOUSE_MOVE)
	//	return;
	//sns::Log::d("sokol", sns::sfmt("%d", event->type));
}

static void frame(void) {
	const int width = sapp_width();
	const int height = sapp_height();

	if (width <= 1 || height <= 1) {
		return;
	}

	//dispatch platform events
	{
		std::unique_lock<std::mutex> lock(platform_events_mutex);
		for (auto const& current : platform_events)
			dispatchPlatformEvent(current);
		platform_events.clear();
	}


	simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

	app.render();

	// the sokol_gfx draw pass
	sg_begin_default_pass(&pass_action, width, height);
	simgui_render();
	sg_end_pass();
	sg_commit();


	if (app.configuration().video_fps > 0) {
		int64_t ts = sns::getCurrentMilliseconds();

		if (app_last_render_checkpoint > 0) {
			int64_t expected_duration = 1000 / sns::maximum(app.configuration().video_fps, 1);

			int64_t delta = ts - app_last_render_checkpoint;
			while (delta < expected_duration) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(15));
				std::this_thread::sleep_for(std::chrono::milliseconds(5));

				ts = sns::getCurrentMilliseconds();
				delta = ts - app_last_render_checkpoint;
			}
		}

		app_last_render_checkpoint = ts;
	}
}

static void cleanup(void) {
	sns::platformClearCallbacks();

	destroyAudioBackend();

	app.cleanup();

	destroyFonts();

	simgui_shutdown();
	sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	sns::Configuration configuration = sns::loadConfiguration(nullptr);

	sapp_icon_desc icon_desc = { };
	icon_desc.sokol_default = false;
	icon_desc.images[0] = { 256, 256, { app_icon_256_data, (sizeof(app_icon_256_data) - 1)} };

	//sns::saveCppBinary(app_icon_256_data, sizeof(app_icon_256_data), "C:\\Users\\ruiva\\Desktop\\dump.cpp");

	sapp_desc desc = { };
	desc.window_title = "Senos";
	desc.icon = icon_desc;

	desc.init_cb = init;
	desc.frame_cb = frame;
	desc.cleanup_cb = cleanup;
	desc.event_cb = dispatchSokolEvent;

	desc.width = configuration.window_width;
	desc.height = configuration.window_height;
	desc.fullscreen = configuration.window_fullscreen;
	desc.high_dpi = true;

	//desc.html5_ask_leave_site = false;
	//desc.ios_keyboard_resizes_canvas = false;
	//desc.gl_force_gles2 = true;
	//desc.enable_clipboard = false;


	return desc;
}
