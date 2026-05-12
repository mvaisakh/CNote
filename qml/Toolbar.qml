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
    signal openRequested()

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20

        // Open Button
        Item {
            width: 40; height: 40
            Rectangle {
                anchors.fill: parent
                radius: 20
                color: "#2a2a2a"
                border.color: "#3a3a3a"
                Text {
                    anchors.centerIn: parent
                    text: "📂"
                    font.pixelSize: 18
                }
            }
            TapHandler {
                onTapped: openRequested()
            }
        }

        Button {
            text: "Pen"
            flat: true
            font.pixelSize: 16
            palette.buttonText: canvas.currentTool === 0 ? "white" : "#aaaaaa"
            onClicked: canvas.currentTool = 0
        }

        Button {
            text: "Highlighter"
            flat: true
            font.pixelSize: 16
            palette.buttonText: canvas.currentTool === 1 ? "white" : "#aaaaaa"
            onClicked: canvas.currentTool = 1
        }

        Button {
            text: "Eraser"
            flat: true
            font.pixelSize: 16
            palette.buttonText: canvas.currentTool === 2 ? "white" : "#aaaaaa"
            onClicked: canvas.currentTool = 2
        }

        Item { Layout.fillWidth: true }

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
