#include "GranularSynth.h"
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
// * verify/fix crackling when UI change, need atomic ?
// * load sounds sample

// high quality even with large values
//typedef double flt;
// faster, todo: make sure we stay near 0
typedef float flt;

#pragma warning( disable : 4189 ) // local variable is initialized but not referenced
#pragma warning( disable : 4100 ) // unreferenced parameter

#define MA_NO_DECODING
#define MA_NO_ENCODING
#include "external/miniAudio/miniaudio.h"

#define MA_ASSERT assert

#define DEVICE_FORMAT       ma_format_s16
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000

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

        if(size < wavHeaderSize)       // minimal wav header
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
            uint16 channels = *(uint16*)&fileData[22];
            uint16 bitsPerSample = *(uint16*)&fileData[34];
            if (riff != 'FFIR' ||   // 'RIFF'
                wave != 'EVAW' ||   // 'WAVE'
                fmt != ' tmf' ||    // 'fmt '
                chunkSize != 16 ||  // 16 / 18 / 40
                format != 1 ||      // PCM
                channels != 1 ||      // mono
                bitsPerSample !=16) // 16 bit
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
    // @param pos = 0..getSampleDataSize()
    short getSample(unsigned int pos) const
    {
        short* data = (short*)&fileData[wavHeaderSize];

        const uint32 sampleDataSize = getSampleDataSize();

        // can be improved
        return data[pos % sampleDataSize];
    }
};


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

double frac(double x)
{
    return x - _floor(x);
}
float frac(float x)
{
    return x - _floor(x);
}

// also works with negative value
uint32 wrapWithin(const flt value, const uint32 display_count)
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

    //  ___
    // /   \___ 
    // 0 1 2 3  : phaseInt
    // x111x000 : return, x is linear crossfade alpha 0..1
//    int phaseInt = 0;
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

        if(phasePos < envelope.attackSec)
        {
            alpha = phasePos / (float)envelope.attackSec;
//            assert(alpha >= 0.0f && alpha <= 1.0f);
            alpha = saturate(alpha);
        }
        else if(phasePos < envelope.attackSec + envelope.holdSec)
        {
            return 1.0f;
        }
        else if (phasePos < envelope.attackSec * 2 + envelope.holdSec)
        {
            alpha = 1.0f - (phasePos - envelope.attackSec - envelope.holdSec) / (float)envelope.attackSec;
            //            assert(alpha >= 0.0f && alpha <= 1.0f);
            alpha = saturate(alpha);
        }

/*
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
 */      
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
//        time = frac(time / sampleRate) * sampleRate;

        phasePos += pitch * deltaTime;

        Envelope envelope = computeEnvelope();

        // wrap even works with backwards
        if(grainSec > 0)
        {
            // wrap phasePos in 0..grainSec * 2
            phasePos = frac(phasePos / grainSec / 2) * grainSec * 2;

/*
            float localPhase = phasePos;

            for (;;)
            {
                if (phaseInt == 0)  // ramp up 0, ramp down 1
                {
                    if (localPhase < envelope.attackSec)break;
                    phaseInt++; localPhase -= envelope.attackSec;
                    assert(localPhase >= 0);
                }

                if (phaseInt == 1)  // hold 0
                {
                    if (localPhase < envelope.holdSec)break;
                    phaseInt++; localPhase -= envelope.holdSec;
                    pos[1] = time;
                    assert(localPhase >= 0);
                }

                if (phaseInt == 2)  // ramp down 0, ramp down 1
                {
                    if (localPhase < envelope.attackSec)break;
                    phaseInt = 3; localPhase -= envelope.attackSec;
                    assert(localPhase >= 0);
                }

                if (phaseInt == 3)  // 1
                {
                    if (localPhase < envelope.holdSec)break;
                    phaseInt = 0; localPhase -= envelope.holdSec;
                    pos[0] = time;
                    assert(localPhase >= 0);
                }
            }
*/
        }

        const float alpha = computeAlpha(envelope);

        // todo: reconsider order
        if(alpha == 0.0f)
            pos[0] = time;
        else if (alpha == 1.0f)
            pos[1] = time;

        flt readerSec0 = pos[0] + computeLocalSamplePos(envelope, 0);
        flt readerSec1 = pos[1] + computeLocalSamplePos(envelope, 1);

        int display_count = audioFile->getSampleDataSize();

//        ma_int16 s0 = getSample((ImU32)(readerSec0 * sampleRate));
//        ma_int16 s1 = getSample((ImU32)(readerSec1 * sampleRate));
        ma_int16 s0 = audioFile->getSample(wrapWithin(readerSec0 * sampleRate, display_count));
        ma_int16 s1 = audioFile->getSample(wrapWithin(readerSec1 * sampleRate, display_count));

        return lerp(s1, s0, alpha);
    }

    // @param grain0Or1 0/1
    // @return in seconds, in 0..grainSec*2 range
    float computeLocalSamplePos(const Envelope envelope, const int grain0Or1) const
    {
        if(grainSec <= 0)
            return 0.0f;

        float shift = 0;

        if(grain0Or1)
            shift = envelope.attackSec + envelope.holdSec;
        //    shift = envelope.holdSec;
        //        shift = grainSec;

//        return frac((phasePos) / grainSec / 2) * grainSec * 2 - shift;
        return frac((phasePos - shift) / grainSec / 2) * grainSec * 2;
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


private:

    bool isInit = false;
    std::string fileName;
    // protected by mutexAudioWave
    Wave audioWave;
    // protect audioWave
//    std::mutex mutexAudioWave;
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
            ImGui::Text("Please convert to: .WAV (variant with chuckSize 16), PCM, 16bit, mono, uncompressed");

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


void GranularSynth::Impl::draw()
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
    ImGui::PopStyleVar();
    //    ImGui::PlotLines("Audio", func, NULL, display_count, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, plotHeight));

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
    

void GranularSynth::gui()
{
    pimpl_->gui(showWindow);
}
