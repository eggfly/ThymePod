#include <Arduino.h>

#include "Audio.h" //https://github.com/schreibfaul1/ESP32-audioI2S
#include "config.h"

#include <vector>
#include <unordered_set>

// 定义音频库需要的回调函数
void audio_info(const char* info) {
    Serial.print("Audio Info: ");
    Serial.println(info);
}

void audio_id3data(const char* info) {
    Serial.print("ID3 Data: ");
    Serial.println(info);
}

void audio_eof_mp3(const char* info) {
    Serial.print("EOF MP3: ");
    Serial.println(info);
}

void audio_showstreamtitle(const char* info) {
    Serial.print("Stream Title: ");
    Serial.println(info);
}

void audio_showstation(const char* info) {
    Serial.print("Station: ");
    Serial.println(info);
}

void audio_bitrate(const char* info) {
    Serial.print("Bitrate: ");
    Serial.println(info);
}

void audio_commercial(const char* info) {
    Serial.print("Commercial: ");
    Serial.println(info);
}

void audio_icyurl(const char* info) {
    Serial.print("ICY URL: ");
    Serial.println(info);
}

void audio_icylogo(const char* info) {
    Serial.print("ICY Logo: ");
    Serial.println(info);
}

void audio_icydescription(const char* info) {
    Serial.print("ICY Description: ");
    Serial.println(info);
}

void audio_lasthost(const char* info) {
    Serial.print("Last Host: ");
    Serial.println(info);
}

void audio_eof_speech(const char* info) {
    Serial.print("EOF Speech: ");
    Serial.println(info);
}

void audio_eof_stream(const char* info) {
    Serial.print("EOF Stream: ");
    Serial.println(info);
}

void audio_id3image(File& file, const size_t pos, const size_t size) {
    Serial.printf("ID3 Image: pos=%d, size=%d\n", pos, size);
}

void audio_oggimage(File& file, std::vector<uint32_t> v) {
    Serial.print("OGG Image: ");
    for (auto& val : v) {
        Serial.printf("%d ", val);
    }
    Serial.println();
}

void audio_id3lyrics(File& file, const size_t pos, const size_t size) {
    Serial.printf("ID3 Lyrics: pos=%d, size=%d\n", pos, size);
}

void audio_process_i2s(int16_t* outBuff, int32_t validSamples, bool *continueI2S) {
    // 这个函数用于处理I2S音频数据，通常用于录音或蓝牙传输
    // 在这里我们不需要做任何处理，所以保持为空
    *continueI2S = true;
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
    audio.setVolume(0);
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
        Serial.println("autoPlay: playNextSong()");
        startNextSong(true);
    }
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
        Serial.println("re-shuffle.");
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
    Serial.print("songIndex=");
    Serial.print(m_activeSongIdx);
    Serial.print(", total=");
    Serial.println(m_songFiles.size());

    if (audio.isRunning())
    {
        stopSongWithMute();
        Serial.println("stop song");
        delay(2000);
        Serial.println("start next song");
    }
    // walkaround
    // setupButtonsNew();
    audio.connecttoFS(MY_SD, m_songFiles[m_activeSongIdx].c_str());

    Serial.println(m_songFiles[m_activeSongIdx].c_str());
}

void populateMusicFileList(String path, size_t depth)
{
    Serial.printf("search: %s, depth=%d\n", path.c_str(), LIST_DIR_RECURSION_DEPTH - depth);
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
                        Serial.print(entry.path());
                        Serial.print(" size=");
                        Serial.println(entry.size());
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
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return false;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return false;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    Serial.println("listDir end");
    return true;
}

void setup()
{
    // pinMode(I2S_WS, OUTPUT);
    // pinMode(4, OUTPUT);
    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect
    }
    // SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0, SD_MMC_D1, SD_MMC_D2, SD_MMC_D3);
    MY_SD.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    if (!SD_MMC.begin("/sdmmc", true, false))
    {
        Serial.println("Card Mount Failed");
        return;
    }
    auto populateStart = millis();

    Serial.printf("PSRAM剩余大小: %d字节\n", ESP.getFreePsram());
    populateMusicFileList("/", LIST_DIR_RECURSION_DEPTH);
    Serial.printf("PSRAM剩余大小: %d字节\n", ESP.getFreePsram());
    auto cost = millis() - populateStart;
    Serial.printf("populateMusicFileList cost %d ms, keep %d songs, total %d songs\n", cost, m_songFiles.size(), totalSongs);

    /* set the i2s pins */
    audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT, I2S_MCLK);
    audio.setVolume(volume);
    lvgl_amoled_init();
}


void loop()
{
    static auto t = millis();
    audio.loop();
    autoPlayNextSong();
    if (millis() - t > 1000)
    {
        t = millis();
        Serial.printf("loop, volume=%d\n", audio.getVolume());
    }
}
