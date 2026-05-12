import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import CNote.Core

Rectangle {
    id: dashboardRoot
    color: "transparent"

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
        title: "New Note"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: root.colorSurface
            radius: 28
            border.color: "#33FFFFFF"
        }

        Column {
            spacing: 20
            width: parent.width
            padding: 10
            Text { 
                text: "What shall we call this note?"
                color: "white" 
                font.pixelSize: 18
                font.weight: Font.Medium
            }
            TextField {
                id: nameField
                width: parent.width
                placeholderText: "Meeting with the team..."
                color: "white"
                font.pixelSize: 16
                padding: 12
                background: Rectangle { 
                    color: "#11FFFFFF"
                    radius: 12
                    border.color: parent.activeFocus ? root.colorPrimary : "#22FFFFFF"
                    border.width: 1
                }
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
        spacing: 32

        RowLayout {
            Layout.fillWidth: true
            
            Column {
                spacing: 4
                Text {
                    text: "My Library"
                    color: "white"
                    font.pixelSize: 36
                    font.weight: Font.Bold
                }
                Text {
                    text: "Welcome back. Pick up where you left off."
                    color: "#AAFFFFFF"
                    font.pixelSize: 16
                }
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                id: newNoteBtn
                onClicked: nameDialog.open()
                background: Rectangle {
                    implicitWidth: 56
                    implicitHeight: 56
                    color: newNoteBtn.hovered ? root.colorPrimary : "#1AFFFFFF"
                    radius: 28
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 28
                        height: 28
                        source: "qrc:/CNote/Core/icons/add.svg"
                        sourceSize: Qt.size(64, 64)
                        smooth: true
                        opacity: newNoteBtn.hovered ? 1.0 : 0.8
                    }
                }
            }

            Button {
                id: importBtn
                text: "Import PDF"
                onClicked: dashboardRoot.importRequested()
                contentItem: Text {
                    text: importBtn.text
                    color: "white"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 120
                    implicitHeight: 48
                    color: importBtn.hovered ? "#33FFFFFF" : "#1AFFFFFF"
                    radius: 24
                    border.color: "#22FFFFFF"
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
            }
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: dashboardRoot.importedFiles
            cellWidth: 220
            cellHeight: 280
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                width: 200
                height: 260

                Rectangle {
                    id: card
                    anchors.fill: parent
                    anchors.margins: 8
                    color: mouseArea.containsMouse ? "#33FFFFFF" : "#1AFFFFFF"
                    radius: 24
                    border.color: mouseArea.containsMouse ? root.colorPrimary : "#11FFFFFF"
                    border.width: 1

                    // Glass Blur Effect
                    MultiEffect {
                        source: card
                        anchors.fill: card
                        blurEnabled: true
                        blur: 1.0
                        blurMax: 32
                        contrast: 0.05
                        brightness: 0.02
                        z: -1
                    }

                    scale: mouseArea.pressed ? 0.95 : (mouseArea.containsMouse ? 1.05 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                    Behavior on color { ColorAnimation { duration: 200 } }

                    // Entry Animation
                    opacity: 0
                    y: 20
                    Component.onCompleted: {
                        entryAnim.start()
                    }
                    ParallelAnimation {
                        id: entryAnim
                        NumberAnimation { target: card; property: "opacity"; to: 1; duration: 400; easing.type: Easing.OutCubic }
                        NumberAnimation { target: card; property: "y"; to: 0; duration: 400; easing.type: Easing.OutCubic }
                    }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 16

                        // Thumbnail
                        Rectangle {
                            width: parent.width
                            height: 160
                            radius: 16
                            clip: true
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: modelData.endsWith(".note") ? "#381E72" : "#1A1C1E" }
                                GradientStop { position: 1.0; color: modelData.endsWith(".note") ? "#4F378B" : "#0F1113" }
                            }
                            
                            Text {
                                anchors.centerIn: parent
                                text: modelData.endsWith(".note") ? "NOTE" : "PDF"
                                color: "white"
                                opacity: 0.2
                                font.pixelSize: 32
                                font.weight: Font.Black
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
                            font.pixelSize: 15
                            font.weight: Font.Medium
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
