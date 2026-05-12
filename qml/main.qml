import QtQuick
import QtQuick.Window
import QtQuick.Controls

import CeriumNotes

Window {
    width: 1024
    height: 768
    visible: true
    title: qsTr("CeriumNotes")

    NoteCanvas {
        id: canvas
        anchors.fill: parent
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        text: qsTr("CeriumNotes GPU Engine")
        color: "#55ffffff"
        font.pixelSize: 16
    }
}
