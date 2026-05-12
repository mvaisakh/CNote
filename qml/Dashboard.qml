import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: dashboardRoot
    color: "#121212"

    property var fileManager
    signal openPdf(string path)
    signal importRequested()
    signal newNoteRequested(string name)

    property var importedFiles: []

    Component.onCompleted: refresh()

    function refresh() {
        importedFiles = fileManager.getImportedFiles()
    }

    Dialog {
        id: nameDialog
        title: "Name your Note"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        Column {
            spacing: 10
            width: parent.width
            Text { text: "Enter note name:"; color: "white" }
            TextField {
                id: nameField
                width: 250
                placeholderText: "Meeting Notes..."
                focus: true
                color: "white"
                background: Rectangle { color: "#333"; radius: 4 }
                onAccepted: nameDialog.accept()
            }
        }

        onAccepted: {
            dashboardRoot.newNoteRequested(nameField.text)
            nameField.text = ""
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 30

        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "My Library"
                color: "white"
                font.pixelSize: 32
                font.bold: true
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: "+ New Note"
                onClicked: nameDialog.open()
                background: Rectangle {
                    color: "#00adb5"
                    radius: 8
                }
                palette.buttonText: "white"
            }

            Button {
                text: "Import PDF"
                onClicked: dashboardRoot.importRequested()
                background: Rectangle {
                    color: "#333"
                    radius: 8
                    border.color: "#444"
                }
                palette.buttonText: "white"
            }
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: dashboardRoot.importedFiles
            cellWidth: 200
            cellHeight: 250
            clip: true

            delegate: Item {
                width: 180
                height: 230

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 5
                    color: "#1e1e1e"
                    radius: 12
                    border.color: mouseArea.containsMouse ? "cyan" : "#333"
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        // Thumbnail
                        Rectangle {
                            width: parent.width
                            height: 150
                            color: modelData.endsWith(".note") ? "#00adb5" : "#2a2a2a"
                            radius: 8
                            Text {
                                anchors.centerIn: parent
                                text: modelData.endsWith(".note") ? "NOTE" : "PDF"
                                color: modelData.endsWith(".note") ? "white" : "#444"
                                font.pixelSize: 24
                                font.bold: true
                            }
                        }

                        Text {
                            width: parent.width
                            text: {
                                var parts = modelData.split('/')
                                var name = parts[parts.length - 1]
                                var subParts = name.split('_')
                                if (subParts.length > 1) return subParts.slice(1).join('_').replace(".pdf", "").replace(".note", "")
                                return name
                            }
                            color: "white"
                            font.pixelSize: 14
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: dashboardRoot.openPdf(modelData)
                    }
                }
            }
        }
    }
}
