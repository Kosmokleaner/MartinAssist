#pragma once

#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wmultichar"
#endif

// minimal granular sound synth without UI and backend specifics

#include <string>
#include <vector>
#include <cmath> // floor()
#include <memory>   // std::shared_ptr<>
#include <assert.h>
#include "types.h"
#include <mutex>

// high quality even with large values
//typedef double flt;
// faster, todo: make sure we stay near 0
typedef float flt;

// ===========================================================================

// pass with std::unique_ptr<> for thread safety
struct AudioFile
{
    // mostly for debugging
    std::string fileName;
    // .wav file
    std::vector<uint8> fileData;

    const static unsigned int wavHeaderSize = 44;

    bool isValid() const
    {
        // todo
        return false;
    }

    bool LoadWav(const char* pathname)
    {
        fileName = pathname;
        size_t IO_GetFileSize(const char* Name);

        size_t size = IO_GetFileSize(pathname);

        fileData.clear();

        if (!size)
            return false;

        if (size < wavHeaderSize)       // minimal wav header
            return false;

        fileData.resize(size);

        FILE* file = 0;

        errno_t err = fopen_s(&file, pathname, "rb");
        //        if ((file = _wfopen(pathname.c_str(), L"rb")) != NULL)
        if (err == 0)
        {
            fread(fileData.data(), size, 1, file);
            fclose(file);

            // https://docs.fileformat.com/audio/wav
            // https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

            uint32 riff = *(uint32*)&fileData[0];
            uint32 wave = *(uint32*)&fileData[8];
            uint32 fmt = *(uint32*)&fileData[12];
            uint32 chunkSize = *(uint32*)&fileData[16];
            uint16 format = *(uint16*)&fileData[20];
            uint16 channels = getChannelCount();
            uint16 bitsPerSample = *(uint16*)&fileData[34];
            if (riff != 'FFIR' ||   // 'RIFF'
                wave != 'EVAW' ||   // 'WAVE'
                fmt != ' tmf' ||    // 'fmt '
                chunkSize != 16 ||  // 16 / 18 / 28 / 40
                format != 1 ||      // PCM
                (channels != 1 && channels != 2) ||      // mono / stereo
                bitsPerSample != 16) // 16 bit
            {
                fileData.clear();
                return false;
            }

            return true;
        }

        fileData.clear();
        return false;
    }

    const uint32 getSampleDataSize() const
    {
        // /2 for DEVICE_FORMAT==ma_format_s16
    //    return *(unsigned int*)&g_sample[40] / 2;
        return *(uint32*)&fileData[40] / 2;
    }
    const unsigned int getSampleRate() const
    {
        return *(unsigned int*)&fileData[24];
    }
    const uint16 getChannelCount() const
    {
        return *(uint16*)&fileData[22];
    }
    // @param pos = 0..getSampleDataSize()
    short getSample(unsigned int pos) const
    {
        const short* data = (short*)&fileData[wavHeaderSize];
        const uint16 channelCount = getChannelCount();

        const uint32 sampleDataSize = getSampleDataSize();

        if (channelCount == 2)
            pos *= 2;              // only use one channel

        return data[pos % sampleDataSize];
    }
};

// @param alpha 0..1
inline float lerp(float s0, float s1, float alpha)
{
    return s0 * (1.0f - alpha) + s1 * alpha;
}

