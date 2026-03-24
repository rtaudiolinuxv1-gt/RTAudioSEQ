// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <string>
#include <vector>

typedef struct _fluid_hashtable_t fluid_settings_t;
typedef struct _fluid_synth_t fluid_synth_t;

namespace groove {

struct SoundFontPreset {
    std::string name;
    int bank = 0;
    int program = 0;
};

class SoundFontSynth {
public:
    SoundFontSynth();
    ~SoundFontSynth();

    void configure(double sampleRate);
    bool load(const std::string& path);
    void clear();
    bool isLoaded() const;
    const std::string& path() const;

    void selectPreset(int channel, int bank, int program);
    void noteOn(int channel, int note, int velocity);
    void noteOff(int channel, int note);
    void allNotesOff();
    void renderFrame(float& left, float& right);
    std::vector<SoundFontPreset> presets() const;

private:
    void recreate();

    fluid_settings_t* settings_ = nullptr;
    fluid_synth_t* synth_ = nullptr;
    double sampleRate_ = 48000.0;
    int soundFontId_ = -1;
    std::string path_;
};

}  // namespace groove
