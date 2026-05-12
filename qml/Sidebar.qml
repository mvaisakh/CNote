import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 250
    color: "#202020"
    opacity: 0.98

    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Text {
            text: "Pages"
            color: "white"
            font.pixelSize: 20
            font.bold: true
        }

        ListView {
            width: parent.width
            height: parent.height - 50
            model: 5 // Placeholder for 5 pages
            delegate: Rectangle {
                width: parent.width - 20
                height: 150
                color: "#303030"
                radius: 8
                border.color: index === 0 ? "#55aaff" : "transparent"
                border.width: 2

                Text {
                    anchors.centerIn: parent
                    text: "Page " + (index + 1)
                    color: "white"
                }
            }
        }
    }
}
