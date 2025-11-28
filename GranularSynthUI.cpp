// ImGui UI and Windows miniaudio implementation

#include "GranularSynthUI.h"
#include "ImGui/imgui.h"
#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <memory> // std::unique_ptr<>
#include <vector>
#include <mutex>
#include "ImGui/imgui_stdlib.h"
#include "FileIODialog.h"

// todo: 
// * verify/fix when UI change, need atomic ?
// * set loop region
// * set and switch between presets, when a loop is finished or instant
// * snap / grid
// * draw waveform nicer
// * Pico friendly user interface

#define DEVICE_FORMAT       ma_format_s16
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000

#pragma warning( disable : 4189 ) // local variable is initialized but not referenced
#pragma warning( disable : 4100 ) // unreferenced parameter

#define MA_NO_DECODING
#define MA_NO_ENCODING
#include "external/miniAudio/miniaudio.h"

#define MA_ASSERT assert

#include "GranularSynth.h"


class GranularSynthUI::Impl
{
public:
    // @return error code, 0:no erro
    int init();
    void deinit();

    void gui(bool& show);
    void draw();

    ma_device_config deviceConfig;
    ma_device device;


private:

    bool isInit = false;
    std::string fileName;
    // protected by mutexAudioWave
    Wave audioWave;
    // protect audioWave
//    std::mutex mutexAudioWave;
};

GranularSynthUI::GranularSynthUI() : pimpl_(new Impl())
{
}

GranularSynthUI::~GranularSynthUI()
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

int GranularSynthUI::Impl::init()
{
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &audioWave;

    extern unsigned char g_sample[];

    {
        auto audioFile = std::make_unique<AudioFile>();
        if(audioFile->LoadWav("audioSample.wav"))
            audioWave.setAudioFile(std::move(audioFile));
    }
//    audioWave.audioFile->LoadWav("pickup_mana.wav");
//    audioWave.audioFile->LoadWav("!step on stage_2.wav");


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

void GranularSynthUI::Impl::deinit()
{
    ma_device_uninit(&device);
}

void GranularSynthUI::Impl::gui(bool& showWindow)
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

    {
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::CalcTextSize("...").x + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetStyle().FramePadding.x * 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemInnerSpacing.x, ImGui::GetStyle().ItemSpacing.y));
        ImGui::InputText("##Sound file", &fileName);
        ImGui::SameLine();
        if(ImGui::Button("..."))
        {
            CFileIODialog dialog;

            std::string oldFileName = fileName;

            if(dialog.FileDialogLoad("Load audio file", ".wav", 
                "wav files (*.wav)\0*.wav\0"
                "all files (*.*)\0*.*\0"
                "\0", fileName))
            {
                auto audioFile = std::make_unique<AudioFile>();
                if (audioFile->LoadWav(fileName.c_str()))
                    audioWave.setAudioFile(std::move(audioFile));
                else
                {
                    fileName = oldFileName;
                    ImGui::OpenPopup("Error##FileFormat");
                }
            }
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Sound file");
        ImGui::PopStyleVar();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)); // Center the modal
        if (ImGui::BeginPopupModal("Error##FileFormat", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("File format not supported!");
            ImGui::Text("Please convert to: .WAV (variant with chuckSize 16), PCM, 16bit, mono/stereo, uncompressed");

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus(); // Set OK button as default focus
            ImGui::EndPopup();
        }
    }


    ImGui::SliderFloat("volume (mute .. max)", &audioWave.volumePercent, 0, 100.0f, "%.0f%%");
    ImGui::Separator();
    ImGui::SliderFloat("advance (in seconds)", &audioWave.advancePerSec, -2.0f, 2.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("0##advancePerSec"))
        audioWave.advancePerSec = 0.0f;
    ImGui::SameLine();
    if(ImGui::SmallButton("1##advancePerSec"))
        audioWave.advancePerSec = 1.0f;

    ImGui::SliderFloat("pitch (1 = original)", &audioWave.pitch, -2.0f, 2.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("0##speedPerSec"))
        audioWave.pitch = 0.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1##speedPerSec"))
        audioWave.pitch = 1.0f;

    ImGui::Separator();
    ImGui::SliderFloat("grain size (in seconds)", &audioWave.grainSec, 0.00f, 0.5f);
    ImGui::SliderFloat("blend (rect .. triangle)", &audioWave.blendPercent, 0, 100.0f, "%.0f%%");

    ImGuiIO& io = ImGui::GetIO();

    draw();

    ImGui::End();
}

