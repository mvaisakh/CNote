import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CNote.Core

Rectangle {
    id: root
    height: 64
    color: "#AA1C1B1F"
    radius: 32
    border.color: "#33FFFFFF"
    border.width: 1
    
    property var canvas
    signal backRequested()
    signal exportRequested()

    // Inner rim light for glass effect
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: 31
        color: "transparent"
        border.color: "#11FFFFFF"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 12

        Button {
            id: backButton
            onClicked: root.backRequested()
            background: Rectangle {
                implicitWidth: 44
                implicitHeight: 44
                color: backButton.hovered ? "#22FFFFFF" : "transparent"
                radius: 22
                Behavior on color { ColorAnimation { duration: 200 } }
            }
            contentItem: Item {
                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "qrc:/CNote/Core/icons/back.svg"
                    sourceSize: Qt.size(64, 64)
                    smooth: true
                    opacity: backButton.pressed ? 0.5 : 0.9
                }
            }
        }

        Rectangle {
            width: 1
            height: 32
            color: "#22FFFFFF"
        }

        Repeater {
            model: [
                { name: "Pen", icon: "pen.svg", tool: 0 },
                { name: "Highlighter", icon: "highlighter.svg", tool: 1 },
                { name: "Eraser", icon: "eraser.svg", tool: 2 }
            ]

            delegate: Button {
                id: toolButton
                property bool isActive: canvas ? canvas.currentTool === modelData.tool : false
                onClicked: if (canvas) canvas.currentTool = modelData.tool
                
                background: Rectangle {
                    implicitWidth: 48
                    implicitHeight: 48
                    radius: 24
                    color: isActive ? root.colorPrimary : (toolButton.hovered ? "#11FFFFFF" : "transparent")
                    Behavior on color { ColorAnimation { duration: 200 } }
                    
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -2
                        radius: 26
                        color: "transparent"
                        border.color: root.colorPrimary
                        border.width: 2
                        visible: isActive
                    }
                }
                
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/CNote/Core/icons/" + modelData.icon
                        sourceSize: Qt.size(64, 64)
                        smooth: true
                        opacity: isActive ? 1.0 : 0.7
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Color Palette
        Row {
            spacing: 8
            Repeater {
                model: ["#FFFFFF", "#D0BCFF", "#FFB4AB", "#C2E8FF", "#B4F1B4"]
                delegate: Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: modelData
                    border.color: canvas.penColor === modelData ? "white" : "#44FFFFFF"
                    border.width: canvas.penColor === modelData ? 3 : 1
                    
                    scale: tapHandler.pressed ? 0.9 : (tapHandler.hovered ? 1.1 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 150 } }
                    Behavior on border.width { NumberAnimation { duration: 150 } }

                    TapHandler {
                        id: tapHandler
                        onTapped: canvas.penColor = modelData
                    }
                }
            }
        }

        Rectangle {
            width: 1
            height: 32
            color: "#22FFFFFF"
        }

        Button {
            id: exportBtn
            onClicked: root.exportRequested()
            background: Rectangle {
                implicitWidth: 44
                implicitHeight: 44
                color: exportBtn.hovered ? "#22FFFFFF" : "transparent"
                radius: 22
                Behavior on color { ColorAnimation { duration: 200 } }
            }
            contentItem: Item {
                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "qrc:/CNote/Core/icons/export.svg"
                    sourceSize: Qt.size(64, 64)
                    smooth: true
                    opacity: 0.9
                }
            }
        }
    }
}
