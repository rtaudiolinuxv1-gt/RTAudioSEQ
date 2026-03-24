// SPDX-License-Identifier: GPL-3.0-only

#include "preview/PreviewPlayer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>

namespace groove {

PreviewPlayer::PreviewPlayer(QObject* parent)
    : QObject(parent) {
}

PreviewPlayer::~PreviewPlayer() {
    stop();
}

bool PreviewPlayer::loadFile(const std::string& path) {
    stop();

    SampleBuffer loadedSample;
    if (loadedSample.loadFromFile(path) == false) {
        sample_ = SampleBuffer();
        loadedPath_.clear();
        pcmData_.clear();
        durationMs_ = 0;
        rebuildAudioOutput();
        return false;
    }

    sample_ = loadedSample;
    loadedPath_ = path;
    durationMs_ = static_cast<std::int64_t>((static_cast<double>(sample_.frameCount()) * 1000.0) / sample_.sampleRate());
    storedPositionMs_ = 0;
    startPositionMs_ = 0;

    rebuildAudioOutput();
    return true;
}

bool PreviewPlayer::hasFile() const {
    return sample_.isValid();
}

void PreviewPlayer::play() {
    if ((output_ == nullptr) || (buffer_ == nullptr) || (sample_.isValid() == false)) {
        return;
    }

    setPositionMs(storedPositionMs_);
    startPositionMs_ = storedPositionMs_;
    output_->start(buffer_);
    playing_ = true;
}

void PreviewPlayer::stop() {
    if (output_ != nullptr) {
        output_->stop();
    }
    if (buffer_ != nullptr) {
        buffer_->seek(0);
    }
    storedPositionMs_ = 0;
    startPositionMs_ = 0;
    playing_ = false;
}

void PreviewPlayer::seek(std::int64_t deltaMs) {
    if (sample_.isValid() == false) {
        return;
    }

    const std::int64_t targetPosition = clampPositionMs(positionMs() + deltaMs);
    if (playing_ && output_ != nullptr && buffer_ != nullptr) {
        output_->stop();
        setPositionMs(targetPosition);
        startPositionMs_ = storedPositionMs_;
        output_->start(buffer_);
        playing_ = true;
        return;
    }

    setPositionMs(targetPosition);
}

std::int64_t PreviewPlayer::positionMs() const {
    if ((playing_ == false) || (output_ == nullptr)) {
        return storedPositionMs_;
    }

    const std::int64_t currentPosition = startPositionMs_ + (output_->processedUSecs() / 1000);
    return clampPositionMs(currentPosition);
}

std::int64_t PreviewPlayer::durationMs() const {
    return durationMs_;
}

const SampleBuffer& PreviewPlayer::sample() const {
    return sample_;
}

void PreviewPlayer::rebuildAudioOutput() {
    if (output_ != nullptr) {
        output_->stop();
        delete output_;
        output_ = nullptr;
    }
    if (buffer_ != nullptr) {
        buffer_->close();
        delete buffer_;
        buffer_ = nullptr;
    }

    if (sample_.isValid() == false) {
        return;
    }

    QAudioFormat format;
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (deviceInfo.isNull()) {
        return;
    }
    format = deviceInfo.preferredFormat();
    if (format.isValid() == false) {
        return;
    }

    outputSampleRate_ = std::max(1, format.sampleRate());
    outputChannelCount_ = std::max(1, format.channelCount());
    outputBytesPerSample_ = std::max(1, format.sampleSize() / 8);

    const std::size_t outputFrameCount = static_cast<std::size_t>(
        std::max<double>(1.0, std::ceil((static_cast<double>(sample_.frameCount()) * outputSampleRate_) / sample_.sampleRate())));
    pcmData_.clear();
    pcmData_.reserve(outputFrameCount * static_cast<std::size_t>(bytesPerFrame()));
    for (std::size_t outputFrameIndex = 0; outputFrameIndex < outputFrameCount; ++outputFrameIndex) {
        const double sourceFrameIndex = (static_cast<double>(outputFrameIndex) * sample_.sampleRate()) / outputSampleRate_;
        const float sampleValue = sample_.sampleAt(sourceFrameIndex);
        appendSampleFrame(pcmData_, format, sampleValue);
    }

    output_ = new QAudioOutput(format, this);
    output_->setVolume(1.0);
    buffer_ = new QBuffer(this);
    buffer_->setData(pcmData_.data(), static_cast<int>(pcmData_.size()));
    buffer_->open(QIODevice::ReadOnly);

    connect(output_, static_cast<void (QAudioOutput::*)(QAudio::State)>(&QAudioOutput::stateChanged), this, [this](QAudio::State state) {
        if (state == QAudio::IdleState) {
            if (output_ != nullptr) {
                output_->stop();
            }
            storedPositionMs_ = durationMs_;
            playing_ = false;
        } else if (state == QAudio::StoppedState && output_ != nullptr && output_->error() != QAudio::NoError) {
            playing_ = false;
        }
    });
}

void PreviewPlayer::setPositionMs(std::int64_t positionMs) {
    storedPositionMs_ = clampPositionMs(positionMs);
    if ((buffer_ == nullptr) || (sample_.isValid() == false)) {
        return;
    }

    const std::size_t outputFrameIndex = static_cast<std::size_t>((static_cast<double>(storedPositionMs_) * outputSampleRate_) / 1000.0);
    const std::size_t byteOffset = std::min<std::size_t>(
        outputFrameIndex * static_cast<std::size_t>(bytesPerFrame()),
        pcmData_.size());
    buffer_->seek(static_cast<qint64>(byteOffset));
}

std::int64_t PreviewPlayer::clampPositionMs(std::int64_t positionMs) const {
    return std::clamp<std::int64_t>(positionMs, 0, durationMs_);
}

void PreviewPlayer::appendSampleFrame(std::string& pcmData, const QAudioFormat& format, float sampleValue) const {
    const float clampedSample = std::clamp(sampleValue, -1.0f, 1.0f);
    for (int channel = 0; channel < outputChannelCount_; ++channel) {
        switch (format.sampleType()) {
        case QAudioFormat::Float:
        {
            const float value = clampedSample;
            const char* bytes = reinterpret_cast<const char*>(&value);
            pcmData.append(bytes, sizeof(float));
            break;
        }
        case QAudioFormat::UnSignedInt:
            if (format.sampleSize() == 8) {
                const std::uint8_t value = static_cast<std::uint8_t>(std::lrint((clampedSample + 1.0f) * 127.5f));
                pcmData.push_back(static_cast<char>(value));
            } else if (format.sampleSize() == 24) {
                const std::uint32_t value = static_cast<std::uint32_t>(std::lrint((clampedSample + 1.0f) * 8388607.5f));
                pcmData.push_back(static_cast<char>(value & 0xff));
                pcmData.push_back(static_cast<char>((value >> 8) & 0xff));
                pcmData.push_back(static_cast<char>((value >> 16) & 0xff));
            } else {
                const std::uint16_t value = static_cast<std::uint16_t>(std::lrint((clampedSample + 1.0f) * 32767.5f));
                const char* bytes = reinterpret_cast<const char*>(&value);
                pcmData.append(bytes, sizeof(std::uint16_t));
            }
            break;
        case QAudioFormat::SignedInt:
        default:
            if (format.sampleSize() == 8) {
                const std::int8_t value = static_cast<std::int8_t>(std::lrint(clampedSample * 127.0f));
                pcmData.push_back(static_cast<char>(value));
            } else if (format.sampleSize() == 24) {
                const std::int32_t value = static_cast<std::int32_t>(std::lrint(clampedSample * 8388607.0f));
                pcmData.push_back(static_cast<char>(value & 0xff));
                pcmData.push_back(static_cast<char>((value >> 8) & 0xff));
                pcmData.push_back(static_cast<char>((value >> 16) & 0xff));
            } else if (format.sampleSize() == 32) {
                const std::int32_t value = static_cast<std::int32_t>(std::lrint(clampedSample * 2147483647.0f));
                const char* bytes = reinterpret_cast<const char*>(&value);
                pcmData.append(bytes, sizeof(std::int32_t));
            } else {
                const std::int16_t value = static_cast<std::int16_t>(std::lrint(clampedSample * 32767.0f));
                const char* bytes = reinterpret_cast<const char*>(&value);
                pcmData.append(bytes, sizeof(std::int16_t));
            }
            break;
        }
    }
}

int PreviewPlayer::bytesPerFrame() const {
    return outputChannelCount_ * outputBytesPerSample_;
}

}  // namespace groove
