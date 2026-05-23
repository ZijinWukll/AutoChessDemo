#include "AudioManager.h"

#include <cmath>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace synera
{
    bool AudioManager::s_initialized = false;
    std::string AudioManager::s_bgmPath;
    std::string AudioManager::s_victoryPath;
    std::string AudioManager::s_defeatPath;

    void AudioManager::Init()
    {
        if (s_initialized) return;
        GenerateWavFiles();
        s_initialized = true;
    }

    void AudioManager::Shutdown()
    {
        StopBGM();
        s_initialized = false;
    }

    void AudioManager::PlayBGM()
    {
        if (!s_initialized) Init();
        // 异步循环播放
        PlaySoundA(s_bgmPath.c_str(), nullptr, SND_ASYNC | SND_LOOP | SND_FILENAME);
    }

    void AudioManager::StopBGM()
    {
        PlaySoundA(nullptr, nullptr, 0);
    }

    void AudioManager::PlayVictory()
    {
        if (!s_initialized) Init();
        PlaySoundA(s_victoryPath.c_str(), nullptr, SND_ASYNC | SND_FILENAME);
    }

    void AudioManager::PlayDefeat()
    {
        if (!s_initialized) Init();
        PlaySoundA(s_defeatPath.c_str(), nullptr, SND_ASYNC | SND_FILENAME);
    }

    // ============================================================
    // WAV 文件生成
    // ============================================================

    void AudioManager::GenerateWavFiles()
    {
        s_bgmPath = GetWavPath("bgm");
        s_victoryPath = GetWavPath("victory");
        s_defeatPath = GetWavPath("defeat");

        // --- BGM: 环境氛围 ~15秒，柔和背景 ---
        // Am 和弦低音：A1(55) 子低音 + A2(110) C3(131) E3(165)
        constexpr int bgmSampleRate = 22050;
        constexpr int bgmDuration = 15;
        const float bgmFreqs[] = { 55.00f, 110.00f, 130.81f, 164.81f };
        WriteSineWav(s_bgmPath, bgmSampleRate, bgmDuration,
                      bgmFreqs, 4, 0.08f, true, true);

        // --- 胜利音效: 上行琶音 C4 E4 G4 C5 (1.5秒) ---
        constexpr int fxSampleRate = 22050;
        const float victoryNotes[] = {
            261.63f, 329.63f, 392.00f, 523.25f,
            523.25f, 659.25f, 783.99f, 1046.50f
        };
        WriteMelodyWav(s_victoryPath, fxSampleRate,
                       victoryNotes, 8, 0.18f, 0.3f);

        // --- 失败音效: 下行 A3 F3 D3 C3 (1.2秒) ---
        const float defeatNotes[] = {
            220.00f, 174.61f, 146.83f, 130.81f,
            110.00f, 87.31f, 73.42f, 65.41f
        };
        WriteMelodyWav(s_defeatPath, fxSampleRate,
                       defeatNotes, 8, 0.15f, 0.25f);
    }

    std::string AudioManager::GetWavPath(const std::string& name)
    {
        char tmpDir[MAX_PATH];
        GetTempPathA(MAX_PATH, tmpDir);
        return std::string(tmpDir) + "SyneraAudio_" + name + ".wav";
    }

    // 写入 WAV 文件（16-bit mono PCM）
    static void WriteWavFile(const std::string& path, const std::vector<int16_t>& samples, int sampleRate)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file) return;

        int dataSize = static_cast<int>(samples.size()) * 2; // 16-bit = 2 bytes per sample
        int fileSize = 36 + dataSize;

        auto write32 = [&](int v) { file.write(reinterpret_cast<const char*>(&v), 4); };
        auto write16 = [&](short v) { file.write(reinterpret_cast<const char*>(&v), 2); };

        // RIFF header
        file.write("RIFF", 4);
        write32(fileSize);
        file.write("WAVE", 4);

        // fmt chunk
        file.write("fmt ", 4);
        write32(16);            // chunk size
        write16(1);             // PCM format
        write16(1);             // mono
        write32(sampleRate);    // sample rate
        write32(sampleRate * 2); // byte rate
        write16(2);             // block align
        write16(16);            // bits per sample

        // data chunk
        file.write("data", 4);
        write32(dataSize);
        file.write(reinterpret_cast<const char*>(samples.data()), dataSize);
    }

    // 生成持续和声音频 — 柔和氛围风格
    void AudioManager::WriteSineWav(const std::string& path, int sampleRate, int durationSec,
                                    const float* freqs, int numFreqs,
                                    float volume, bool fadeInOut, bool loopable)
    {
        (void)loopable;
        int totalSamples = sampleRate * durationSec;

        // 延迟线（简单混响）
        int delaySamples = sampleRate / 2; // 0.5s 延迟
        std::vector<float> delayBuf(delaySamples, 0.0f);
        int delayPos = 0;

        std::vector<int16_t> samples(totalSamples);
        float prevSample = 0.0f;

        for (int i = 0; i < totalSamples; ++i)
        {
            float t = static_cast<float>(i) / sampleRate;
            float sample = 0.0f;

            // 三角形波混合（比正弦波温暖）
            for (int f = 0; f < numFreqs; ++f)
            {
                float amp = 1.0f / numFreqs;
                if (f == 0) amp *= 1.5f; // 低音加强

                float phase = fmodf(freqs[f] * t, 1.0f);
                // 三角形波: 0->0.5 上升, 0.5->1 下降
                float tri = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                sample += amp * tri;
            }

            // 低通滤波（简单单极）
            sample = prevSample + 0.15f * (sample - prevSample);
            prevSample = sample;

            // 柔和颤音
            float tremolo = 1.0f + 0.2f * sinf(2.0f * 3.14159f * 0.3f * t);
            sample *= tremolo;

            // 混响（反馈延迟）
            float wet = delayBuf[delayPos] * 0.35f;
            delayBuf[delayPos] = sample + wet * 0.4f;
            delayPos = (delayPos + 1) % delaySamples;
            sample += wet;

            // 淡入淡出
            if (fadeInOut)
            {
                float fadeLen = 0.15f;
                float fade = 1.0f;
                if (t < fadeLen * durationSec)
                    fade = t / (fadeLen * durationSec);
                else if (t > (1.0f - fadeLen) * durationSec)
                    fade = (durationSec - t) / (fadeLen * durationSec);
                sample *= fade;
            }

            sample = std::max(-1.0f, std::min(1.0f, sample));
            samples[i] = static_cast<int16_t>(sample * volume * 32767);
        }

        WriteWavFile(path, samples, sampleRate);
    }

    // 生成旋律音频（一系列音符）
    void AudioManager::WriteMelodyWav(const std::string& path, int sampleRate,
                                      const float* notes, int numNotes,
                                      float noteDuration, float volume)
    {
        int samplesPerNote = static_cast<int>(sampleRate * noteDuration);
        int totalSamples = samplesPerNote * numNotes;
        std::vector<int16_t> samples(totalSamples);

        for (int n = 0; n < numNotes; ++n)
        {
            float freq = notes[n];
            for (int i = 0; i < samplesPerNote; ++i)
            {
                int idx = n * samplesPerNote + i;
                float t = static_cast<float>(idx) / sampleRate;
                float noteT = static_cast<float>(i) / sampleRate;

                // 三角波 + 泛音（比纯正弦温暖）
                float phase = fmodf(freq * t, 1.0f);
                float tri = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                float sample = tri + 0.4f * sinf(2.0f * 3.14159f * freq * 2.0f * t)
                                    + 0.15f * sinf(2.0f * 3.14159f * freq * 3.0f * t);

                // 包络：快速起-缓慢落
                float envelope = 1.0f;
                if (noteT < 0.02f)
                    envelope = noteT / 0.02f;
                else if (noteT > noteDuration * 0.65f)
                    envelope = 1.0f - (noteT - noteDuration * 0.65f) / (noteDuration * 0.35f);

                sample *= envelope / 1.55f;

                sample = std::max(-1.0f, std::min(1.0f, sample));
                samples[idx] = static_cast<int16_t>(sample * volume * 32767);
            }
        }

        WriteWavFile(path, samples, sampleRate);
    }
}
