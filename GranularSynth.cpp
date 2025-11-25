#include "GranularSynth.h"
#include "ImGui/imgui.h"
#include "types.h"
#include <math.h>
#include <stdlib.h>

// todo: backwards
// verify/fix crackling

// high quality even with large values
//typedef double flt;
// faster, todo: make sure we stay near 0
typedef float flt;

#pragma warning( disable : 4189 ) // local variable is initialized but not referenced
#pragma warning( disable : 4100 ) // unreferenced parameter

#define MA_NO_DECODING
#define MA_NO_ENCODING
#include "external/miniAudio/miniaudio.h"

unsigned char g_sample[];

#define MA_ASSERT assert

#define DEVICE_FORMAT       ma_format_s16
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000



// https://docs.fileformat.com/audio/wav
const unsigned int g_wavHeader = 44;
const unsigned int getSampleDataSize()
{
    // /2 for DEVICE_FORMAT==ma_format_s16
//    return *(unsigned int*)&g_sample[40] / 2;
    return *(unsigned int*)&g_sample[40] / 2;
}
const unsigned int getSampleRate()
{
    return *(unsigned int*)&g_sample[24];
}
// @param pos = 0..getSampleDataSize()
short getSample(unsigned int pos)
{
    short* data = (short*)&g_sample[g_wavHeader];

    // can be improved
    return data[pos % getSampleDataSize()];
}

// @param alpha 0..1
float lerp(float s0, float s1, float alpha)
{
    return s0 * (1.0f - alpha) + s1 * alpha;
}

