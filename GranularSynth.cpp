#include "GranularSynth.h"
#include "ImGui/imgui.h"
#include <math.h>

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
    float holdSec = 1.0f;
    // in seconds
    float attackSec = 0.1f;
    // 0:mute .. 1:max
    float volume = 0.5f;
    // in seconds
    float speedSec = 1.0f;

    //     ___
    // ___/   \   
    //  0 1 2 3
    int phase = 0;
    // in seconds
    float phasePos = 0;

    // side effect: update phase and phasePos
    // @return 0..1
    float update()
    {
        time += advanceSec / DEVICE_SAMPLE_RATE;
        phasePos += advanceSec / DEVICE_SAMPLE_RATE;

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

        float ret = 1.0f;                          // phase 2

        if (phase == 0)
            ret = 0.0f;                            // phase 0
        else if (phase == 1)
        {
            ret = phasePos / (float)attackSec;     // phase 1
        }
        else if (phase == 3)
        {
            ret = 1.0f - phasePos / (float)attackSec; // phase 3

            assert(ret >= 0.0f && ret <= 1.0f);
        }

        pos[0] += speedSec / DEVICE_SAMPLE_RATE;
        pos[1] += speedSec / DEVICE_SAMPLE_RATE;

        return ret;
    }
};

// @param alpha 0..1
float lerp(float s0, float s1, float alpha)
{
    return s0 * (1.0f - alpha) + s1 * alpha;
}

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

        ma_int16 s0 = getSample((ImU32)(wave.pos[0] * DEVICE_SAMPLE_RATE));
        ma_int16 s1 = getSample((ImU32)(wave.pos[1] * DEVICE_SAMPLE_RATE));

        // 0..1
        float alpha = wave.update();

        ma_int16 s = (ma_int16)(lerp(s0, s1, alpha) * wave.volume);

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

    ImGui::SliderFloat("volume", &wave.volume, 0, 1.0f);
    ImGui::SliderFloat("advance", &wave.advanceSec, 0, 2.0f);
    ImGui::SliderFloat("speed", &wave.speedSec, 0, 1.0f);
    ImGui::SliderFloat("hold", &wave.holdSec, 0, 1.0f);
    ImGui::SliderFloat("attack", &wave.attackSec, 0, 1.0f);

    draw();

    ImGui::End();
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
    
    // Get the draw list
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float normalizedX = 0;

    // Draw the vertical lines
//    for(int i = 0; i < 3; ++i)
    int i = 0;
    {
        double sec = wave.time;
        if (i == 1) sec = wave.pos[0];
        if (i == 2) sec = wave.pos[1];

        normalizedX = ((ImU32)(sec * DEVICE_SAMPLE_RATE) % display_count)/ (float)display_count;
        {
            ImU32 col = IM_COL32(255, 0, 0, 255);
            if (i == 1) col = IM_COL32(0, 255, 0, 255);
            if (i == 2) col = IM_COL32(0, 0, 255, 255);

            float lineScreenX = plotStartPos.x + (normalizedX * plotWidth);
            drawList->AddLine(ImVec2(lineScreenX, plotStartPos.y),ImVec2(lineScreenX, actualPlotBottomRight.y), col, 2.0f);
        }
    }

//    ImGui::Text("time:%.2f", wave.time);
}
    

void GranularSynth::gui()
{
    pimpl_->gui(showWindow);
}
