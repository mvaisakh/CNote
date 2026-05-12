import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: dashboardRoot
    color: "#121212"

    property var fileManager
    signal openPdf(string path)
    signal importRequested()

    property var importedFiles: []

    Component.onCompleted: refresh()

    function refresh() {
        importedFiles = fileManager.getImportedFiles()
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
                text: "+ Import PDF"
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

                        // Placeholder for Thumbnail
                        Rectangle {
                            width: parent.width
                            height: 150
                            color: "#2a2a2a"
                            radius: 8
                            Text {
                                anchors.centerIn: parent
                                text: "PDF"
                                color: "#444"
                                font.pixelSize: 24
                                font.bold: true
                            }
                        }

                        Text {
                            width: parent.width
                            text: modelData.split('/').pop().split('_').slice(1).join('_')
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
