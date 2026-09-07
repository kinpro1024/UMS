
#pragma once

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

#include "subsystem.hpp"
#include "thermal.hpp"

namespace ums
{
    class ThermalPresenter : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int frameCounter
                   READ frameCounter
                   NOTIFY frameCounterChanged)
        public:
            ThermalPresenter(Thermal& manager_reference, QObject* parent = nullptr)
                : QObject(parent),
                curr_thermal_reference_(manager_reference)
            {
                connect(this, &ThermalPresenter::frameReady,
                        this, &ThermalPresenter::updateImage,
                        Qt::QueuedConnection);
            }

            int frameCounter() const;
            QImage currentImage() const;

            void loop();

        signals:
            void frameReady(QImage image);
            void frameCounterChanged();

        private slots:
            void updateImage(QImage image);
        
        private:
            void copy();
            void convertAndEmit();

            Thermal& curr_thermal_reference_;
            Thermal::ThermalFrame presenter_buffer_;

            mutable QMutex m_mutex;
            QImage m_image;
            int m_counter = 0;
    };

    class ThermalImageProvider : public QQuickImageProvider
    {
        public:
            explicit ThermalImageProvider(ThermalPresenter* presenter)
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
            ThermalPresenter* presenter_;
    };
}
