import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 60
    color: "#2a2a2a"
    radius: 12
    opacity: 0.95

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20

        Button {
            text: "Pen"
            flat: true
            font.pixelSize: 16
            palette.buttonText: "white"
        }

        Button {
            text: "Highlighter"
            flat: true
            font.pixelSize: 16
            palette.buttonText: "#aaaaaa"
        }

        Button {
            text: "Eraser"
            flat: true
            font.pixelSize: 16
            palette.buttonText: "#aaaaaa"
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            width: 30; height: 30; radius: 15
            color: "white"
            border.color: "grey"
            border.width: 1
        }
        
        Rectangle {
            width: 30; height: 30; radius: 15
            color: "#ff5555"
        }

        Rectangle {
            width: 30; height: 30; radius: 15
            color: "#55ff55"
        }
    }
}
