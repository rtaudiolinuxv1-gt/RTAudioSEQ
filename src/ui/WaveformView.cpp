// SPDX-License-Identifier: GPL-3.0-only

#include "ui/WaveformView.h"

#include <algorithm>
#include <cmath>

#include <QPaintEvent>
#include <QPainter>
#include <QPen>

#include "sample/SampleBuffer.h"

namespace groove {

WaveformView::WaveformView(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(150);
}

void WaveformView::setSample(const SampleBuffer* sample) {
    sample_ = sample;
    peaks_.clear();
    positionMs_ = 0;
    durationMs_ = 0;

    if ((sample_ == nullptr) || (sample_->isValid() == false)) {
        update();
        return;
    }

    constexpr std::size_t kPeakCount = 2048;
    peaks_.assign(kPeakCount, 0.0f);
    const std::size_t totalFrames = sample_->frameCount();
    for (std::size_t peakIndex = 0; peakIndex < kPeakCount; ++peakIndex) {
        const std::size_t startFrame = (peakIndex * totalFrames) / kPeakCount;
        const std::size_t endFrame = std::max(startFrame + 1, ((peakIndex + 1) * totalFrames) / kPeakCount);
        float peak = 0.0f;
        for (std::size_t frameIndex = startFrame; frameIndex < endFrame; ++frameIndex) {
            peak = std::max(peak, std::abs(sample_->sampleAt(static_cast<double>(frameIndex))));
        }
        peaks_[peakIndex] = peak;
    }

    durationMs_ = static_cast<std::int64_t>((static_cast<double>(sample_->frameCount()) * 1000.0) / sample_->sampleRate());
    update();
}

void WaveformView::setPlayhead(std::int64_t positionMs, std::int64_t durationMs) {
    positionMs_ = std::max<std::int64_t>(0, positionMs);
    durationMs_ = std::max<std::int64_t>(0, durationMs);
    update();
}

void WaveformView::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor("#171b24"));
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QRect drawRect = rect().adjusted(8, 8, -8, -8);
    painter.setPen(QPen(QColor("#323a4d"), 1));
    painter.drawRect(drawRect);

    if ((sample_ == nullptr) || (sample_->isValid() == false) || peaks_.empty()) {
        painter.setPen(QColor("#95a0b6"));
        painter.drawText(drawRect, Qt::AlignCenter, "No preview waveform loaded");
        return;
    }

    const int midY = drawRect.center().y();
    painter.setPen(QPen(QColor("#293143"), 1));
    painter.drawLine(drawRect.left(), midY, drawRect.right(), midY);

    painter.setPen(QPen(QColor("#58c4dd"), 1));
    for (int x = 0; x < drawRect.width(); ++x) {
        const std::size_t peakIndex = std::min<std::size_t>(
            (static_cast<std::size_t>(x) * peaks_.size()) / std::max(1, drawRect.width()),
            peaks_.size() - 1);
        const int halfHeight = static_cast<int>(peaks_[peakIndex] * (drawRect.height() / 2.0f));
        painter.drawLine(drawRect.left() + x, midY - halfHeight, drawRect.left() + x, midY + halfHeight);
    }

    if (durationMs_ > 0) {
        const double playheadRatio = std::clamp(static_cast<double>(positionMs_) / static_cast<double>(durationMs_), 0.0, 1.0);
        const int playheadX = drawRect.left() + static_cast<int>(playheadRatio * drawRect.width());
        painter.setPen(QPen(QColor("#e0a458"), 2));
        painter.drawLine(playheadX, drawRect.top(), playheadX, drawRect.bottom());
    }
}

}  // namespace groove
