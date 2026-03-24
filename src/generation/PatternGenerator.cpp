// SPDX-License-Identifier: GPL-3.0-only

#include "generation/PatternGenerator.h"

#include <algorithm>
#include <limits>

namespace groove {

PatternGenerator::PatternGenerator() : engine_(std::random_device {}()) {}

GrooveScene PatternGenerator::createScene(const GrooveScene& templateScene) {
    GrooveScene scene = normalizedScene(templateScene);
    scene.seed = engine_();

    for (auto& instrument : scene.instruments) {
        resizeInstrumentSteps(instrument, totalStepCount(scene));
        for (auto& step : instrument.steps) {
            step = Step {};
            step.active = false;
            step.locked = false;
            step.velocity = 0.0f;
            step.note = instrument.rootNote;
        }
        generateInstrument(instrument, scene);
        applyInstrumentDefaultsToUnlockedSteps(instrument);
    }

    return scene;
}

GrooveScene PatternGenerator::mutateScene(const GrooveScene& currentScene) {
    GrooveScene mutated = normalizedScene(currentScene);
    mutated.seed = engine_();
    if ((mutated.mutationEnabled == false) || (mutated.mutationAmount <= 0.0f)) {
        return mutated;
    }

    const int totalSteps = totalStepCount(mutated);
    for (auto& instrument : mutated.instruments) {
        const float density = std::clamp(instrument.density, 0.0f, 1.0f);
        const float mutation = std::clamp(mutated.mutationAmount, 0.05f, 1.0f);

        for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
            auto& step = instrument.steps[static_cast<std::size_t>(stepIndex)];
            if (isAnchor(instrument, stepIndex, mutated.stepsPerBar)) {
                continue;
            }

            if (chance(mutation * 0.35f)) {
                step.active = (step.active == false);
            }

            if (step.active) {
                step.velocity = randomVelocity(0.45f, 1.0f);
                const float noteMutationChance = std::clamp((mutation * 0.25f) + (mutated.noteVariation * 0.55f), 0.10f, 0.90f);
                if (chance(noteMutationChance)) {
                    if (instrument.role == InstrumentRole::Bass) {
                        const int lowRange = 5 + static_cast<int>(std::round(mutated.noteVariation * 12.0f));
                        const int highRange = 7 + static_cast<int>(std::round(mutated.noteVariation * 14.0f));
                        step.note = randomNoteForInstrument(mutated, instrument, instrument.rootNote - lowRange, instrument.rootNote + highRange, true);
                    } else if (instrument.role == InstrumentRole::Lead || instrument.role == InstrumentRole::Custom) {
                        const int leadRange = 7 + static_cast<int>(std::round(mutated.noteVariation * 17.0f));
                        step.note = randomNoteForInstrument(mutated, instrument, instrument.rootNote - leadRange, instrument.rootNote + leadRange, false);
                    } else if (instrument.role == InstrumentRole::Perc) {
                        const int variationRange = 2 + static_cast<int>(std::round(mutated.noteVariation * 10.0f));
                        step.note = randomNoteForInstrument(mutated, instrument, instrument.rootNote - variationRange, instrument.rootNote + variationRange, false);
                    } else {
                        step.note = instrument.rootNote;
                    }
                }
            } else if (chance(density * mutation * 0.25f)) {
                step.active = true;
                step.velocity = randomVelocity(0.35f, 0.82f);
                if (instrument.role == InstrumentRole::Bass) {
                    const int lowRange = 5 + static_cast<int>(std::round(mutated.noteVariation * 12.0f));
                    const int highRange = 7 + static_cast<int>(std::round(mutated.noteVariation * 14.0f));
                    step.note = randomNoteForInstrument(mutated, instrument, instrument.rootNote - lowRange, instrument.rootNote + highRange, true);
                } else if (instrument.role == InstrumentRole::Lead || instrument.role == InstrumentRole::Custom) {
                    const int leadRange = 7 + static_cast<int>(std::round(mutated.noteVariation * 17.0f));
                    step.note = randomNoteForInstrument(mutated, instrument, instrument.rootNote - leadRange, instrument.rootNote + leadRange, false);
                } else if (instrument.role == InstrumentRole::Perc) {
                    const int variationRange = 2 + static_cast<int>(std::round(mutated.noteVariation * 10.0f));
                    step.note = randomNoteForInstrument(mutated, instrument, instrument.rootNote - variationRange, instrument.rootNote + variationRange, false);
                } else {
                    step.note = instrument.rootNote;
                }
            }
        }
    }

    return mutated;
}

