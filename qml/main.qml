import QtQuick
import QtQuick.Window
import QtQuick.Controls

import CeriumNotes

Window {
    width: 1280
    height: 800
    visible: true
    title: qsTr("CeriumNotes")
    color: "#121212"

    required property string initialPdf

    Row {
        anchors.fill: parent

        Item {
            width: parent.width
            height: parent.height

            Toolbar {
                id: toolbar
                canvas: canvas
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 20
                width: parent.width * 0.7
                z: 10 // Ensure it's on top
            }

            NoteCanvas {
                id: canvas
                anchors.top: toolbar.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 10
                pdfPath: initialPdf
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 10
                text: qsTr("CeriumNotes Spatial Engine")
                color: "#33ffffff"
                font.pixelSize: 12
            }
        }
    }
}
