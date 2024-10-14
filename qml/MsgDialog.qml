import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Item {
    id: root
    anchors.centerIn: parent
    property alias tipText: msg.text
    property bool isSuccess: false

    //可以自行扩展一些属性
    property string backGroundColor: "white"
    property Item parentItem : Rectangle {}

    //Dialog header、contentItem、footer之间的间隔默认是12
    // 提示框的最小宽度是 100
    property int minWidth: 250

    property var func: undefined // 回调函数
    property bool hasCancel: false

    function iResetParams(){
        func = undefined
        hasCancel = false;
    }

    width: {
        if(msg.implicitWidth < minWidth || msg.implicitWidth == minWidth)
            return minWidth;
        else
            return msg.implicitWidth > 300 ? 300 + 24 : (msg.implicitWidth + 24);
    }
    height: msg.implicitHeight + 24 + 100

    Dialog {
        id: dialog
        width: root.width
        height: root.height
        modal: true
        background: Rectangle {
            color: backGroundColor
            anchors.fill: parent
            radius: 5
        }
        header: Rectangle {
            width: dialog.width
            height: 50
            border.color: backGroundColor
            radius: 5
            Image {
                width: 40
                height: 40
                anchors.centerIn: parent
                source: isSuccess ? "qrc:/Res/success.png" : "qrc:/Res/note.png"
            }
        }
        contentItem: Rectangle {
            border.color: backGroundColor
            color: backGroundColor
            Text {
                anchors.fill: parent
                anchors.centerIn: parent
                font.family: "Microsoft Yahei"
                color: "gray"
                text: tipText
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter

            }
        }
        footer: Rectangle {
            width: msg.width
            height: 50
            border.color: backGroundColor
            color: backGroundColor
            radius: 5
            Row {
                width: implicitWidth
                height: parent.height
                anchors.centerIn: parent
                Item {
                    width: msg.width / 2 - 20
                    height: parent.height
                    Button {
                        anchors.centerIn: parent
                        width: 80
                        height: 30
                        background: Rectangle {
                            anchors.centerIn: parent
                            width: 80
                            height: 30
                            radius: 5
                            border.color: "#FD8129"
                            border.width: 1
                            color: backGroundColor
                            Text {
                                anchors.centerIn: parent
                                font.family: "Microsoft Yahei"
                                font.bold: true
                                color: "#FD8129"
                                text: "确定"
                            }
                        }
                        onClicked: {
                            if(func)  func();
                            iResetParams();
                            dialog.close();
                        }
                    }
                }
                Item {
                    width: msg.width / 2 - 20
                    height: parent.height
                    visible: hasCancel
                    Button {
                        anchors.centerIn: parent
                        width: 80
                        height: 30
                        background: Rectangle {
                            anchors.centerIn: parent
                            width: 80
                            height: 30
                            radius: 5
                            border.color: "#FD8129"
                            border.width: 1
                            color: backGroundColor
                            Text {
                                anchors.centerIn: parent
                                font.family: "Microsoft Yahei"
                                font.bold: true
                                color: "#FD8129"
                                text: "取消"
                            }
                        }
                        onClicked: {
                            iResetParams();
                            dialog.close();
                        }
                    }
                }
            }
        }
    }

    //利用Text 的implicitWidth属性来调节提示框的大小
    //该Text的字体格式需要与contentItem中的字体一模一样
    Text {
        id: msg
        visible: false
        width: 300
        wrapMode: Text.WordWrap
        font.family: "Microsoft Yahei"
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    function openMsg() {
        root.x = (parent.width - dialog.width) * 0.5
        root.y = (parent.height - dialog.height) * 0.5
        dialog.open();
    }
}