void PatternGenerator::generateInstrument(InstrumentDefinition& instrument, const GrooveScene& scene) {
    const int stepsPerBar = scene.stepsPerBar;
    const int totalSteps = totalStepCount(scene);
    switch (instrument.role) {
    case InstrumentRole::Kick:
        generateKick(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::Snare:
        generateSnare(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::ClosedHat:
        generateHat(instrument, stepsPerBar, totalSteps, false);
        break;
    case InstrumentRole::OpenHat:
        generateHat(instrument, stepsPerBar, totalSteps, true);
        break;
    case InstrumentRole::Clap:
        generateClap(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::Perc:
        generatePerc(instrument, scene, totalSteps);
        break;
    case InstrumentRole::Bass:
        generateBass(instrument, scene, totalSteps);
        break;
    case InstrumentRole::Lead:
    case InstrumentRole::Custom:
        generateLead(instrument, scene, totalSteps);
        break;
    }
}

void PatternGenerator::generateKick(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        if (stepInBar == 0) {
            instrument.steps[stepIndex] = makeStep(1.0f, instrument.rootNote);
            continue;
        }
        if ((stepInBar == (beat * 2)) && chance(instrument.density * 0.75f)) {
            instrument.steps[stepIndex] = makeStep(0.9f, instrument.rootNote);
            continue;
        }
        if (((stepInBar == (beat - 1)) || (stepInBar == (beat + 1)) || (stepInBar == (beat * 3))) && chance(instrument.density * 0.42f)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.45f, 0.82f), instrument.rootNote);
        }
    }
}

void PatternGenerator::generateSnare(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        if ((stepInBar == beat) || (stepInBar == (beat * 3))) {
            instrument.steps[stepIndex] = makeStep(stepInBar == beat ? 0.92f : 1.0f, instrument.rootNote);
            continue;
        }
        if (((stepInBar == (beat * 2 + beat / 2)) || (stepInBar == (stepsPerBar - 1))) && chance(instrument.density * 0.28f)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.35f, 0.65f), instrument.rootNote);
        }
    }
}

void PatternGenerator::generateHat(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps, bool open) {
    const int subdivision = open ? std::max(1, stepsPerBar / 4) : std::max(1, stepsPerBar / 8);
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool slot = (stepInBar % subdivision) == 0;
        if (slot == false) {
            continue;
        }
        const float chanceScale = open ? 0.35f : 0.86f;
        if (chance(instrument.density * chanceScale)) {
            const float minVelocity = open ? 0.35f : 0.30f;
            const float maxVelocity = open ? 0.62f : 0.78f;
            instrument.steps[stepIndex] = makeStep(randomVelocity(minVelocity, maxVelocity), instrument.rootNote);
        }
    }
}

void PatternGenerator::generateClap(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        if ((stepInBar == beat) && chance(instrument.density * 0.55f)) {
            instrument.steps[stepIndex] = makeStep(0.75f, instrument.rootNote);
            continue;
        }
        if ((stepInBar == (beat * 3)) && chance(instrument.density * 0.80f)) {
            instrument.steps[stepIndex] = makeStep(0.88f, instrument.rootNote);
            continue;
        }
        if ((stepInBar == (beat * 2 + beat / 2)) && chance(instrument.density * 0.22f)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.35f, 0.58f), instrument.rootNote);
        }
    }
}

void PatternGenerator::generatePerc(InstrumentDefinition& instrument, const GrooveScene& scene, int totalSteps) {
    const int stepsPerBar = scene.stepsPerBar;
    const int variationRange = 2 + static_cast<int>(std::round(scene.noteVariation * 10.0f));
    const int accent = std::max(1, stepsPerBar / 8);
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool favored = ((stepInBar + accent) % (accent * 2)) == 0;
        const float probability = favored ? instrument.density * 0.65f : instrument.density * 0.25f;
        if (chance(probability)) {
            instrument.steps[stepIndex] = makeStep(
                randomVelocity(0.30f, 0.72f),
                randomNoteForInstrument(scene, instrument, instrument.rootNote - variationRange, instrument.rootNote + variationRange, false));
        }
    }
}

void PatternGenerator::generateBass(InstrumentDefinition& instrument, const GrooveScene& scene, int totalSteps) {
    const int stepsPerBar = scene.stepsPerBar;
    const int lowRange = 5 + static_cast<int>(std::round(scene.noteVariation * 12.0f));
    const int highRange = 7 + static_cast<int>(std::round(scene.noteVariation * 14.0f));
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool favored = (stepInBar == 0) || (stepInBar == beat) || (stepInBar == (beat * 2)) || (stepInBar == (beat * 3));
        const float probability = favored ? instrument.density * 0.90f : instrument.density * 0.30f;
        if (chance(probability)) {
            const int note = favored
                ? keyRootNearMidi(scene, 36)
                : randomNoteForInstrument(scene, instrument, instrument.rootNote - lowRange, instrument.rootNote + highRange, false);
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.45f, 0.88f), note);
        }
    }

    for (int barIndex = 0; barIndex < (totalSteps / stepsPerBar); ++barIndex) {
        const int downbeat = barIndex * stepsPerBar;
        instrument.steps[downbeat] = makeStep(0.88f, keyRootNearMidi(scene, 36));
    }
}

