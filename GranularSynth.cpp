#include "GranularSynth.h"
#include "ImGui/imgui.h"
#include <math.h>

#pragma warning( disable : 4189 ) // local variable is initialized but not referenced

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
    return *(unsigned int*)&g_sample[40] / 2;
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

struct Wave
{
    // in seconds
    double time = 0;
    // in seconds
    float advanceSec = 1.0f;

    // in seconds [mode0, mode1] = { time, time }
    double pos[2] = { 0, 0 };

    // volume shape

    // in seconds
    float grainSec = 0.1f;
    // 0:hard..100:max
    float blendPercent = 10.0f;
    // 0:mute .. 100:max
    float volumePercent = 40.0f;
    // in seconds
    float speedSec = 1.0f;

    //     ___
    // ___/   \   
    //  0 1 2 3
    int phase = 0;
    // in seconds
    float phasePos = 0;

    float computeAlpha() const
    {
        const float blendFrac = blendPercent / 100.0f;
        const float holdSec = grainSec * (1.0f - blendFrac);
        const float attackSec = grainSec * blendFrac;

        float alpha = 1.0f;                             // phase 2

        if (phase == 0)
            alpha = 0.0f;                               // phase 0
        else if (phase == 1)
        {
            alpha = phasePos / (float)attackSec;        // phase 1
            assert(alpha >= 0.0f && alpha <= 1.0f);
        }
        else if (phase == 3)
        {
            alpha = 1.0f - phasePos / (float)attackSec; // phase 3
            assert(alpha >= 0.0f && alpha <= 1.0f);
        }
        
        return alpha;
    }

    // side effect: update phase and phasePos
    // @return value without volume applied
    float update()
    {
        time += advanceSec / DEVICE_SAMPLE_RATE;
        phasePos += advanceSec / DEVICE_SAMPLE_RATE;

        const float blendFrac = blendPercent / 100.0f;
        const float holdSec = grainSec * (1.0f - blendFrac);
        const float attackSec = grainSec * blendFrac;

        for (;;)
        {
            if (phase == 0)
            {
                if (phasePos < holdSec)break;
                phase++; phasePos -= holdSec;
                pos[0] = time;
                assert(phasePos >= 0);
            }

            if (phase == 1)
            {
                if (phasePos < attackSec)break;
                phase++; phasePos -= attackSec;
                assert(phasePos >= 0);
            }

            if (phase == 2)
            {
                if (phasePos < holdSec)break;
                phase++; phasePos -= holdSec;
                pos[1] = time;
                assert(phasePos >= 0);
            }

            if (phase == 3)
            {
                if (phasePos < attackSec)break;
                phase = 0; phasePos -= attackSec;
                assert(phasePos >= 0);
            }
        }


        //     ___
        // ___/   \   
        //  0 1 2 3 : phase
        // 000x111x : return, x is linear crossfade area 0..1

        const float alpha = computeAlpha();

        // this could be refactored
        float samplePos = computeSamplePos();

        double readerPos0 = pos[0] + samplePos / DEVICE_SAMPLE_RATE;
        double readerPos1 = pos[1] + samplePos / DEVICE_SAMPLE_RATE;

        ma_int16 s0 = getSample((ImU32)(readerPos0 * DEVICE_SAMPLE_RATE));
        ma_int16 s1 = getSample((ImU32)(readerPos1 * DEVICE_SAMPLE_RATE));

        return lerp(s0, s1, alpha);
    }

