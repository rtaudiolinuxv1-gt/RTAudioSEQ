// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <random>

#include "core/GrooveTypes.h"

namespace groove {

class PatternGenerator {
public:
    PatternGenerator();

    GrooveScene createScene(const GrooveScene& templateScene);
    GrooveScene mutateScene(const GrooveScene& currentScene);

private:
    void generateInstrument(InstrumentDefinition& instrument, const GrooveScene& scene);
    void generateKick(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generateSnare(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generateHat(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps, bool open);
    void generateClap(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generatePerc(InstrumentDefinition& instrument, const GrooveScene& scene, int totalSteps);
    void generateBass(InstrumentDefinition& instrument, const GrooveScene& scene, int totalSteps);
    void generateLead(InstrumentDefinition& instrument, const GrooveScene& scene, int totalSteps);
    bool chance(float probability);
    float randomVelocity(float minVelocity, float maxVelocity);
    int randomNoteForInstrument(const GrooveScene& scene, const InstrumentDefinition& instrument, int lowMidi, int highMidi, bool favorRoot);
    int keyRootNearMidi(const GrooveScene& scene, int targetMidi) const;
    bool isAnchor(const InstrumentDefinition& instrument, int stepIndex, int stepsPerBar) const;

    std::mt19937 engine_;
};

}  // namespace groove
