import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import CeriumNotes

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    title: qsTr("CeriumNotes")
    color: "#121212"

    required property string initialPdf

    FileManager {
        id: fileManager
    }

    FileDialog {
        id: fileDialog
        title: "Import PDF to CeriumNotes"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: {
            var localPath = fileManager.importPdf(selectedFile)
            if (localPath !== "") {
                canvas.pdfPath = localPath
            }
        }
    }

    // Main App Layout
    Item {
        anchors.fill: parent

        Toolbar {
            id: toolbar
            canvas: canvas
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 20
            width: parent.width * 0.7
            z: 10 
            
            // Add a signal or method to trigger opening file
            onOpenRequested: fileDialog.open()
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
