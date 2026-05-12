import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 1024
    height: 768
    visible: true
    title: qsTr("CeriumNotes")

    Rectangle {
        anchors.fill: parent
        color: "#1a1a1a" // Deep dark background for premium feel

        Text {
            anchors.centerIn: parent
            text: qsTr("CeriumNotes Engine Initialized")
            color: "#ffffff"
            font.pixelSize: 24
            font.family: "Inter" // Placeholder for modern typography
        }
    }
}
