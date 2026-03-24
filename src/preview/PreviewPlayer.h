// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <string>

#include <QObject>

#include "sample/SampleBuffer.h"

QT_BEGIN_NAMESPACE
class QAudioFormat;
class QAudioOutput;
class QBuffer;
QT_END_NAMESPACE

namespace groove {

class PreviewPlayer : public QObject {
public:
    explicit PreviewPlayer(QObject* parent = nullptr);
    ~PreviewPlayer() override;

    bool loadFile(const std::string& path);
    bool hasFile() const;
    void play();
    void stop();
    void seek(std::int64_t deltaMs);
    std::int64_t positionMs() const;
    std::int64_t durationMs() const;
    const SampleBuffer& sample() const;

private:
    void rebuildAudioOutput();
    void setPositionMs(std::int64_t positionMs);
    std::int64_t clampPositionMs(std::int64_t positionMs) const;
    void appendSampleFrame(std::string& pcmData, const QAudioFormat& format, float sampleValue) const;
    int bytesPerFrame() const;

    SampleBuffer sample_;
    QAudioOutput* output_ = nullptr;
    QBuffer* buffer_ = nullptr;
    std::string loadedPath_;
    std::string pcmData_;
    std::int64_t storedPositionMs_ = 0;
    std::int64_t startPositionMs_ = 0;
    std::int64_t durationMs_ = 0;
    bool playing_ = false;
    int outputSampleRate_ = 48000;
    int outputChannelCount_ = 2;
    int outputBytesPerSample_ = 2;
};

}  // namespace groove
