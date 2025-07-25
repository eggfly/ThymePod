#include <Arduino.h>

#include "Audio.h" //https://github.com/schreibfaul1/ESP32-audioI2S
#include "config.h"
#include <lvgl.h>
#include "ft2build.h"
#include <vector>
#include <unordered_set>

// #define USE_BINARY_FONT
#ifdef USE_BINARY_FONT
extern const uint8_t _binary_font_ttf_start[] asm("_binary_font_ttf_start");
extern const size_t _binary_font_ttf_length asm("font_ttf_length");
#endif

static const char *TAG = "main";


void autoPlayNextSong();

// 定义音频库需要的回调函数
void audio_info(const char *info)
{
    ESP_LOGI(TAG, "Audio Info: %s", info);
}

void audio_id3data(const char *info)
{
    ESP_LOGI(TAG, "ID3 Data: %s", info);
}

void audio_eof_mp3(const char *info)
{
    ESP_LOGI(TAG, "EOF MP3: %s", info);
    autoPlayNextSong();
}

void audio_showstreamtitle(const char *info)
{
    ESP_LOGI(TAG, "Stream Title: %s", info);
}

void audio_showstation(const char *info)
{
    ESP_LOGI(TAG, "Station: %s", info);
}

void audio_bitrate(const char *info)
{
    ESP_LOGI(TAG, "Bitrate: %s", info);
}

void audio_commercial(const char *info)
{
    ESP_LOGI(TAG, "Commercial: %s", info);
}

void audio_icyurl(const char *info)
{
    ESP_LOGI(TAG, "ICY URL: %s", info);
}

void audio_icylogo(const char *info)
{
    ESP_LOGI(TAG, "ICY Logo: %s", info);
}

void audio_icydescription(const char *info)
{
    ESP_LOGI(TAG, "ICY Description: %s", info);
}

void audio_lasthost(const char *info)
{
    ESP_LOGI(TAG, "Last Host: %s", info);
}

void audio_eof_speech(const char *info)
{
    ESP_LOGI(TAG, "EOF Speech: %s", info);
}

void audio_eof_stream(const char *info)
{
    ESP_LOGI(TAG, "EOF Stream: %s", info);
}

void audio_id3image(File &file, const size_t pos, const size_t size)
{
    ESP_LOGI(TAG, "ID3 Image: pos=%d, size=%d", pos, size);
}

void audio_oggimage(File &file, std::vector<uint32_t> v)
{
    ESP_LOGI(TAG, "OGG Image: ");
    for (auto &val : v)
    {
        ESP_LOGI(TAG, "%d ", val);
    }
    ESP_LOGI(TAG, "");
}

void audio_id3lyrics(File &file, const size_t pos, const size_t size)
{
    ESP_LOGI(TAG, "ID3 Lyrics: pos=%d, size=%d", pos, size);
}

bool diff_mode = false;
bool bass_mode = false;
bool treble_mode = false;

void audio_process_i2s(int16_t *outBuff, int32_t validSamples, bool *continueI2S)
{
    // 这个函数用于处理I2S音频数据，通常用于录音或蓝牙传输
    *continueI2S = true;
    if (!diff_mode)
    {
        return;
    }
    ESP_LOGI(TAG, "audio_process_i2s: validSamples=%d", validSamples);
    // 我需要把左右声道做减法，然后输出到I2S，左右声道是一样的结果
    int16_t diffValue = 0;
    for (int i = 0; i < validSamples * 2; i += 2)
    {
        // 左声道减去右声道，可以模拟实现去人声的效果
        diffValue = outBuff[i] - outBuff[i + 1];
        // int16_t - int16_t 结果一定是 int16_t 范围
        outBuff[i] = diffValue;
        outBuff[i + 1] = diffValue;
    }
}

#define MY_SD SD_MMC

#define LIST_DIR_RECURSION_DEPTH 5
const bool APP_DEBUG = true;

extern "C"
{
    void lvgl_amoled_init(void);
}

Audio audio;

int volume = 6; // 0...21?

bool shuffle_mode = true;

std::vector<String> m_songFiles{};
int m_activeSongIdx{-1};

// 0.5 代表随机去掉一半, 1.0 代表全都去掉, 越大去掉越多
float randomSkipRatio = 0.5;
size_t MaxSongsCount = 200; // max songs to keep in memory, if more than this, will not add to m_songFiles

