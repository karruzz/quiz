import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

// main window
Window {	
	visible: true
	width: 700
	height: 500
	MouseArea {
		anchors.fill: parent
//		onClicked: {
//			Qt.quit();
	//	}
	}


//	GridLayout {
//		id: grid
//		anchors.fill: parent
//		rows: 3

	ColumnLayout {
		anchors.fill: parent
//		spacing: 2

		Rectangle
		{
			Layout.fillWidth: true
			height: 60

			color: "bisque"

				Text {
					text: "Toolbar"
					anchors.centerIn: parent
				}

//			ToolBar
//			{
//				id: buttonPanel
//				width: parent.width
//				anchors.top: parent.top
//				height: 40
//			}
		}

		Rectangle {
			id: centerItem3
			Layout.minimumHeight: 20
			Layout.fillWidth: true
			color: "lightyellow"
			Text {
				text: "Statistics"
				anchors.centerIn: parent
			}
		}

		SplitView {
			orientation: Qt.Horizontal
			Layout.fillHeight: true
			Layout.fillWidth: true

			SplitView {
				orientation: Qt.Vertical
				width: 400
				Layout.fillWidth: true

				Rectangle {
					Layout.minimumHeight: 100
					color: "lightblue"

					GroupBox {
						anchors.fill: parent
						title: qsTr("Question")

						TextArea {
							id: textAreaQuestion
							anchors.fill: parent
							text: "Question example"
						}
					}
				}
				Rectangle {
					id: centerItem
					Layout.minimumHeight: 100
					color: "lightgray"

					GroupBox {
						anchors.fill: parent
						title: qsTr("Answer")

						TextArea {
							id: textAreaAnswer
							anchors.fill: parent
							text: "Answer example"
						}
					}
				}
				Rectangle {
					id: centerItem2
					Layout.minimumHeight: 100
					color: "lavender"

					GroupBox {
						anchors.fill: parent
						title: qsTr("Solution")

						TextArea {
							id: textAreaSolution
							anchors.fill: parent
							text: "Solution example"
						}
					}
				}
			}

			Rectangle {
				width: 200
				Layout.fillWidth: true
				color: "lightgreen"

				GroupBox {
					anchors.fill: parent
					title: qsTr("Problems")

					TableView {
						anchors.fill: parent

						TableViewColumn {
							title: "Questions"
							width: 200
						}
						TableViewColumn {
							title: "Errors"
							width: 50
						}
						TableViewColumn {
							title: "Topics"
							width: 50
						}
					}
				}
			}

			Rectangle {
				width: 100
				color: "lightcyan"

				GroupBox {
					anchors.fill: parent
					title: qsTr("Topics")

					Column {
						CheckBox {
							text: qsTr("Breakfast")
							checked: true
						}
						CheckBox {
							text: qsTr("Lunch")
						}
						CheckBox {
							text: qsTr("Dinner")
							checked: true
						}
					}
				}
			}
		}
	}
}