float saturate(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

double _floor(double x)
{
    return floor(x);
}
float _floor(float x)
{
    return floorf(x);
}

// also works with negative value
uint32 wrapWithin(const flt value, const uint32 display_count)
{
    flt f = value / display_count;
    // 0..1
    flt g = f - _floor(f);

    return (uint32)(g * display_count);
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

    //  ___
    // /   \___ 
    // 0 1 2 3  : phaseInt
    // x111x000 : return, x is linear crossfade alpha 0..1
    int phaseInt = 0;
    // in seconds
    float phasePos = 0;

    float computeAlpha() const
    {
        const float blendFrac = blendPercent / 100.0f;
        const float holdSec = grainSec * (1.0f - blendFrac);
        const float attackSec = grainSec * blendFrac;

        float alpha = 0.0f;

        if (phaseInt == 0)
        {
            alpha = phasePos / (float)attackSec;
//            assert(alpha >= 0.0f && alpha <= 1.0f);
            alpha = saturate(alpha);
        }
        else if (phaseInt == 1)
            alpha = 1.0f;
        else if (phaseInt == 2)
        {
            alpha = 1.0f - phasePos / (float)attackSec;
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
        ret.attackSec = grainSec * blendFrac;

        return ret;
    }

    // side effect: update phase and phasePos
    // @param deltaTime in seconds 
    // @return value without volume applied
    float update(float deltaTime)
    {
        const unsigned int sampleRate = getSampleRate();

        time += advancePerSec * deltaTime;
        phasePos += pitch * deltaTime;

        Envelope envelope = computeEnvelope();

        if(envelope.attackSec + envelope.holdSec > 0)
        for (;;)
        {
            if (phaseInt == 0)  // ramp up 0, ramp down 1
            {
                if (phasePos < envelope.attackSec)break;
                phaseInt++; phasePos -= envelope.attackSec;
                assert(phasePos >= 0);
            }

            if (phaseInt == 1)  // hold 0
            {
                if (phasePos < envelope.holdSec)break;
                phaseInt++; phasePos -= envelope.holdSec;
                pos[1] = time;
                assert(phasePos >= 0);
            }

            if (phaseInt == 2)  // ramp down 0, ramp down 1
            {
                if (phasePos < envelope.attackSec)break;
                phaseInt = 3; phasePos -= envelope.attackSec;
                assert(phasePos >= 0);
            }

            if (phaseInt == 3)  // 1
            {
                if (phasePos < envelope.holdSec)break;
                phaseInt = 0; phasePos -= envelope.holdSec;
                pos[0] = time;
                assert(phasePos >= 0);
            }
        }

        const float alpha = computeAlpha();

        flt readerSec0 = pos[0] + computeSamplePos(envelope, 0);
        flt readerSec1 = pos[1] + computeSamplePos(envelope, 1);

        static int display_count = getSampleDataSize();

//        ma_int16 s0 = getSample((ImU32)(readerSec0 * sampleRate));
//        ma_int16 s1 = getSample((ImU32)(readerSec1 * sampleRate));
        ma_int16 s0 = getSample(wrapWithin(readerSec0 * sampleRate, display_count));
        ma_int16 s1 = getSample(wrapWithin(readerSec1 * sampleRate, display_count));

        return lerp(s0, s1, alpha);
    }

    // @param grain0Or1 0/1
    // @return in seconds
    float computeSamplePos(const Envelope envelope, const int grain0Or1) const
    {
        unsigned int localPhaseInt = (grain0Or1 == 0) ? phaseInt : ((phaseInt + 2)%4);

        float samplePos = phasePos;
        if (localPhaseInt > 0)
            samplePos += envelope.attackSec;
        if (localPhaseInt > 1)
            samplePos += envelope.holdSec;
        if (localPhaseInt > 2)
            samplePos += envelope.attackSec;

        return samplePos;
    }
};


class GranularSynth::Impl
{
public:
    // @return error code, 0:no erro
    int init();
    void deinit();

    void gui(bool& show);
    void draw();

    ma_device_config deviceConfig;
    ma_device device;
//    Wave audioWave;
//    Wave drawWave;
    Wave audioWave;
    Wave &drawWave = audioWave;

private:

    bool isInit = false;
};

GranularSynth::GranularSynth() : pimpl_(new Impl())
{
}

GranularSynth::~GranularSynth()
{
    pimpl_->deinit();
    delete pimpl_; // Deallocate the implementation object
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    MA_ASSERT(pDevice->playback.channels == DEVICE_CHANNELS);

    Wave& audioWave = *(Wave*)pDevice->pUserData;

    //    ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);

    ma_int16* pFramesOutS16 = (ma_int16*)pOutput;
    for (ma_uint64 iFrame = 0; iFrame < frameCount; iFrame += 1) {
        //        ma_int16 s = ma_waveform_sine_s16(pWaveform->time, pWaveform->config.amplitude);

        //        ma_int16 s = getSample(wave.time);

        float value = audioWave.update(1.0f / DEVICE_SAMPLE_RATE);

        ma_int16 s = (ma_int16)(value * audioWave.volumePercent / 100.0f);

        for (ma_uint64 iChannel = 0; iChannel < DEVICE_CHANNELS; iChannel += 1) {
            pFramesOutS16[iFrame * DEVICE_CHANNELS + iChannel] = s;
        }
    }
}

int GranularSynth::Impl::init()
{
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &audioWave;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
//        printf("Failed to open playback device.\n");
        return -4;
    }

//    printf("Device Name: %s\n\n", device.playback.name);

    if (ma_device_start(&device) != MA_SUCCESS) {
//        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -5;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop__em, 0, 1);
#else

/*    printf("1..9:volume  0:mute  ,.:advance  ;':speed  -=:hold  []:attack  enter:quit\n\n");

    bool print = true;

    for (;;)
    {
        if (_kbhit())
        {
            print = true;
            int c = _getch();
            if (c == 13)
                break;
            if (c == '-' && wave.hold > 1) wave.hold -= 1;
            if (c == '=') wave.hold += 1;

            if (c == '[' && wave.attack > 0) wave.attack -= 1;
            if (c == ']') wave.attack += 1;

            if (c == ',') wave.advance -= 8;
            if (c == '.') wave.advance += 8;

            if (c == ';') wave.speed -= 8;
            if (c == '\'') wave.speed += 8;

            if (c >= '0' && c <= '9') wave.volume = (c - '0') / 9.0f;
        }

        if (print)
        {
            print = false;
            printf("vol:%d  adv:%d  speed:%d  hold:%d  attack:%d               \r", (int)(wave.volume * 9.0f + 0.5f), wave.advance, wave.speed, wave.hold, wave.attack);
        }

    }
*/
#endif


    return 0;
}

void GranularSynth::Impl::deinit()
{
    ma_device_uninit(&device);
}

void GranularSynth::Impl::gui(bool& showWindow)
{
    if (showWindow)
    {
        if(!isInit)
        {
            init();
            isInit = true;
        }
    }
    else
    {
        if (isInit)
        {
            deinit();
            isInit = false;
        }
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 200), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(ImVec2(850, 680), ImGuiCond_FirstUseEver);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 300, main_viewport->WorkPos.y + 220), ImGuiCond_FirstUseEver);
    ImGui::Begin("GranularSynth", &showWindow, ImGuiWindowFlags_NoCollapse);

    ImGui::SliderFloat("volume (mute .. max)", &audioWave.volumePercent, 0, 100.0f, "%.0f%%");
    ImGui::Separator();
    ImGui::SliderFloat("advance (in seconds)", &drawWave.advancePerSec, 0, 2.0f);
    ImGui::SameLine();
    if(ImGui::SmallButton("1##advancePerSec"))
        drawWave.advancePerSec = 1.0f;

    ImGui::SliderFloat("pitch (1 = original)", &drawWave.pitch, 0, 2.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("1##speedPerSec"))
        drawWave.pitch = 1.0f;

    ImGui::Separator();
    ImGui::SliderFloat("grain size (in seconds)", &drawWave.grainSec, 0.00f, 0.5f);
    ImGui::SliderFloat("blend (rect .. triangle)", &drawWave.blendPercent, 0, 100.0f, "%.0f%%");

    ImGuiIO& io = ImGui::GetIO();

    if(&drawWave != &audioWave)
        drawWave.update(io.DeltaTime);

    draw();

    ImGui::End();
}