size_t totalSongs = 0;
std::unordered_set<int> m_played_songs{};

void stopSongWithMute()
{
    // audio.setVolume(0);
    audio.stopSong();
}

void unmute()
{
    audio.setVolume(volume);
}

int strncmpci(const char *str1, const char *str2, size_t num)
{
    int ret_code = 0;
    size_t chars_compared = 0;

    if (!str1 || !str2)
    {
        ret_code = INT_MIN;
        return ret_code;
    }

    while ((chars_compared < num) && (*str1 || *str2))
    {
        ret_code = tolower((int)(*str1)) - tolower((int)(*str2));
        if (ret_code != 0)
        {
            break;
        }
        chars_compared++;
        str1++;
        str2++;
    }

    return ret_code;
}

bool startsWithIgnoreCase(const char *pre, const char *str)
{
    return strncmpci(pre, str, strlen(pre)) == 0;
}

bool endsWithIgnoreCase(const char *base, const char *str)
{
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strncmpci(base + blen - slen, str, strlen(str)));
}

void startNextSong(bool isNextOrPrev);

void autoPlayNextSong()
{
    if (m_songFiles.size() == 0)
    {
        delay(100);
        return;
    }
    if (!audio.isRunning())
    {
        ESP_LOGI(TAG, "autoPlay: playNextSong()");
        startNextSong(true);
    }
}

void setMuxHeadPhoneConnect(bool isConnected)
{
    digitalWrite(41, isConnected ? HIGH : LOW);
}

void startNextSong(bool isNextOrPrev)
{
    if (m_songFiles.size() == 0)
    {
        return;
    }
    m_played_songs.insert(m_activeSongIdx);
    if (m_played_songs.size() * 2 > m_songFiles.size())
    {
        ESP_LOGI(TAG, "re-shuffle.");
        m_played_songs.clear();
    }
    if (isNextOrPrev)
    {
        m_activeSongIdx++;
    }
    else
    {
        m_activeSongIdx--;
    }
    if (shuffle_mode)
    {
        do
        {
            m_activeSongIdx = random(m_songFiles.size());
        } while (m_played_songs.find(m_activeSongIdx) != std::end(m_played_songs));
    }
    //  if (m_activeSongIdx >= m_songFiles.size() || m_activeSongIdx < 0) {
    //    m_activeSongIdx = 0;
    //  }
    m_activeSongIdx %= m_songFiles.size();
    ESP_LOGI(TAG, "songIndex=%d, total=%d", m_activeSongIdx, m_songFiles.size());

    if (audio.isRunning())
    {
        stopSongWithMute();
        ESP_LOGI(TAG, "stop song");
        delay(2000);
        ESP_LOGI(TAG, "start next song");
    }
    // walkaround
    // setupButtonsNew();
    audio.connecttoFS(MY_SD, m_songFiles[m_activeSongIdx].c_str());
    setMuxHeadPhoneConnect(true);

    ESP_LOGI(TAG, "%s", m_songFiles[m_activeSongIdx].c_str());
}

void populateMusicFileList(String path, size_t depth)
{
    ESP_LOGI(TAG, "search: %s, depth=%d", path.c_str(), LIST_DIR_RECURSION_DEPTH - depth);
    // if contains 2019乐队的夏天, then ignore and return
    if (strstr(path.c_str(), "2019乐队的夏天")) {
        ESP_LOGI(TAG, "Ignore 2019乐队的夏天!");
        return;
    }
    File musicDir = MY_SD.open(path);
    bool nextFileFound;
    do
    {
        nextFileFound = false;
        File entry = musicDir.openNextFile();
        if (entry)
        {
            nextFileFound = true;
            if (!entry.name() || entry.name()[0] == '.')
            {
                continue;
            }
            if (entry.isDirectory())
            {
                if (depth)
                {
                    populateMusicFileList(entry.path(), depth - 1);
                }
            }
            else
            {
                const bool entryIsFile = entry.size() > 4096;
                if (entryIsFile)
                {
                    if (APP_DEBUG)
                    {
                        ESP_LOGI(TAG, "%s size=%d", entry.path(), entry.size());
                    }
                    if (endsWithIgnoreCase(entry.name(), ".mp3") || endsWithIgnoreCase(entry.name(), ".flac") || endsWithIgnoreCase(entry.name(), ".aac") || endsWithIgnoreCase(entry.name(), ".wav"))
                    {
                        totalSongs++;
                        long r = random(10000);
                        bool keep = r > 10000 * randomSkipRatio;
                        if (keep)
                        {
                            m_songFiles.push_back(entry.path());
                        }
                    }
                }
            }
            entry.close();
        }
    } while (nextFileFound && m_songFiles.size() < MaxSongsCount);
}

