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

        Sidebar {
            height: parent.height
        }

        Item {
            width: parent.width - 250
            height: parent.height

            NoteCanvas {
                id: canvas
                anchors.fill: parent
                pdfPath: initialPdf
            }

            Toolbar {
                id: toolbar
                canvas: canvas
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 20
                width: parent.width * 0.7
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