// @param grain0Or1 0/1
// @param sec0 start pos in seconds
void drawRamp(ImVec2 plotStartPos, ImVec2 actualPlotBottomRight, const Wave &wave, flt sec0, const int grain0Or1)
{
    const float plotWidth = actualPlotBottomRight.x - plotStartPos.x;

    const ImU32 col = (grain0Or1 == 0) ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 0, 255);

    const float blendFrac = wave.blendPercent / 100.0f;
    const float holdSec = wave.grainSec * (1.0f - blendFrac);
    const float attackSec = wave.grainSec * blendFrac;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static int display_count = getSampleDataSize();

    const uint32 sampleRate = getSampleRate();

    // 0.. display_count-1 in samples
    const int32 sampleStart = wrapWithin(sec0 * sampleRate, display_count);

    Wave::Envelope envelope = wave.computeEnvelope();

    for(uint32 wrap = 0; wrap < 2; ++wrap) 
    {
        int32 sample0 = sampleStart;

        if(wrap)
            sample0 -= display_count;

        const int32 sample1 = sample0 + (uint32)(attackSec * sampleRate);
        const int32 sample2 = sample1 + (uint32)(holdSec * sampleRate);
        const int32 sample3 = sample2 + (uint32)(attackSec * sampleRate);

        const float mul = plotWidth / display_count;

        ImVec2 p[4];
        p[0] = ImVec2(plotStartPos.x + sample0 * mul, actualPlotBottomRight.y);
        p[1] = ImVec2(plotStartPos.x + sample1 * mul, plotStartPos.y);
        p[2] = ImVec2(plotStartPos.x + sample2 * mul, plotStartPos.y);
        p[3] = ImVec2(plotStartPos.x + sample3 * mul, actualPlotBottomRight.y);

        // attack up
        drawList->AddLine(p[0], p[1], col, 1.0f);
        // hold
        drawList->AddLine(p[1], p[2], col, 1.0f);
        // attack down
        drawList->AddLine(p[2], p[3], col, 1.0f);

        // current pos
        {
            float alpha = wave.computeAlpha();
            float thisAlpha = (grain0Or1 == 0) ? alpha : (1.0f - alpha);

            // in samples
            float readerPos0 = sample0 + wave.computeSamplePos(envelope, grain0Or1) * sampleRate;

            const float x = (plotStartPos.x + readerPos0 * mul);
            const float midy = lerp(actualPlotBottomRight.y, plotStartPos.y, thisAlpha);
            drawList->AddLine(ImVec2(x, midy), ImVec2(x, actualPlotBottomRight.y), col, 2.0f);
        }

    }
}


void GranularSynth::Impl::draw()
{
    struct Funcs
    {
//        static float Audio(void*, int i) { return sinf(i * 0.1f) * 70; }
        static float Audio(void*, int i) { return (float)getSample(i); }
    };

    static int display_count = getSampleDataSize();
    float (*func)(void*, int) = Funcs::Audio;

    float plotHeight = 80.0f;

    ImVec2 plotStartPos = ImGui::GetCursorScreenPos();

    ImVec2 actualPlotBottomRight = ImVec2(plotStartPos.x + ImGui::CalcItemWidth(), plotStartPos.y + plotHeight);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PlotHistogram("Audio Sample", func, NULL, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));
    ImGui::PopStyleVar();
    //    ImGui::PlotLines("Audio", func, NULL, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));

    const unsigned int sampleRate = getSampleRate();

    // Draw the two grains

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(plotStartPos, actualPlotBottomRight);

    // draw both grains
    for(int i = 0; i < 2; ++i)
    {
        const flt sec = drawWave.pos[i];

        drawRamp(plotStartPos, actualPlotBottomRight, drawWave, sec, i);
    }

    if(1)
    {
        const float plotWidth = actualPlotBottomRight.x - plotStartPos.x;
        const float mul = plotWidth / display_count;
        const ImU32 col = IM_COL32(255,255, 255, 255);
        const ImU32 sample0 = (ImU32)(drawWave.time * sampleRate) % display_count;
        const float lineScreenX = plotStartPos.x + sample0 * mul;
        drawList->AddLine(ImVec2(lineScreenX, plotStartPos.y), ImVec2(lineScreenX, actualPlotBottomRight.y), col, 4.0f);
    }

    drawList->PopClipRect();

//    ImGui::Text("time:%.2f", wave.time);
}
    

void GranularSynth::gui()
{
    pimpl_->gui(showWindow);
}