bool listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    ESP_LOGI(TAG, "Listing directory: %s", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        ESP_LOGI(TAG, "Failed to open directory");
        return false;
    }
    if (!root.isDirectory())
    {
        ESP_LOGI(TAG, "Not a directory");
        return false;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            ESP_LOGI(TAG, "  DIR : %s", file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            ESP_LOGI(TAG, "  FILE: %s, SIZE: %d", file.name(), file.size());
        }
        file = root.openNextFile();
    }
    ESP_LOGI(TAG, "listDir end");
    return true;
}

#define FONT_PATH "/sdmmc/lv_fonts/Lato-Regular.ttf"

#define BIG_FONT_PATH "/sdmmc/GenSenRounded.ttf"

// 新增字体初始化函数
lv_font_t* init_freetype_font(void)
{
    // 确保 FreeType 库已初始化
    static bool freetype_initialized = false;
    if (!freetype_initialized) {
        if (!lv_freetype_init(0, 0, 0)) {
            ESP_LOGI(TAG, "FreeType 库初始化失败");
            return NULL;
        }
        freetype_initialized = true;
        ESP_LOGI(TAG, "FreeType 库初始化成功");
    }
    
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = BIG_FONT_PATH;
    // info.name = "P:/GenSenRounded.ttf"; // 这样会失败
    info.weight = 32;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
        ESP_LOGI(TAG, "create failed.");
        return NULL;
    } else {
        ESP_LOGI(TAG, "create success.");
        return info.font;
    }
}

#ifdef USE_BINARY_FONT
lv_font_t* init_freetype_font_from_binary(void)
{
    // 确保 FreeType 库已初始化
    static bool freetype_initialized = false;
    if (!freetype_initialized) {
        if (!lv_freetype_init(0, 0, 0)) {
            ESP_LOGI(TAG, "FreeType 库初始化失败");
            return NULL;
        }
        freetype_initialized = true;
        ESP_LOGI(TAG, "FreeType 库初始化成功");
    }
    
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = nullptr;
    // info.name = "P:/GenSenRounded.ttf"; // 这样会失败
    info.weight = 32;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = _binary_font_ttf_start;
    info.mem_size = _binary_font_ttf_length;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
        ESP_LOGI(TAG, "create failed.");
        return NULL;
    } else {
        ESP_LOGI(TAG, "create success.");
        return info.font;
    }
}
#endif

