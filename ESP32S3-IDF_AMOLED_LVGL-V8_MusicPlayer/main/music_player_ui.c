#include "music_player_ui.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "music_player_ui";

// 音乐播放器UI组件
static lv_obj_t *music_screen = NULL;
static lv_obj_t *album_cover = NULL;
static lv_obj_t *song_title_label = NULL;
static lv_obj_t *artist_label = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *duration_label = NULL;
static lv_obj_t *play_btn = NULL;
static lv_obj_t *prev_btn = NULL;
static lv_obj_t *next_btn = NULL;
static lv_obj_t *volume_slider = NULL;

// 播放状态
static bool is_playing = false;

extern lv_font_t* custom_font_ttf;

// 播放控制回调函数
static void play_button_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (is_playing) {
        // 暂停播放
        lv_label_set_text(label, LV_SYMBOL_PLAY);
        is_playing = false;
        ESP_LOGI(TAG, "音乐暂停");
    } else {
        // 开始播放
        lv_label_set_text(label, LV_SYMBOL_PAUSE);
        is_playing = true;
        ESP_LOGI(TAG, "音乐播放");
    }
}

static void prev_button_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "上一首");
    // 这里可以添加切换上一首歌曲的逻辑
}

static void next_button_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "下一首");
    // 这里可以添加切换下一首歌曲的逻辑
}

static void volume_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    ESP_LOGI(TAG, "音量: %ld", value);
    // 这里可以添加音量控制逻辑
}

// 更新播放进度
void music_player_update_progress(uint32_t current, uint32_t total)
{
    if (progress_bar) {
        uint32_t progress = (current * 100) / total;
        lv_slider_set_value(progress_bar, progress, LV_ANIM_ON);
        
        // 更新时间标签
        uint32_t current_min = current / 60;
        uint32_t current_sec = current % 60;
        uint32_t total_min = total / 60;
        uint32_t total_sec = total % 60;
        
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02lu:%02lu", current_min, current_sec);
        lv_label_set_text(time_label, time_str);
        
        char duration_str[16];
        snprintf(duration_str, sizeof(duration_str), "%02lu:%02lu", total_min, total_sec);
        lv_label_set_text(duration_label, duration_str);
    }
}

// 更新歌曲信息
void music_player_update_song_info(const char *title, const char *artist)
{
    if (song_title_label) {
        lv_label_set_text(song_title_label, title);
    }
    if (artist_label) {
        lv_label_set_text(artist_label, artist);
    }
}

