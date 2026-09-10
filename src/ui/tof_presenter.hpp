
#pragma once

#include <mutex>
#include <atomic>

#include <QObject>
#include <QImage>
#include <QQuickImageProvider>

#include "subsystem.hpp"
#include "tof.hpp"

namespace ums
{
    class TofPresenter : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int frameCounter
                   READ frameCounter
                   NOTIFY frameCounterChanged)

        public:
            TofPresenter(Tof& umsd_reference, QObject* parent = nullptr)
                : QObject(parent),
                  curr_tof_reference_(umsd_reference)
            {
                connect(this, &TofPresenter::frameReady,
                        this, &TofPresenter::updateImage,
                        Qt::QueuedConnection);
            }

            int frameCounter() const;
            QImage currentImage() const;
            void abortWorker();

            void loop();

        signals:
            void frameReady(QImage image);
            void frameCounterChanged();

        private slots:
            void updateImage(QImage image);
        
        private:
            void copy();
            void convertAndEmit();

            Tof& curr_tof_reference_;
            Tof::TofFrame presenter_buffer_;
            std::atomic<bool> abort_tof_preview_worker_{false};

            mutable std::mutex m_mutex_;
            QImage m_image_;
            int m_counter_ = 0;
    };

    class TofImageProvider : public QQuickImageProvider
    {
        public:
            explicit TofImageProvider(TofPresenter* presenter)
                : QQuickImageProvider(QQuickImageProvider::Image),
                  presenter_(presenter)
            {}

            QImage requestImage(const QString& id, QSize* size, const QSize&) override
            {

                std::cout<<"img prvdr req: "<<id.toStdString()<<"\n";
                QImage image = presenter_->currentImage();

                if(size)
                {
                    *size = image.size();
                }
                return image;
            }

        private:
            TofPresenter* presenter_;
    };
}
