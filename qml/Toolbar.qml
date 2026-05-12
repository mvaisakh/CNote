import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 60
    color: "#2a2a2a"
    radius: 12
    opacity: 0.95
    
    property var canvas
    signal backRequested()
    signal exportRequested()

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        Button {
            onClicked: root.backRequested()
            background: Rectangle {
                color: "transparent"
                radius: 15
                Rectangle {
                    anchors.fill: parent
                    color: "white"
                    opacity: parent.parent.hovered ? 0.1 : 0
                }
            }
            contentItem: Item {
                implicitWidth: 44
                implicitHeight: 44
                Image {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: "qrc:/CeriumNotes/icons/back.svg"
                    sourceSize: Qt.size(64, 64)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }
        }

        Row {
            spacing: 15
            Layout.alignment: Qt.AlignHCenter

            Repeater {
                model: [
                    { name: "Pen", icon: "pen.svg", tool: 0 },
                    { name: "Highlighter", icon: "highlighter.svg", tool: 1 },
                    { name: "Eraser", icon: "eraser.svg", tool: 2 }
                ]

                delegate: Button {
                    property bool isActive: canvas ? canvas.currentTool === modelData.tool : false
                    onClicked: if (canvas) canvas.currentTool = modelData.tool
                    
                    background: Rectangle {
                        implicitWidth: 44
                        implicitHeight: 44
                        radius: 22
                        color: isActive ? "#00adb5" : "transparent"
                        opacity: isActive ? 1.0 : (hovered ? 0.1 : 0)
                        
                        Rectangle {
                            anchors.fill: parent
                            color: "white"
                            radius: 22
                            visible: !isActive && parent.parent.hovered
                        }
                    }
                    
                    contentItem: Item {
                        implicitWidth: 44
                        implicitHeight: 44
                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            source: "qrc:/CeriumNotes/icons/" + modelData.icon
                            sourceSize: Qt.size(64, 64)
                            fillMode: Image.PreserveAspectFit
                            opacity: isActive ? 1.0 : 0.7
                            smooth: true
                        }
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        Button {
            onClicked: root.exportRequested()
            background: Rectangle {
                implicitWidth: 44
                implicitHeight: 44
                color: "transparent"
                radius: 22
                Rectangle {
                    anchors.fill: parent
                    color: "white"
                    opacity: parent.parent.hovered ? 0.1 : 0
                    radius: 22
                }
            }
            contentItem: Item {
                implicitWidth: 44
                implicitHeight: 44
                Image {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: "qrc:/CeriumNotes/icons/export.svg"
                    sourceSize: Qt.size(64, 64)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }
        }

        Rectangle {
            width: 30; height: 30; radius: 15
            color: "white"
            border.color: canvas.penColor === color ? "cyan" : "grey"
            border.width: canvas.penColor === color ? 2 : 1
            TapHandler {
                onTapped: canvas.penColor = "white"
            }
        }
        
        Rectangle {
            width: 30; height: 30; radius: 15
            color: "#ff5555"
            border.color: "cyan"
            border.width: canvas.penColor === color ? 2 : 0
            TapHandler {
                onTapped: canvas.penColor = "#ff5555"
            }
        }

        Rectangle {
            width: 30; height: 30; radius: 15
            color: "#55ff55"
            border.color: "cyan"
            border.width: canvas.penColor === color ? 2 : 0
            TapHandler {
                onTapped: canvas.penColor = "#55ff55"
            }
        }
    }
}
