// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "core/GrooveTypes.h"
#include "recording/AudioRecorder.h"
#include "sample/SampleBuffer.h"
#include "sample/SampleVoice.h"
#include "sf2/SoundFontSynth.h"

namespace groove {

class JackEngine {
public:
    JackEngine();
    ~JackEngine();

    bool start(const std::string& clientName);
    void stop();

    void setScene(const GrooveScene& scene);
    GrooveScene sceneSnapshot() const;

    bool loadSample(int instrumentIndex, const std::string& path);
    void clearSample(int instrumentIndex);
    bool loadSoundfont(const std::string& path);
    void clearSoundfont();
    std::vector<SoundFontPreset> soundfontPresets() const;
    bool autoConnectOutputs();
    bool loadPreview(const std::string& path);
    void playPreview();
    void stopPreview();
    void seekPreview(std::int64_t deltaMs);
    std::int64_t previewPositionMs() const;
    std::int64_t previewDurationMs() const;
    void setPreviewGainDb(float gainDb);
    float previewGainDb() const;

    bool startRecording(const std::string& path, AudioFileFormat format);
    void stopRecording();
    bool isRecording() const;
    std::string recordingPath() const;

    void setPlaying(bool playing);
    bool isPlaying() const;
    int currentStep() const;
    int consumeCompletedBars();

private:
    struct NoteState {
        bool active = false;
        std::uint8_t note = 0;
        std::uint8_t channel = 0;
        int samplesRemaining = 0;
    };

    struct PreviewStereoBuffer {
        std::vector<float> left;
        std::vector<float> right;
        double sampleRate = 48000.0;

        bool isValid() const {
            return (left.empty() == false) && (left.size() == right.size()) && (sampleRate > 0.0);
        }

        std::size_t frameCount() const {
            return left.size();
        }

        float leftSampleAt(double frameIndex) const;
        float rightSampleAt(double frameIndex) const;
    };

    static int processShim(jack_nframes_t nframes, void* arg);
    int process(jack_nframes_t nframes);
    void ensureRuntimeSize(std::size_t count);
    void renderStepLocked(const GrooveScene& snapshot, void* midiBuffer, jack_nframes_t frameOffset, double stepDuration);
    double nextStepDuration(const GrooveScene& snapshot) const;
    void resetTransportLocked();
    double samplePlaybackRate(const Step& step, const InstrumentLayerSettings& settings, const SampleBuffer& sample) const;
    void writePendingTransportLocked(void* midiBuffer);
    void advanceMidiNotesLocked(void* midiBuffer, jack_nframes_t frameOffset);
    void writeMidiNoteOn(void* midiBuffer, jack_nframes_t frameOffset, std::uint8_t channel, std::uint8_t note, std::uint8_t velocity);
    void writeMidiNoteOff(void* midiBuffer, jack_nframes_t frameOffset, std::uint8_t channel, std::uint8_t note);

    jack_client_t* client_ = nullptr;
    jack_port_t* leftPort_ = nullptr;
    jack_port_t* rightPort_ = nullptr;
    jack_port_t* midiOutPort_ = nullptr;

    mutable std::mutex stateMutex_;
    GrooveScene scene_ {};
    std::vector<SampleVoice> sampleVoices_;
    std::vector<std::shared_ptr<const SampleBuffer>> sampleBuffers_;
    PreviewStereoBuffer previewBuffer_;
    std::vector<NoteState> midiNotes_;
    std::vector<NoteState> soundfontNotes_;
    SoundFontSynth soundfont_;

    std::atomic<bool> playing_ {false};
    std::atomic<int> currentStep_ {-1};
    std::atomic<int> completedBars_ {0};
    std::atomic<bool> pendingMidiStart_ {false};
    std::atomic<bool> pendingMidiStop_ {false};

    double sampleRate_ = 48000.0;
    double stepSamplesRemaining_ = 0.0;
    int transportStep_ = 0;
    double previewFrame_ = 0.0;
    bool previewPlaying_ = false;
    float previewGainDb_ = 0.0f;

    AudioRecorder recorder_ {};
};

}  // namespace groove
