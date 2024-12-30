import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.0

Rectangle {
    id: root
    border.width: 1
    border.color: "#F5DDD0"
    color: "#f6f6f6"
    property alias dayList : dayTumbler.model

    signal iChoose(var day, var hour, var min, var second);

    function formatText(count, modelData) {
        var data = count === 12 ? modelData + 1 : modelData;
        return data.toString().length < 2 ? "0" + data : data;
    }
    function iOpen(date){
        if(date === "") return;
        var a1 = date.split(" ")[0];
        var a2 = date.split(" ")[1];
        var year = a1.split("-")[0];
        var mon = a1.split("-")[1];
        var day = a1.split("-")[2];

        var hour = a2.split(":")[0];
        var min  = a2.split(":")[1];
        var sec  = a2.split(":")[2];
        console.log("data = ", date, hour, min, sec);
        for(var i = 0; i < dayList.length; i++)
        {
            if(a1 == dayList[i])
            {
                dayTumbler.currentIndex = i;
                break;
            }
        }

        hoursTumbler.currentIndex = hour
        minutesTumbler.currentIndex = min
        secondsTumbler.currentIndex = sec;
    }

    FontMetrics {
        id: fontMetrics
    }

    Component {
        id: delegateComponent

        Label {
            text: formatText(Tumbler.tumbler.count, modelData)
            opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 14
            font.family: "微软雅黑"
//            color: "#FFFFFF"

        }
    }
    Column {
        width: parent.width - 5
        height: parent.height - 5
        anchors.centerIn: parent
        spacing: 10
        Item {
            id: frame
            width: parent.width
            height: parent.height - 30 - 10
            Rectangle {
                width: parent.width
                height: 30
                anchors.centerIn:parent
                color: "#F5DDD0"
            }

            Row {
                id: row
                anchors.fill: parent
                Tumbler {
                    id: dayTumbler
                    width: parent.width / 2
                    height: parent.height
                    wrap: false
                    delegate: delegateComponent
                }

                Tumbler {
                    id: hoursTumbler
                    width: parent.width / 6
                    height: parent.height
                    model: 24
                    delegate: delegateComponent
                }

                Tumbler {
                    id: minutesTumbler
                    width: parent.width / 6
                    height: parent.height
                    model: 60
                    delegate: delegateComponent
                }
                Tumbler {
                    id: secondsTumbler
                    width: parent.width / 6
                    height: parent.height
                    model: 60
                    delegate: delegateComponent
                }

            }
        }
        Item {
            width: parent.width
            height: 30
            Rectangle{
                width: 60
                height: parent.height
                color: "#FD8129"
                radius: 5
                anchors.centerIn: parent
                Text {
                    anchors.fill: parent
                    text: qsTr("确定")
                    font.pixelSize: 14
                    font.bold: true
                    font.family: "微软雅黑"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#FFFFFF"
                }
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered:  parent.color = "#FD9433"
                    onExited: parent.color = "#FD8129"
                    onClicked: {

                        iChoose(dayTumbler.currentIndex,
                                hoursTumbler.currentIndex,
                                minutesTumbler.currentIndex,
                                secondsTumbler.currentIndex)
                    }
                }
            }
        }
    }
}
