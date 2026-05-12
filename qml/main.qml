import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore
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
            var localPath = fileManager.importPdf(fileDialog.selectedFile)
            if (localPath !== "") {
                if (stackView.depth > 1) {
                    stackView.currentItem.canvas.pdfPath = localPath
                } else {
                    stackView.push(canvasComponent, {"pdfToLoad": localPath})
                }
                dashboard.refresh()
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export Annotated PDF"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PDF files (*.pdf)"]
        defaultSuffix: "pdf"
        onAccepted: {
            if (stackView.depth > 1) {
                stackView.currentItem.canvas.exportCurrentPdf(exportDialog.selectedFile)
            }
        }
    }

    function openExport() {
        if (stackView.depth > 1) {
            var currentPath = stackView.currentItem.canvas.pdfPath
            var fileName = currentPath.split('/').pop().replace(".pdf", "_Annotated.pdf")
            exportDialog.currentFile = "file://" + fileManager.getStoragePath() + "/" + fileName
            exportDialog.open()
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: dashboard

        Dashboard {
            id: dashboard
            fileManager: fileManager
            onOpenPdf: (path) => stackView.push(canvasComponent, {"pdfToLoad": path})
            onImportRequested: fileDialog.open()
            onNewNoteRequested: (name) => {
                var path = fileManager.createNewNote(name)
                stackView.push(canvasComponent, {"pdfToLoad": path})
            }
        }
    }

    Component {
        id: canvasComponent
        
        Item {
            property alias canvas: canvas
            property string pdfToLoad

            Toolbar {
                id: toolbar
                canvas: canvas
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 20
                width: parent.width * 0.7
                z: 10 
                
                onBackRequested: {
                    stackView.pop()
                    dashboard.refresh()
                }
                
                onExportRequested: root.openExport()
            }

            NoteCanvas {
                id: canvas
                anchors.top: toolbar.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 10
                pdfPath: pdfToLoad
            }
            
            Component.onCompleted: {
                if (pdfToLoad === "" && root.initialPdf !== "") {
                    pdfPath = root.initialPdf
                }
            }
        }
    }

    // Handle initial PDF from command line
    Component.onCompleted: {
        if (initialPdf !== "") {
            stackView.push(canvasComponent, {"pdfToLoad": initialPdf})
        }
    }
}