    // @return in seconds
    float computeSamplePos() const
    {
        // can be optimized
        const float blendFrac = blendPercent / 100.0f;
        const float holdSec = grainSec * (1.0f - blendFrac);
        const float attackSec = grainSec * blendFrac;

        float samplePos = phasePos;
        if (phase > 0)
            samplePos += holdSec;
        if (phase > 1)
            samplePos += attackSec;
        if (phase > 2)
            samplePos += holdSec;

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
    Wave wave;

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

float saturate(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    MA_ASSERT(pDevice->playback.channels == DEVICE_CHANNELS);

    Wave& wave = *(Wave*)pDevice->pUserData;

    //    ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);

    ma_int16* pFramesOutS16 = (ma_int16*)pOutput;
    for (ma_uint64 iFrame = 0; iFrame < frameCount; iFrame += 1) {
        //        ma_int16 s = ma_waveform_sine_s16(pWaveform->time, pWaveform->config.amplitude);

        //        ma_int16 s = getSample(wave.time);

        float value = wave.update();

        ma_int16 s = (ma_int16)(value * wave.volumePercent / 100.0f);

        for (ma_uint64 iChannel = 0; iChannel < DEVICE_CHANNELS; iChannel += 1) {
            pFramesOutS16[iFrame * DEVICE_CHANNELS + iChannel] = s;
        }
    }


    (void)pInput;
}

int GranularSynth::Impl::init()
{
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &wave;

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

    ImGui::SliderFloat("volume (mute .. max)", &wave.volumePercent, 0, 100.0f, "%.0f%%");
    ImGui::Separator();
    ImGui::SliderFloat("advance (in seconds)", &wave.advanceSec, 0, 2.0f);
    ImGui::SliderFloat("speed (in seconds)", &wave.speedSec, 0, 1.0f);
    ImGui::Separator();
    ImGui::SliderFloat("hold (in seconds)", &wave.grainSec, 0, 0.1f);
    ImGui::SliderFloat("blend (rect .. triangle)", &wave.blendPercent, 0, 100.0f, "%.0f%%");

    draw();

    ImGui::End();
}

void drawRamp(ImVec2 plotStartPos, float plotWidth, ImVec2 actualPlotBottomRight, const Wave &wave, double sec0, const int grain0Or1)
{
    const ImU32 col = (grain0Or1 == 0) ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 0, 255);

    const float blendFrac = wave.blendPercent / 100.0f;
    const float holdSec = wave.grainSec * (1.0f - blendFrac);
    const float attackSec = wave.grainSec * blendFrac;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static int display_count = getSampleDataSize();

    // 0.. display_count-1 in samples
    const ImU32 sample0 = (ImU32)(sec0 * DEVICE_SAMPLE_RATE) % display_count;
    const ImU32 sample1 = sample0 + (ImU32)(attackSec * DEVICE_SAMPLE_RATE);
    const ImU32 sample2 = sample1 + (ImU32)(holdSec * DEVICE_SAMPLE_RATE);
    const ImU32 sample3 = sample2 + (ImU32)(attackSec * DEVICE_SAMPLE_RATE);

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
        float thisAlpha = (grain0Or1 == 1) ? (1.0f - alpha) : alpha;

        // in samples
        float readerPos0 = sample0 + wave.computeSamplePos() * DEVICE_SAMPLE_RATE;

        const float x = (plotStartPos.x + readerPos0 * mul);
        const float midy = lerp(actualPlotBottomRight.y, plotStartPos.y, alpha);
        drawList->AddLine(ImVec2(x, midy), ImVec2(x, actualPlotBottomRight.y), col, 2.0f);
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

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PlotHistogram("Audio Sample", func, NULL, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));
//    ImGui::PlotLines("Audio", func, NULL, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));

    ImVec2 actualPlotBottomRight = ImVec2(plotStartPos.x + ImGui::GetContentRegionAvail().x, plotStartPos.y + plotHeight);

    float plotWidth = actualPlotBottomRight.x - plotStartPos.x;
    
    // Draw the two grains
    for(int i = 0; i < 2; ++i)
    {
        const double sec = wave.pos[i];

        drawRamp(plotStartPos, plotWidth, actualPlotBottomRight, wave, sec, i);
        // draw another to make it appear cyclic
        drawRamp(plotStartPos, plotWidth, actualPlotBottomRight, wave, sec - display_count / (float)DEVICE_SAMPLE_RATE, i);
    }

    if(0)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float mul = plotWidth / display_count;
        const ImU32 col = IM_COL32(0, 0, 255, 255);
        const ImU32 sample0 = (ImU32)(wave.time * DEVICE_SAMPLE_RATE) % display_count;
        const float lineScreenX = plotStartPos.x + sample0 * mul;
        drawList->AddLine(ImVec2(lineScreenX, plotStartPos.y), ImVec2(lineScreenX, actualPlotBottomRight.y), col, 2.0f);
    }

//    ImGui::Text("time:%.2f", wave.time);
}
    

void GranularSynth::gui()
{
    pimpl_->gui(showWindow);
}
