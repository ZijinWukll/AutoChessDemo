#pragma once

#include <string>

namespace synera
{
    // 音频管理器：使用 Windows PlaySound API 播放程序化生成的 WAV 音频。
    // 无需 Qt Multimedia 模块或外部音频文件。
    class AudioManager
    {
    public:
        static void Init();
        static void Shutdown();

        // BGM 控制
        static void PlayBGM();
        static void StopBGM();

        // 音效
        static void PlayVictory();
        static void PlayDefeat();

    private:
        static void GenerateWavFiles();
        static std::string GetWavPath(const std::string& name);
        static void WriteSineWav(const std::string& path, int sampleRate, int durationSec,
                                 const float* freqs, int numFreqs,
                                 float volume, bool fadeInOut, bool loopable);
        static void WriteMelodyWav(const std::string& path, int sampleRate,
                                   const float* notes, int numNotes,
                                   float noteDuration, float volume);

        static bool s_initialized;
        static std::string s_bgmPath;
        static std::string s_victoryPath;
        static std::string s_defeatPath;
    };
}