void PatternGenerator::generateLead(InstrumentDefinition& instrument, const GrooveScene& scene, int totalSteps) {
    const int stepsPerBar = scene.stepsPerBar;
    const int leadRange = 7 + static_cast<int>(std::round(scene.noteVariation * 17.0f));
    const int accent = std::max(1, stepsPerBar / 8);
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool favored = (stepInBar % (accent * 2)) == 0;
        const float favoredBoost = 0.55f + (scene.noteVariation * 0.22f);
        const float offbeatBoost = 0.18f + (scene.noteVariation * 0.20f);
        const float probability = favored ? instrument.density * favoredBoost : instrument.density * offbeatBoost;
        if (chance(probability)) {
            instrument.steps[stepIndex] = makeStep(
                randomVelocity(0.30f, 0.72f),
                randomNoteForInstrument(scene, instrument, instrument.rootNote - leadRange, instrument.rootNote + leadRange, favored));
        }
    }
}

bool PatternGenerator::chance(float probability) {
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    return distribution(engine_) < std::clamp(probability, 0.0f, 1.0f);
}

float PatternGenerator::randomVelocity(float minVelocity, float maxVelocity) {
    std::uniform_real_distribution<float> distribution(minVelocity, maxVelocity);
    return distribution(engine_);
}

int PatternGenerator::randomNoteForInstrument(
    const GrooveScene& scene, const InstrumentDefinition& instrument, int lowMidi, int highMidi, bool favorRoot) {
    lowMidi = std::clamp(lowMidi, 0, 127);
    highMidi = std::clamp(highMidi, 0, 127);
    if (lowMidi > highMidi) {
        std::swap(lowMidi, highMidi);
    }

    std::vector<int> candidates;
    const std::vector<int> intervals = scaleIntervals(scene.scaleMode);
    for (int midiNote = lowMidi; midiNote <= highMidi; ++midiNote) {
        const int pitchClass = (midiNote % 12 + 12) % 12;
        const int relativePitchClass = (pitchClass - scene.keyRoot + 12) % 12;
        if (scene.scaleMode == ScaleMode::Chromatic
            || std::find(intervals.begin(), intervals.end(), relativePitchClass) != intervals.end()) {
            candidates.push_back(midiNote);
        }
    }

    if (candidates.empty()) {
        return std::clamp(instrument.rootNote, 0, 127);
    }

    const float rootBias = std::clamp(0.70f - (scene.noteVariation * 0.55f), 0.10f, 0.70f);
    if (favorRoot && chance(rootBias)) {
        return keyRootNearMidi(scene, std::clamp(instrument.rootNote, lowMidi, highMidi));
    }

    std::uniform_int_distribution<int> distribution(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(distribution(engine_))];
}

int PatternGenerator::keyRootNearMidi(const GrooveScene& scene, int targetMidi) const {
    targetMidi = std::clamp(targetMidi, 0, 127);
    int bestNote = targetMidi;
    int bestDistance = std::numeric_limits<int>::max();
    for (int octave = -1; octave <= 10; ++octave) {
        const int candidate = (octave * 12) + scene.keyRoot;
        if ((candidate < 0) || (candidate > 127)) {
            continue;
        }
        const int distance = std::abs(candidate - targetMidi);
        if ((distance < bestDistance) || ((distance == bestDistance) && (candidate >= targetMidi) && (bestNote < targetMidi))) {
            bestDistance = distance;
            bestNote = candidate;
        }
    }
    return bestNote;
}

bool PatternGenerator::isAnchor(const InstrumentDefinition& instrument, int stepIndex, int stepsPerBar) const {
    const int stepInBar = stepIndex % stepsPerBar;
    const int beat = stepsPerBar / 4;

    switch (instrument.role) {
    case InstrumentRole::Kick:
    case InstrumentRole::Bass:
        return stepInBar == 0;
    case InstrumentRole::Snare:
    case InstrumentRole::Clap:
        return (stepInBar == beat) || (stepInBar == (beat * 3));
    case InstrumentRole::ClosedHat:
    case InstrumentRole::OpenHat:
    case InstrumentRole::Perc:
    case InstrumentRole::Lead:
    case InstrumentRole::Custom:
        return false;
    }
    return false;
}

}  // namespace groove