// @param grain0Or1 0/1
// @param audioFile same as wave.getAudioFile() but passing it here is less calling overhead
// @param sec0 start pos in seconds
void drawRamp(ImVec2 plotStartPos, ImVec2 actualPlotBottomRight, Wave &wave, std::shared_ptr<AudioFile> audioFile, flt sec0, const int grain0Or1)
{
    const float plotWidth = actualPlotBottomRight.x - plotStartPos.x;

    const ImU32 col = (grain0Or1 == 0) ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 0, 255);

    const float blendFrac = wave.blendPercent / 100.0f;
    const float holdSec = wave.grainSec * (1.0f - blendFrac);
    const float attackSec = wave.grainSec * blendFrac;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    int display_count = audioFile->getSampleDataSize();

    const uint32 sampleRate = audioFile->getSampleRate();

    // 0.. display_count-1 in samples
    const int32 sampleStart = wrapWithin(sec0 * sampleRate, display_count);

    Wave::Envelope envelope = wave.computeEnvelope();

    // samples to pixel
    const float mul = plotWidth / display_count;

    for(uint32 wrap = 0; wrap < 2; ++wrap)
    {
        int32 sample0 = sampleStart;

        if(wrap)
            sample0 -= display_count;

        const int32 sample1 = sample0 + (uint32)(attackSec * sampleRate);
        const int32 sample2 = sample1 + (uint32)(holdSec * sampleRate);
        const int32 sample3 = sample2 + (uint32)(attackSec * sampleRate);

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
            float alpha = wave.computeAlpha(envelope);
            float thisAlpha = (grain0Or1 == 0) ? alpha : (1.0f - alpha);

            // in samples
            float readerPos0 = sample0 + wave.computeLocalSamplePos(envelope, grain0Or1) * sampleRate;

            const float x = (plotStartPos.x + readerPos0 * mul);
            const float midy = lerp(actualPlotBottomRight.y, plotStartPos.y, thisAlpha);
            drawList->AddLine(ImVec2(x, midy), ImVec2(x, actualPlotBottomRight.y), col, 2.0f);
        }

    }
}


void GranularSynthUI::Impl::draw()
{
    // for PlotHistogram
    struct Funcs
    {
        static float Audio(void*userdata, int i)
        {
            std::shared_ptr<AudioFile>& audioFile = *(std::shared_ptr<AudioFile>*)userdata;
            return (float)audioFile->getSample(i);
        }
    };

    auto audioFile = audioWave.getAudioFile();

    int display_count = audioFile->getSampleDataSize();
    float (*func)(void*, int) = Funcs::Audio;

    float plotHeight = 80.0f;

    ImVec2 plotStartPos = ImGui::GetCursorScreenPos();

    ImVec2 actualPlotBottomRight = ImVec2(plotStartPos.x + ImGui::CalcItemWidth(), plotStartPos.y + plotHeight);

//    ImVec2 actualPlotBottomRight = ImVec2(plotStartPos.x + 1024, plotStartPos.y + plotHeight);
//    ImGui::SetNextItemWidth(1024);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PlotHistogram("Audio Sample", func, &audioFile, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));
//    ImGui::PlotLines("Audio", func, &audioFile, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));
    ImGui::PopStyleVar();

    const unsigned int sampleRate = audioFile->getSampleRate();

    // Draw the two grains

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(plotStartPos, actualPlotBottomRight);

    // draw both grains
    for(int i = 0; i < 2; ++i)
    {
        const flt sec = audioWave.pos[i];

        drawRamp(plotStartPos, actualPlotBottomRight, audioWave, audioFile, sec, i);
    }

    if(1)
    {
        const float plotWidth = actualPlotBottomRight.x - plotStartPos.x;
        const float mul = plotWidth / display_count;
        const ImU32 col = IM_COL32(255,255, 255, 255);
        const ImU32 sample0 = (ImU32)(audioWave.time * sampleRate) % display_count;
        const float lineScreenX = plotStartPos.x + sample0 * mul;
        drawList->AddLine(ImVec2(lineScreenX, plotStartPos.y), ImVec2(lineScreenX, actualPlotBottomRight.y), col, 4.0f);
    }

    drawList->PopClipRect();

//    ImGui::Text("time:%.2f", wave.time);
}
    

void GranularSynthUI::gui()
{
    pimpl_->gui(showWindow);
}
