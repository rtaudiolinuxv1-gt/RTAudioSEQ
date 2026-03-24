// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <vector>

#include <QWidget>

namespace groove {

class SampleBuffer;

class WaveformView : public QWidget {
public:
    explicit WaveformView(QWidget* parent = nullptr);

    void setSample(const SampleBuffer* sample);
    void setPlayhead(std::int64_t positionMs, std::int64_t durationMs);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const SampleBuffer* sample_ = nullptr;
    std::vector<float> peaks_;
    std::int64_t positionMs_ = 0;
    std::int64_t durationMs_ = 0;
};

}  // namespace groove