void lv_example_tiny_ttf_2(void)
{
    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_font_t * font = lv_tiny_ttf_create_file("P:/GenSenRounded.ttf", 30);
    if (font == NULL) {
        ESP_LOGE(TAG, "lv_tiny_ttf_create_file return font is NULL");
        return;
    }
    ESP_LOGI(TAG, "font load ok pointer=%p", font);
    ESP_LOGI(TAG, "internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
    lv_style_set_text_color(&style, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style, font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a label with the new style*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text(label, "Hello world\n我是TF卡里面的裸奔TTF字体，使用LVGL渲染，不需要取模，矢量字体，可以任意设置大小\ncreated\nwith Tiny TTF");
    lv_obj_center(label);
}

void lv_example_freetype_1(lv_font_t* font)
{
    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style, font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a label with the new style*/
    lv_obj_t * screen = lv_scr_act();
    if (screen == NULL) {
        ESP_LOGI(TAG, "screen is NULL");
        return;
    }
    lv_obj_t * label = lv_label_create(screen);
    ESP_LOGI(TAG, "label=%p", label);
    lv_obj_add_style(label, &style, 0);
    ESP_LOGI(TAG, "label=%p, style=%p", label, &style);
    lv_label_set_text(label, "Hello\n我是TF卡里面的裸奔\nTTF字体，使用LVGL渲染\n，不需要取模，矢量字体\n，可以任意设置大小\ncreated\nwith FreeType library");
    lv_obj_center(label);
}

void setup()
{
    lvgl_amoled_init();
    ESP_LOGI(TAG, "setup(): internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
    // ESP_LOGI(TAG, "font.ttf start=%p, size=%d", _binary_font_ttf_start, _binary_font_ttf_length);
    pinMode(0, INPUT_PULLUP);
    pinMode(41, OUTPUT);
    setMuxHeadPhoneConnect(false);
    Serial.begin(115200);
    // 可以Serial.begin但是不要做下面等待Serial的代码，因为Arduino不开串口的时候，板子起不来
    // while (!Serial)
    // {
    //     ; // wait for serial port to connect
    // }
    bool tfCardMounted = false;
    // SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0, SD_MMC_D1, SD_MMC_D2, SD_MMC_D3);
    MY_SD.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    if (!SD_MMC.begin("/sdmmc", true, false))
    {
        ESP_LOGE(TAG, "Card Mount Failed");
        tfCardMounted = false;
    } else {
        ESP_LOGI(TAG, "Card Mount OK!");
        tfCardMounted = true;
    }
    // ESP_LOGI(TAG, "Before malloc: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
    // void * mem = malloc(1024 * 1024);
    // ESP_LOGI(TAG, "malloc: mem=%p", mem);
    // ESP_LOGI(TAG, "After malloc: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
    // free(mem);
    // ESP_LOGI(TAG, "After free: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());

    if (tfCardMounted) {
        FILE *f = fopen(FONT_PATH, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            ESP_LOGI(TAG, "字体文件大小: %ld 字节", size);
            fclose(f);
            // 通常TTF文件 > 10KB
            if (size < 10240) {
                ESP_LOGE(TAG, "警告: 字体文件可能损坏");
            }
        }

        auto populateStart = millis();

        ESP_LOGI(TAG, "Before populateMusicFileList: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
        populateMusicFileList("/", LIST_DIR_RECURSION_DEPTH);
        ESP_LOGI(TAG, "After populateMusicFileList: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());

        auto cost = millis() - populateStart;
        ESP_LOGI(TAG, "populateMusicFileList cost %d ms, keep %d songs, total %d songs", cost, m_songFiles.size(), totalSongs);
    }
    /* set the i2s pins */
    audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT, I2S_MCLK);
    audio.setVolume(volume);
    autoPlayNextSong();
    ESP_LOGI(TAG, "After lvgl_amoled_init: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());

    // 初始化 FreeType 字体
    // lv_font_t* custom_font = init_freetype_font_from_binary();
    // if (custom_font) {
    //     ESP_LOGI(TAG, "After init_freetype_font: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
    // } else {
    //     ESP_LOGE(TAG, "FreeType 字体初始化失败");
    // }
    // ESP_LOGI(TAG, "After init_freetype_font: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());

    // 例子在 managed_components/lvgl__lvgl/examples/libs/freetype/lv_example_freetype_1.c
    // lv_example_freetype_1(custom_font);
    // ESP_LOGI(TAG, "After lv_example_freetype_1: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());

    // lv_example_tiny_ttf_2();
    // ESP_LOGI(TAG, "After lv_example_tiny_ttf_2: internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
}

void handleButton()
{
    if (digitalRead(0) == LOW)
    {
        while (digitalRead(0) == LOW)
        {
            delay(1);
        }
        ESP_LOGI(TAG, "play next song");
        startNextSong(true);
    }
}
void parseSerialCommand()
{
    if (Serial.available())
    {
        Serial.setTimeout(5);
        String r = Serial.readString();
        r.trim();
        if (r.equalsIgnoreCase("n"))
        {
            ESP_LOGI(TAG, "play next song");
            startNextSong(true);
        }
        else if (r.equalsIgnoreCase("r"))
        {
            // toggle random shuffle mode
            shuffle_mode = !shuffle_mode;
            ESP_LOGI(TAG, "shuffle mode: %s", shuffle_mode ? "on" : "off");
        }
        else if (r.equalsIgnoreCase("f"))
        {
            // play current song again
            audio.setAudioPlayPosition(0);
            ESP_LOGI(TAG, "play current song again");
        }
        else if (r.equalsIgnoreCase("<"))
        {
            // play current song again
            int32_t currentTime = audio.getAudioCurrentTime();
            ESP_LOGI(TAG, "current time: %d", currentTime);
            int32_t newTime = currentTime - 10;
            if (newTime < 0)
            {
                newTime = 0;
            }
            audio.setAudioPlayPosition(newTime);
            ESP_LOGI(TAG, "seek current song to %d", newTime);
        }
        else if (r.equalsIgnoreCase(">"))
        {
            // play current song again
            int32_t currentTime = audio.getAudioCurrentTime();
            ESP_LOGI(TAG, "current time: %d", currentTime);
            int32_t newTime = currentTime + 10;
            if (newTime > audio.getAudioFileDuration())
            {
                newTime = audio.getAudioFileDuration();
            }
            audio.setAudioPlayPosition(newTime);
            ESP_LOGI(TAG, "seek current song to %d", newTime);
        }
        else if (r.equalsIgnoreCase("d"))
        {
            // toggle diff mode
            diff_mode = !diff_mode;
            ESP_LOGI(TAG, "diff mode: %s", diff_mode ? "on" : "off");
        }
        else if (r.equalsIgnoreCase("b"))
        {
            // toggle bass mode
            bass_mode = !bass_mode;
            ESP_LOGI(TAG, "bass mode: %s", bass_mode ? "on" : "off");
            audio.setTone(bass_mode ? 6: 0, -1, -4);
        }
        else if (r.equalsIgnoreCase("t"))
        {
            // toggle treble mode
            treble_mode = !treble_mode;
            ESP_LOGI(TAG, "treble mode: %s", treble_mode ? "on" : "off");
            audio.setTone(-4, -1, treble_mode ? 6: 0);
        }
        else if (r.equalsIgnoreCase("s"))
        {
            stopSongWithMute();
            ESP_LOGI(TAG, "stop song");
        }
        else if (r.equalsIgnoreCase("p"))
        {
            audio.pauseResume();
            ESP_LOGI(TAG, "pause/resume song");
        }
        else if (r.equalsIgnoreCase("+") || r.equalsIgnoreCase("="))
        {
            volume += 1;
            if (volume > 21)
            {
                volume = 21;
            }
            if (volume > 0)
            {
                unmute();
            }
            audio.setVolume(volume);
            ESP_LOGI(TAG, "volume up: %d", volume);
        }
        else if (r.equalsIgnoreCase("-"))
        {
            volume -= 1;
            if (volume < 0)
            {
                volume = 0;
            }
            if (volume == 0)
            {
            }
            audio.setVolume(volume);
            ESP_LOGI(TAG, "volume down: %d", volume);
        }
        else if (r.equalsIgnoreCase("info"))
        {
            ESP_LOGI(TAG, "Audio info:");
            ESP_LOGI(TAG, "  codec: %s", audio.getCodecname());
            ESP_LOGI(TAG, "  sample rate: %d", audio.getSampleRate());
            ESP_LOGI(TAG, "  bits per sample: %d", audio.getBitsPerSample());
            ESP_LOGI(TAG, "  channels: %d", audio.getChannels());
            ESP_LOGI(TAG, "  bitrate: %d", audio.getBitRate());
            ESP_LOGI(TAG, "  file size: %d", audio.getFileSize());
            ESP_LOGI(TAG, "  file pos: %d", audio.getFilePos());
            ESP_LOGI(TAG, "  file duration: %d sec", audio.getAudioFileDuration());
        }
        else if (r.equalsIgnoreCase("free"))
        {
            ESP_LOGI(TAG, "free heap=%i, free psram=%i", ESP.getFreeHeap(), ESP.getFreePsram());
        }
        else if (r.equalsIgnoreCase("list"))
        {
            ESP_LOGI(TAG, "list songs:");
            for (int i = 0; i < m_songFiles.size(); i++)
            {
                ESP_LOGI(TAG, "%d: %s", i, m_songFiles[i].c_str());
            }
        }
        else if (r.length() > 5)
        {
            // put streamURL in serial monitor
            stopSongWithMute();
            audio.connecttoFS(MY_SD, r.c_str());
        }
        // log_i("free heap=%i", ESP.getFreeHeap());
    }
}

void loop()
{
    static auto t = millis();
    delay(1);
    audio.loop();
    handleButton();
    // autoPlayNextSong();
    parseSerialCommand();
    if (millis() - t > 100)
    {
        t = millis();
        ESP_LOGI(TAG, "internal free heap=%d, psram=%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
        ESP_LOGI(TAG, "loop, volume=%d, duration=%d, uv_level=%d", audio.getVolume(), audio.getAudioCurrentTime(), audio.getVUlevel());
    }
}
