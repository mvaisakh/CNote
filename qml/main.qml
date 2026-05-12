import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore
import CNote.Core

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    title: qsTr("CeriumNotes")
    color: "#0F1113"

    readonly property color colorPrimary: "#D0BCFF"
    readonly property color colorBg: "#0F1113"
    readonly property color colorSurface: "#1C1B1F"
    readonly property color colorSurfaceVariant: "#49454F"
    readonly property color colorGlass: "#CC1C1B1F"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1A1C1E" }
            GradientStop { position: 1.0; color: "#0F1113" }
        }
        z: -1
    }

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

        pushEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 300 }
            PropertyAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 300; easing.type: Easing.OutCubic }
        }
        pushExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 300 }
            PropertyAnimation { property: "scale"; from: 1.0; to: 1.05; duration: 300; easing.type: Easing.OutCubic }
        }
        popEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 300 }
            PropertyAnimation { property: "scale"; from: 1.05; to: 1.0; duration: 300; easing.type: Easing.OutCubic }
        }
        popExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 300 }
            PropertyAnimation { property: "scale"; from: 1.0; to: 0.95; duration: 300; easing.type: Easing.OutCubic }
        }

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

            NoteCanvas {
                id: canvas
                anchors.fill: parent
                pdfPath: pdfToLoad

                // Subtle Dotted Background
                Canvas {
                    anchors.fill: parent
                    z: -1
                    opacity: 0.1
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);
                        ctx.fillStyle = "white";
                        var spacing = 30;
                        for (var x = spacing; x < width; x += spacing) {
                            for (var y = spacing; y < height; y += spacing) {
                                ctx.beginPath();
                                ctx.arc(x, y, 1, 0, Math.PI * 2);
                                ctx.fill();
                            }
                        }
                    }
                }
            }

            Toolbar {
                id: toolbar
                canvas: canvas
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 24
                width: Math.min(parent.width * 0.8, 800)
                z: 10 
                
                onBackRequested: {
                    stackView.pop()
                    dashboard.refresh()
                }
                
                onExportRequested: root.openExport()
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
    // Build Version Label
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 8
        text: typeof appVersion !== 'undefined' ? appVersion : "v0.1-unknown"
        color: "#44FFFFFF"
        font.pixelSize: 10
        font.family: "Outfit"
        z: 100
    }
}
