#pragma once

#include <memory>

#include "core/GrooveTypes.h"
#include "sample/SampleBuffer.h"

namespace groove {

class SampleVoice {
public:
    void trigger(std::shared_ptr<const SampleBuffer> sample, const Step& step, float gateSeconds, double playbackRate = 1.0);
    float render();
    void reset();

private:
    float envelopeLevel() const;

    std::shared_ptr<const SampleBuffer> sample_;
    double position_ = 0.0;
    double increment_ = 1.0;
    float velocity_ = 0.0f;
    float time_ = 0.0f;
    float attackSeconds_ = 0.002f;
    float decaySeconds_ = 0.120f;
    float sustainLevel_ = 0.0f;
    float releaseSeconds_ = 0.080f;
    float holdSeconds_ = 0.0f;
    bool active_ = false;
};

}  // namespace groove