// 创建音乐播放器界面
void music_player_create_ui(void)
{
    // 创建主屏幕
    music_screen = lv_obj_create(NULL);
    
    // 设置屏幕背景色为深色主题
    lv_obj_set_style_bg_color(music_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(music_screen, LV_OPA_COVER, 0);
    
    // 创建专辑封面容器
    lv_obj_t *cover_container = lv_obj_create(music_screen);
    lv_obj_set_size(cover_container, 200, 200);
    lv_obj_align(cover_container, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_radius(cover_container, 20, 0);
    lv_obj_set_style_bg_color(cover_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(cover_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cover_container, 0, 0);
    lv_obj_set_style_shadow_width(cover_container, 20, 0);
    lv_obj_set_style_shadow_color(cover_container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(cover_container, LV_OPA_30, 0);
    
    // 创建专辑封面图片（使用音乐图标作为占位符）
    album_cover = lv_label_create(cover_container);
    lv_obj_set_size(album_cover, 160, 160);
    lv_obj_center(album_cover);
    lv_obj_set_style_text_font(album_cover, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(album_cover, lv_color_hex(0x888888), 0);
    if (custom_font_ttf) {
        lv_obj_set_style_text_font(album_cover, custom_font_ttf, 0);
    }
    lv_label_set_text(album_cover, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_align(album_cover, LV_TEXT_ALIGN_CENTER, 0);
    
    // 创建歌曲标题
    song_title_label = lv_label_create(music_screen);
    lv_obj_set_size(song_title_label, 400, 30);
    lv_obj_align_to(song_title_label, cover_container, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    if (custom_font_ttf) {
        lv_obj_set_style_text_font(song_title_label, custom_font_ttf, 0);
    }
    // lv_obj_set_style_text_font(song_title_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(song_title_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(song_title_label, "未知歌曲");
    lv_obj_set_style_text_align(song_title_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // 创建艺术家名称
    artist_label = lv_label_create(music_screen);
    lv_obj_set_size(artist_label, 400, 25);
    lv_obj_align_to(artist_label, song_title_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    // lv_obj_set_style_text_font(artist_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x888888), 0);
    if (custom_font_ttf) {
        lv_obj_set_style_text_font(artist_label, custom_font_ttf, 0);
    }
    lv_label_set_text(artist_label, "未知艺术家");
    lv_obj_set_style_text_align(artist_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // 创建进度条容器
    lv_obj_t *progress_container = lv_obj_create(music_screen);
    lv_obj_set_size(progress_container, 400, 60);
    lv_obj_align_to(progress_container, artist_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_opa(progress_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(progress_container, 0, 0);
    
    // 创建进度条
    progress_bar = lv_slider_create(progress_container);
    lv_obj_set_size(progress_bar, 380, 10);
    lv_obj_align(progress_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x1DB954), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar, 5, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(progress_bar, 8, LV_PART_KNOB);
    lv_obj_set_style_pad_all(progress_bar, 0, LV_PART_KNOB);
    lv_slider_set_range(progress_bar, 0, 100);
    lv_slider_set_value(progress_bar, 0, LV_ANIM_OFF);
    
    // 创建时间标签
    time_label = lv_label_create(progress_container);
    lv_obj_set_size(time_label, 60, 20);
    lv_obj_align_to(time_label, progress_bar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_text_font(time_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x888888), 0);
    lv_label_set_text(time_label, "00:00");
    
    // 创建总时长标签
    duration_label = lv_label_create(progress_container);
    lv_obj_set_size(duration_label, 60, 20);
    lv_obj_align_to(duration_label, progress_bar, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);
    lv_obj_set_style_text_font(duration_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(duration_label, lv_color_hex(0x888888), 0);
    lv_label_set_text(duration_label, "03:00");
    
    // 创建控制按钮容器
    lv_obj_t *control_container = lv_obj_create(music_screen);
    lv_obj_set_size(control_container, 400, 80);
    lv_obj_align_to(control_container, progress_container, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_opa(control_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(control_container, 0, 0);
    
    // 创建上一首按钮
    prev_btn = lv_btn_create(control_container);
    lv_obj_set_size(prev_btn, 60, 60);
    lv_obj_align(prev_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(prev_btn, 30, 0);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(prev_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(prev_btn, 0, 0);
    lv_obj_set_style_shadow_width(prev_btn, 10, 0);
    lv_obj_set_style_shadow_color(prev_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(prev_btn, LV_OPA_20, 0);
    
    lv_obj_t *prev_label = lv_label_create(prev_btn);
    lv_obj_set_style_text_font(prev_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(prev_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(prev_label, LV_SYMBOL_PREV);
    lv_obj_center(prev_label);
    lv_obj_add_event_cb(prev_btn, prev_button_cb, LV_EVENT_CLICKED, NULL);
    
    // 创建播放/暂停按钮
    play_btn = lv_btn_create(control_container);
    lv_obj_set_size(play_btn, 80, 80);
    lv_obj_align(play_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(play_btn, 40, 0);
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x1DB954), 0);
    lv_obj_set_style_bg_opa(play_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(play_btn, 0, 0);
    lv_obj_set_style_shadow_width(play_btn, 15, 0);
    lv_obj_set_style_shadow_color(play_btn, lv_color_hex(0x1DB954), 0);
    lv_obj_set_style_shadow_opa(play_btn, LV_OPA_30, 0);
    
    lv_obj_t *play_label = lv_label_create(play_btn);
    lv_obj_set_style_text_font(play_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(play_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(play_label, LV_SYMBOL_PLAY);
    lv_obj_center(play_label);
    lv_obj_set_style_text_align(play_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_event_cb(play_btn, play_button_cb, LV_EVENT_CLICKED, NULL);
    
    // 创建下一首按钮
    next_btn = lv_btn_create(control_container);
    lv_obj_set_size(next_btn, 60, 60);
    lv_obj_align(next_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(next_btn, 30, 0);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(next_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(next_btn, 0, 0);
    lv_obj_set_style_shadow_width(next_btn, 10, 0);
    lv_obj_set_style_shadow_color(next_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(next_btn, LV_OPA_20, 0);
    
    lv_obj_t *next_label = lv_label_create(next_btn);
    lv_obj_set_style_text_font(next_label, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(next_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(next_label, LV_SYMBOL_NEXT);
    lv_obj_center(next_label);
    lv_obj_add_event_cb(next_btn, next_button_cb, LV_EVENT_CLICKED, NULL);
    
    // 创建音量控制容器
    lv_obj_t *volume_container = lv_obj_create(music_screen);
    lv_obj_set_size(volume_container, 400, 60);
    lv_obj_align_to(volume_container, control_container, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_opa(volume_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(volume_container, 0, 0);
    
    // 创建音量图标
    lv_obj_t *volume_icon = lv_label_create(volume_container);
    lv_obj_set_size(volume_icon, 30, 30);
    lv_obj_align(volume_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(volume_icon, &lv_font_simsun_16_cjk, 0);
    lv_obj_set_style_text_color(volume_icon, lv_color_hex(0x888888), 0);
    lv_label_set_text(volume_icon, LV_SYMBOL_VOLUME_MID);
    
    // 创建音量滑块
    volume_slider = lv_slider_create(volume_container);
    lv_obj_set_size(volume_slider, 350, 10);
    lv_obj_align_to(volume_slider, volume_icon, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(volume_slider, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(volume_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(volume_slider, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_slider, lv_color_hex(0x1DB954), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(volume_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(volume_slider, 5, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volume_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(volume_slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(volume_slider, 6, LV_PART_KNOB);
    lv_obj_set_style_pad_all(volume_slider, 0, LV_PART_KNOB);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // 加载屏幕
    lv_scr_load(music_screen);
    
    ESP_LOGI(TAG, "音乐播放器界面创建完成");
}

// 获取音乐播放器屏幕对象
lv_obj_t *music_player_get_screen(void)
{
    return music_screen;
}

// 设置播放状态
void music_player_set_playing_state(bool playing)
{
    is_playing = playing;
    if (play_btn) {
        lv_obj_t *label = lv_obj_get_child(play_btn, 0);
        if (playing) {
            lv_label_set_text(label, LV_SYMBOL_PAUSE);
        } else {
            lv_label_set_text(label, LV_SYMBOL_PLAY);
        }
    }
}

// 获取播放状态
bool music_player_get_playing_state(void)
{
    return is_playing;
} 