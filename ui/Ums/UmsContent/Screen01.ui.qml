

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import Ums
import QtQuick.Studio.DesignEffects
import QtMultimedia
import QtQuick.Studio.Effects

Rectangle {
    id: rectangle
    width: Constants.width
    height: Constants.height
    color: "#000000"
    layer.format: ShaderEffectSource.Alpha

    Image {
        id: ums_bg
        x: 0
        y: 0
        width: 720
        height: 1560
        source: "images/ums.001.png"
        fillMode: Image.PreserveAspectFit
    }

    Column {
        id: preview_column
        x: 40
        y: 173
        spacing: 10
        Rectangle {
            id: rgb_frame
            width: 640
            height: 369
            color: "#ffffff"
            radius: 50

            Image {
                id: rgb_image
                x: 10
                y: 10
                width: 620
                height: 349
                source: "images/ums.001.png"
                layer.enabled: true
                layer.effect: OpacityMaskEffect {
                    id: opacityMask
                    visible: true
                    maskSource: rgb_image_mask
                }
                fillMode: Image.Stretch
            }

            Rectangle {
                id: rgb_image_mask
                x: 10
                y: 10
                width: 620
                height: 349
                visible: false
                color: "#000000"
                radius: 40
            }
        }

        Rectangle {
            id: tof_frame
            width: 640
            height: 485
            color: "#ffffff"
            radius: 50
            Image {
                id: tof_image
                x: 10
                y: 10
                width: 620
                height: 465
                source: "images/ums.001.png"
                layer.enabled: true
                layer.effect: OpacityMaskEffect {
                    id: opacityMask1
                    maskSource: tof_image_mask
                }
                cache: false
                fillMode: Image.Stretch
            }

            Rectangle {
                id: tof_image_mask
                x: 10
                y: 10
                width: 620
                height: 465
                visible: false
                color: "#000000"
                radius: 40
            }
        }

        Rectangle {
            id: thermal_frame
            width: 640
            height: 485
            color: "#ffffff"
            radius: 50
            Image {
                id: thermal_image
                x: 10
                y: 10
                width: 620
                height: 465
                source: "images/ums.001.png"
                layer.enabled: true
                layer.effect: OpacityMaskEffect {
                    id: opacityMask2
                    maskSource: thermal_image_mask
                }
                cache: false
                fillMode: Image.Stretch
            }

            Rectangle {
                id: thermal_image_mask
                x: 10
                y: 10
                width: 620
                height: 465
                visible: false
                color: "#000000"
                radius: 40
            }
        }
    }
}