inline float saturate(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

inline double _floor(double x)
{
    return floor(x);
}
inline float _floor(float x)
{
    return floorf(x);
}

inline double frac(double x)
{
    return x - _floor(x);
}
inline float frac(float x)
{
    return x - _floor(x);
}

// also works with negative value
inline uint32 wrapWithin(const flt value, const uint32 display_count)
{
    return (uint32)(frac(value / display_count) * display_count);
}



struct Wave
{
    struct Envelope
    {
        float holdSec;
        float attackSec;
    };

    // in seconds
    flt time = 0;
    // in sec seconds per dst second
    float advancePerSec = 1.0f;

    // in seconds [mode0, mode1] = { time, time }
    flt pos[2] = { 0, 0 };

    // volume shape

    // in seconds
    float grainSec = 0.1f;
    // 0:hard..100:max
    float blendPercent = 10.0f;
    // 0:mute .. 100:max
    float volumePercent = 40.0f;
    // in sec seconds per dst second
    float pitch = 1.0f;
    // in seconds
    float phasePos = 0;

private:
    // protect audioFile
    std::mutex mutexAudioFile;
    // protected by mutexAudioFile 
    std::shared_ptr<AudioFile> audioFile;
public:
    void setAudioFile(std::shared_ptr<AudioFile> inAudioFile)
    {
        std::lock_guard<std::mutex> lock(mutexAudioFile);
        audioFile = inAudioFile;
    }
    std::shared_ptr<AudioFile> getAudioFile()
    {
        std::lock_guard<std::mutex> lock(mutexAudioFile);
        return audioFile;
    }

    float computeAlpha(const Envelope envelope) const
    {
        float alpha = 0.0f;

        if (phasePos < envelope.attackSec)
        {
            alpha = phasePos / (float)envelope.attackSec;
            //            assert(alpha >= 0.0f && alpha <= 1.0f);
            alpha = saturate(alpha);
        }
        else if (phasePos < envelope.attackSec + envelope.holdSec)
        {
            return 1.0f;
        }
        else if (phasePos < envelope.attackSec * 2 + envelope.holdSec)
        {
            alpha = 1.0f - (phasePos - envelope.attackSec - envelope.holdSec) / (float)envelope.attackSec;
            //            assert(alpha >= 0.0f && alpha <= 1.0f);
            alpha = saturate(alpha);
        }
        return alpha;
    }

    Envelope computeEnvelope() const
    {
        Envelope ret;

        const float blendFrac = blendPercent / 100.0f;
        ret.holdSec = grainSec * (1.0f - blendFrac);
        ret.attackSec = (grainSec - ret.holdSec);

        float test1 = fabsf(grainSec * 2 - (ret.holdSec * 2 + ret.attackSec * 2));
        assert(test1 < 0.01f);

        float test2 = fabsf(grainSec - (ret.holdSec + ret.attackSec));
        assert(test2 < 0.01f);

        return ret;
    }

    // side effect: update phase and phasePos
    // @param deltaTime in seconds 
    // @return value without volume applied
    float update(float deltaTime)
    {
        const unsigned int sampleRate = audioFile->getSampleRate();

        time += advancePerSec * deltaTime;

        // for better precision with float
        // wrap time in 
        time = frac(time / sampleRate) * sampleRate;

        phasePos += pitch * deltaTime;

        Envelope envelope = computeEnvelope();

        // wrap even works with backwards
        if (grainSec > 0)
        {
            // wrap phasePos in 0..grainSec * 2
            phasePos = frac(phasePos / grainSec / 2) * grainSec * 2;
        }

        const float alpha = computeAlpha(envelope);

        // todo: reconsider order
        if (alpha == 0.0f)
            pos[0] = time;
        else if (alpha == 1.0f)
            pos[1] = time;

        flt readerSec0 = pos[0] + computeLocalSamplePos(envelope, 0);
        flt readerSec1 = pos[1] + computeLocalSamplePos(envelope, 1);

        int display_count = audioFile->getSampleDataSize();

        //        ma_int16 s0 = getSample((ImU32)(readerSec0 * sampleRate));
        //        ma_int16 s1 = getSample((ImU32)(readerSec1 * sampleRate));
        int16 s0 = audioFile->getSample(wrapWithin(readerSec0 * sampleRate, display_count));
        int16 s1 = audioFile->getSample(wrapWithin(readerSec1 * sampleRate, display_count));

        return lerp(s1, s0, alpha);
    }

    // @param grain0Or1 0/1
    // @return in seconds, in 0..grainSec*2 range
    float computeLocalSamplePos(const Envelope envelope, const int grain0Or1) const
    {
        if (grainSec <= 0)
            return 0.0f;

        float shift = 0;

        if (grain0Or1)
            shift = envelope.attackSec + envelope.holdSec;

        return frac((phasePos - shift) / grainSec / 2) * grainSec * 2;
    }
};


class GranularSynth
{
public:

    GranularSynth();
    ~GranularSynth();
};


