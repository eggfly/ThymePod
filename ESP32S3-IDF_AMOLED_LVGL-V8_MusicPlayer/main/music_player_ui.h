#ifndef MUSIC_PLAYER_UI_H
#define MUSIC_PLAYER_UI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建音乐播放器界面
 * 
 * 创建一个完整的音乐播放器界面，包括：
 * - 专辑封面显示区域
 * - 歌曲标题和艺术家信息
 * - 播放进度条
 * - 播放控制按钮（上一首、播放/暂停、下一首）
 * - 音量控制滑块
 */
void music_player_create_ui(void);

/**
 * @brief 获取音乐播放器屏幕对象
 * 
 * @return lv_obj_t* 音乐播放器屏幕对象
 */
lv_obj_t *music_player_get_screen(void);

/**
 * @brief 更新播放进度
 * 
 * @param current 当前播放时间（秒）
 * @param total 总时长（秒）
 */
void music_player_update_progress(uint32_t current, uint32_t total);

/**
 * @brief 更新歌曲信息
 * 
 * @param title 歌曲标题
 * @param artist 艺术家名称
 */
void music_player_update_song_info(const char *title, const char *artist);

/**
 * @brief 设置播放状态
 * 
 * @param playing true表示播放，false表示暂停
 */
void music_player_set_playing_state(bool playing);

/**
 * @brief 获取播放状态
 * 
 * @return true 正在播放
 * @return false 已暂停
 */
bool music_player_get_playing_state(void);

#ifdef __cplusplus
}
#endif

#endif // MUSIC_PLAYER_UI_H 