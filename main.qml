import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.3
import QtQuick.Controls 2.5
//import Qt.labs.platform 1.1

import "./qml/"
import IbpicpDataControl 1.0
import CustomPlot 1.0


ApplicationWindow  {
    id: root
    visible: true
    width: iMinWidth;
    height: iMinHeight;
    flags: Qt.Window | Qt.FramelessWindowHint

    property int iMinWidth:1920
    property int iMinHeight:1080

    property bool isMaxShow: false
    property bool isMoveWindow: false
    property point rootTitleMousePos: Qt.point(x,y)
    property string bgColor: "#FD8129"
    property var dateList: []
    property var m_timeData: []

    property var userDataList
    property int cur_user_index: -1
    onCur_user_indexChanged:
    {
        if(userDataList && cur_user_index > -1 && userDataList.length > cur_user_index)
        {
            var data = userDataList[cur_user_index];
            var timeData = data["timeData"];
            m_timeData = timeData;
            var pressureData = data["pressureData"];
            var temperatureData = data["temperatureData"];
            const startDate = new Date(timeData[0] * 1000);
            const endDate = new Date(timeData[timeData.length - 1] * 1000);
            const dayList = getAllDay(startDate, endDate);

            stimeText.text = dateToString(startDate);
            etimeText.text = dateToString(endDate);
            dc.dayList = dayList;
            endDc.dayList = dayList;
            dateList = dayList;

            myplot.setCurrentGraphData(timeData, pressureData);
            myplot.setVoltageGraphData(timeData, temperatureData);
            myplot.rescaleAxes()
            myplot.replot();
        }
    }

    property bool showTempData: false
    property bool showPresData: false

    function updateTimeRanger()
    {
        var sDate = new Date(stimeText.text);
        const stime = Math.trunc(Date.parse(sDate) / 1000);

        var eDate = new Date(etimeText.text);
        const etime = Math.trunc(Date.parse(eDate) / 1000);

        myplot.setTimeRange(stime, etime);
        myplot.replot();
    }

    function dateToString(date)
    {
        var year = date.getFullYear().toString().padStart(4, "0");
        const month = (date.getMonth() + 1).toString().padStart(2, "0");
        const day = date.getDate().toString().padStart(2, "0");
        const hour = date.getHours().toString().padStart(2, "0");
        const min = date.getMinutes().toString().padStart(2, "0");
        const sec = date.getSeconds().toString().padStart(2, "0");
        return year + "-" + month + "-" + day + " " + (hour) + ":" + (min)+ ":" + (sec);
    }

    function getAllDay(start, end) {
        let dateList = [];
        var startTime = getDate(start);
        var endTime = getDate(end);

        while ((endTime.getTime() - startTime.getTime()) >= 0) {
            var year = startTime.getFullYear();
            var month = startTime.getMonth() < 10 ? '0' + (startTime.getMonth()) : startTime.getMonth();
            var day = startTime.getDate().toString().length === 1 ? "0" + startTime.getDate() : startTime.getDate();
            dateList.push(year + "-" + month + "-" + day);
            startTime.setDate(startTime.getDate() + 1);
        }
        return dateList;
    }


    function getDate(time) {
        const year = time.getFullYear().toString().padStart(4, "0");
        const month = (time.getMonth() + 1).toString().padStart(2, "0");
        const day = time.getDate().toString().padStart(2, "0");
        var date = new Date(year, month, day);
        return date;
    }

    function iGetDate(dayIndex, hour, min)
    {
        if(dateList.length - 1 < dayIndex){
            return;
        }
        const day = dateList[dayIndex];
        const hourStr = hour < 10 ? ('0' + hour) : hour;
        const minStr = min < 10 ?  ('0' + min) : min;
        const timeStr = day + " " + (hourStr) + ":" + (minStr)+ ":" + "00";
        return timeStr;
    }

    Component.onCompleted:
    {
        userInfoRep.model = [
                    {
                        "id" : "",
                        "name" : "",
                        "age" : 0,
                        "isMan" : true,
                    }
                ];


        // 获取屏幕分辨率
        var sw =  Screen.desktopAvailableWidth
        var sh =  Screen.desktopAvailableHeight

        if(sw < root.width || sh < root.height){
            iMinWidth = sw*0.66;
            iMinHeight = sh*0.66;
            root.showMaximized();
            isMaxShow = true;
        }
        console.log("aq" ,FileDialog.OpenFile,  outputFileDialog.fileMode == FileDialog.OpenFile)
    }

    FileDialog {
        id: inputFileDialog
        title: "选择数据文件"
        nameFilters: ["files (*.bin)"]
        onAccepted: {
            var path = inputFileDialog.fileUrl.toString().replace("file:///", "");
            ibpicpDataControl.ReadIbpicpData(path, true);
        }
    }

    FileDialog {
        id: outputFileDialog
        title: "导出PDF"
        nameFilters: ["files (*.pdf)"]
        //fileMode: FileDialog.SaveFile
        selectExisting: false
        onAccepted: {
            var path = outputFileDialog.fileUrl.toString().replace("file:///", "")
            myplot.exportPdf(path);
        }
    }

    MsgDialog{
        id : msg
        z : 99999
    }

    IbpicpDataControl
    {
        id: ibpicpDataControl
        onUserDataChanged:
        {
            userInfoRep.model = [];
            userDataList = ibpicpDataControl.userData;
            userInfoRep.model = userDataList;
        }
    }


    Column{
        anchors.fill: parent
        // Title
        Rectangle{
            id : titleRect
            width: parent.width
            height: 43
            gradient: Gradient {
                orientation:Gradient.Horizontal    //该属性设置渐变的方向
                GradientStop { position: 0.0; color: "#FD9433" }
                GradientStop { position: 1.0; color: "#FD8129" }
            }
            Text {
                anchors.fill: parent
                anchors.leftMargin: 30
                text: "颅内压监护仪监护数据查看软件"
                font.pixelSize:16
                color: "#E2FFFF"
                font.family: "微软雅黑"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
            Row {
                anchors.fill: parent
                layoutDirection: Qt.RightToLeft
                Rectangle{
                    width: 40
                    height: parent.height
                    color: bgColor
                    Image {
                        width: 15
                        height: width
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit;
                        source: "qrc:/Res/quit.png"
                    }
                    MouseArea{
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        onEntered:  parent.color = "#FF0000"
                        onExited: parent.color = bgColor
                        onClicked: {
                            msg.func = function(){
                                root.close()
                            };
                            msg.isSuccess = false;
                            msg.tipText = "是否退出程序,请确认!";
                            msg.hasCancel = true;
                            msg.openMsg();
                        }
                    }
                }

                Rectangle{
                    color: bgColor
                    width: 40
                    height: parent.height
                    Image {
                        width: 15
                        height: width
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit;
                        source: isMaxShow? "qrc:/Res/normal.png":"qrc:/Res/max.png"
                    }
                    MouseArea{
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        onEntered:  parent.color = "#FD9433"
                        onExited: parent.color = bgColor
                        onClicked: {
                            if(isMaxShow)   {
                                root.width = iMinWidth;
                                root.height = iMinHeight;
                                root.showNormal();
                            }
                            else    {
                                root.showMaximized()
                            }
                            isMaxShow = !isMaxShow
                        }
                    }
                }

                Rectangle{
                    width: 40
                    height: parent.height
                    color: bgColor

                    Image {
                        width: 15
                        height: width
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit;
                        source: "qrc:/Res/hide.png"
                    }
                    MouseArea{
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        onEntered:  parent.color = "#FD9433"
                        onExited: parent.color = bgColor
                        onClicked: {
                            root.showMinimized();
                        }
                    }
                }
                Item {
                    width: parent.width - 40*3
                    height: parent.height
                    MouseArea{
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onPressed: {
                            root.rootTitleMousePos = Qt.point(mouseX,mouseY)
                            root.isMoveWindow = true
                        }
                        onReleased: {
                            if(mouse.button === Qt.LeftButton){
                                root.isMoveWindow = false
                            }
                        }
                        onMouseXChanged: {
                            if(root.isMoveWindow){
                                root.x += mouseX - root.rootTitleMousePos.x;
                            }
                        }
                        onMouseYChanged: {
                            root.y += mouseY - root.rootTitleMousePos.y;
                        }
                    }
                }
            }
        }
        // Title

        // Window
        Rectangle
        {
            width: parent.width
            height: parent.height - titleRect.height
            color: "#F9F8F8"
            Column{
                width: parent.width - 30
                height: parent.height - 30
                anchors.centerIn: parent
                // User
                Item {
                    width: parent.width
                    height: 66
                    Text {
                        id: name
                        text: qsTr("数据列表")
                        anchors.fill: parent
                        font.pixelSize:16
                        color: "#333333"
                        font.family: "微软雅黑"
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Item{
                    width: parent.width
                    height: 100
                    Flickable
                    {
                        width: parent.width
                        height: parent.height
                        contentHeight: parent.height
                        contentWidth: userRow.implicitWidth
                        clip: true
                        Row{
                            id : userRow
                            anchors.fill: parent
                            spacing: 20
                            Repeater{
                                id: userInfoRep
                                //model: userInfoList
                                delegate: Component{
                                    Item{
                                        width: 312
                                        height: parent.height
                                        Rectangle{
                                            width: parent.width - 1
                                            height: parent.height - 1
                                            anchors.centerIn: parent
                                            border.color: cur_user_index === index ? "#FD8129" : "#B1B1B1"
                                            border.width: 1
                                            radius: 14
                                            color: "#FFFFFF"
                                            Item
                                            {
                                                width: parent.width - 40
                                                height: 60
                                                anchors.centerIn: parent
                                                Row
                                                {
                                                    anchors.fill: parent
                                                    spacing: 30
                                                    Rectangle
                                                    {
                                                        width: 60
                                                        height: parent.height
                                                        radius: width / 2
                                                        color: cur_user_index === index ? "#FD8129" : "#B1B1B1"
                                                        Text {
                                                            anchors.fill: parent
                                                            text: "数据" + (index + 1)
                                                            font.pixelSize: 14
                                                            color: "#FFFFFF"
                                                            font.family: "微软雅黑"
                                                            horizontalAlignment: Text.AlignHCenter
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                    }

                                                    Column{
                                                        width: parent.width - 60 - 60 - parent.spacing * 2
                                                        height: parent.height
                                                        Text {
                                                            width: parent.width
                                                            height: parent.height / 2
                                                            text: "姓名: " + modelData.name
                                                            font.pixelSize: 14
                                                            color: cur_user_index === index ? "#000000" : "#999999"
                                                            font.family: "微软雅黑"
                                                            horizontalAlignment: Text.AlignLeft
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                        Text {
                                                            width: parent.width
                                                            height: parent.height / 2
                                                            text: "ID: " + modelData.id
                                                            font.pixelSize: 12
                                                            color: cur_user_index === index ? "#000000" : "#999999"
                                                            font.family: "微软雅黑"
                                                            horizontalAlignment: Text.AlignLeft
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                    }

                                                    Column{
                                                        width: 60
                                                        height: parent.height
                                                        Text {
                                                            width: parent.width
                                                            height: parent.height / 2
                                                            text: "性别: " + (modelData.isMan ? "男" : "女")
                                                            font.pixelSize: 14
                                                            color: cur_user_index === index ? "#000000" : "#999999"
                                                            font.family: "微软雅黑"
                                                            horizontalAlignment: Text.AlignLeft
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                        Text {
                                                            width: parent.width
                                                            height: parent.height / 2
                                                            text: "年龄: " + modelData.age
                                                            font.pixelSize: 14
                                                            color: cur_user_index === index ? "#000000" : "#999999"
                                                            font.family: "微软雅黑"
                                                            horizontalAlignment: Text.AlignLeft
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                    }
                                                }
                                            }

                                            MouseArea{
                                                anchors.fill: parent
                                                onClicked: {
                                                    if(cur_user_index == index || modelData.id == "")
                                                    {
                                                        return;
                                                    }
                                                    cur_user_index = index;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // User End
                // Spacing
                Item {
                    width: parent.width
                    height: 60
                    Rectangle
                    {
                        width: parent.width
                        height: 2
                        anchors.centerIn: parent
                        color: bgColor
                    }
                }
                // Spacing End
                // Info
                Row
                {
                    width: parent.width
                    height: 40
                    Text {
                        width: 80
                        height: parent.height
                        text: "时间选择"
                        font.pixelSize: 16
                        color: "#333333"
                        font.family: "微软雅黑"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Rectangle
                    {
                        width : 476
                        height : parent.height
                        border.color: "#B7B7B7"
                        border.width: 1
                        radius: 6
                        Row
                        {
                            width: parent.width - 60
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Item{
                                id: sTimeRect
                                width: (parent.width - 12) / 2
                                height: parent.height
                                Text {
                                    id: stimeText
                                    anchors.fill: parent
                                    font.pixelSize: 14
                                    font.family: "微软雅黑"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    onTextChanged:
                                    {
                                        updateTimeRanger();
                                    }
                                }
                                MouseArea{
                                    anchors.fill: parent
                                    onClicked: {
                                        dateChoose.open();
                                        var stime = stimeText.text;
                                        dc.iOpen(stime)
                                    }
                                    Popup {
                                        visible: false
                                        id: dateChoose
                                        x : sTimeRect.x
                                        y : sTimeRect.y + sTimeRect.height
                                        width : sTimeRect.width
                                        height : stimeText.width
                                        DateChoose{
                                            id:dc
                                            anchors.fill: parent
                                            onIChoose: {
                                                var d = iGetDate(day, hour, min);
                                                stimeText.text = d;
                                                dateChoose.close();
                                            }
                                        }
                                    }
                                }
                            }
                            Item {
                                width: 12
                                height: parent.height
                                Image
                                {
                                    width: parent.width
                                    height: width
                                    anchors.centerIn: parent
                                    fillMode: Image.PreserveAspectFit;
                                    source: "qrc:/Res/dateMid.png"
                                }
                            }

                            Item{
                                id: eTimeRect
                                width: (parent.width - 12) / 2
                                height: parent.height
                                Text {
                                    id: etimeText
                                    anchors.fill: parent
                                    font.pixelSize: 14
                                    font.family: "微软雅黑"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    onTextChanged:
                                    {
                                        updateTimeRanger();
                                    }
                                }
                                MouseArea{
                                    anchors.fill: parent
                                    onClicked: {
                                        endDateChoose.open();
                                        var stime = etimeText.text;
                                        endDc.iOpen(stime)
                                    }
                                    Popup {
                                        visible: false
                                        id: endDateChoose
                                        x : eTimeRect.x - eTimeRect.width
                                        y : eTimeRect.y + eTimeRect.height
                                        width : eTimeRect.width
                                        height : etimeText.width
                                        DateChoose{
                                            id:endDc
                                            anchors.fill: parent
                                            onIChoose: {
                                                var d = iGetDate(day, hour, min)
                                                etimeText.text = d;
                                                endDateChoose.close();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Item
                    {
                        width: 30
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 120
                        height : parent.height
                        border.color: "#B7B7B7"
                        border.width: 1
                        radius: 6
                        Row
                        {
                            anchors.fill: parent

                            Text {
                                id: dataTytpeText
                                width: 80
                                height: parent.height
                                text: showTempData ? "温度": "压强"
                                font.pixelSize: 14
                                color: "#2C7F75"
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Item {
                                width: 40
                                height: parent.height
                                Image
                                {
                                    width: 20
                                    height: width
                                    anchors.centerIn: parent
                                    fillMode: Image.PreserveAspectFit;
                                    source: "qrc:/Res/dateMid.png"
                                }
                                MouseArea
                                {
                                    anchors.fill: parent
                                    onClicked:
                                    {
                                        showTempData = !showTempData;
                                    }
                                }
                            }
                        }
                    }
                    Item
                    {
                        width: 30
                        height: parent.height
                    }
                    Item
                    {
                        width: 120
                        height: parent.height
                        Row
                        {
                            anchors.fill: parent
                            Text {
                                width: 80
                                height: parent.height
                                text: qsTr("最大值")
                                color: "#0A6086"
                                font.pixelSize: 16
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                width: 40
                                height: parent.height
                                text: "108"
                                color: "#0A6086"
                                font.pixelSize: 20
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Item
                    {
                        width: 20
                        height: parent.height
                        Rectangle
                        {
                            width: 2
                            height: parent.height - 10
                            color: "#E7E7E7"
                            anchors.centerIn: parent
                        }
                    }
                    Item
                    {
                        width: 120
                        height: parent.height
                        Row
                        {
                            anchors.fill: parent
                            Text {
                                width: 80
                                height: parent.height
                                text: qsTr("最小值")
                                color: "#BA6C00"
                                font.pixelSize: 16
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                width: 40
                                height: parent.height
                                text: "108"
                                color: "#BA6C00"
                                font.pixelSize: 20
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Item
                    {
                        width: 20
                        height: parent.height
                        Rectangle
                        {
                            width: 2
                            height: parent.height - 10
                            color: "#E7E7E7"
                            anchors.centerIn: parent
                        }
                    }
                    Item
                    {
                        width: 120
                        height: parent.height
                        Row
                        {
                            anchors.fill: parent
                            Text {
                                width: 80
                                height: parent.height
                                text: qsTr("平均值")
                                color: "#127070"
                                font.pixelSize: 16
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                width: 40
                                height: parent.height
                                text: "108"
                                color: "#127070"
                                font.pixelSize: 20
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Item{
                        width: parent.width - 110*3 - 30*4 - 120*4 - 20*2 - 476 - 80
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 110
                        height : parent.height
                        border.color: "#2C7F75"
                        border.width: 1
                        radius: 6
                        Row
                        {
                            width: parent.width - 28
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Image
                            {
                                width: height
                                height: parent.height
                                fillMode: Image.PreserveAspectFit;
                                source: "qrc:/Res/exportPDF.png"
                            }
                            Text {
                                width: parent.width - parent.height
                                height: parent.height
                                font.pixelSize: 14
                                text: "导出PDF"
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        MouseArea
                        {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            hoverEnabled: true
                            onEntered: parent.color = "#2C7F75";
                            onExited: parent.color = "#FFFFFF";
                            onClicked:
                            {
                                outputFileDialog.open();
                            }
                        }

                    }
                    Item
                    {
                        width: 30
                        height: parent.height
                    }

                    Rectangle
                    {
                        width : 110
                        height : parent.height
                        border.color: "#2C7F75"
                        border.width: 1
                        radius: 6
                        Row
                        {
                            width: parent.width - 28
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Image
                            {
                                width: height
                                height: parent.height
                                fillMode: Image.PreserveAspectFit;
                                source: "qrc:/Res/export.png"
                            }
                            Text {
                                width: parent.width - parent.height
                                height: parent.height
                                font.pixelSize: 14
                                text: "导出文本"
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        MouseArea
                        {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            hoverEnabled: true
                            onEntered: parent.color = "#2C7F75";
                            onExited: parent.color = "#FFFFFF";
                            onClicked:
                            {
                                myplot.replot();
                            }
                        }
                    }
                    Item
                    {
                        width: 30
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 110
                        height : parent.height
                        color: "#2C7F75"
                        radius: 6
                        Row
                        {
                            width: parent.width - 28
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Image
                            {
                                width: height
                                height: parent.height
                                fillMode: Image.PreserveAspectFit;
                                source: "qrc:/Res/openFile.png"
                            }
                            Text {
                                width: parent.width - parent.height
                                height: parent.height
                                font.pixelSize: 14
                                text: "选择文件"
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        MouseArea
                        {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            hoverEnabled: true
                            onEntered: {
                                parent.border.color = "#2C7F75";
                                parent.color = "#FFFFFF";
                            }
                            onExited: parent.color = "#2C7F75";
                            onClicked:
                            {
                                inputFileDialog.open();
                            }
                        }
                    }
                }
                // Info End

                // Data
                Item{
                    width: parent.width
                    height: parent.height - 66 - 100 - 60 - 40
                    CustomPlotItem {
                        id: myplot
                        width: parent.width - 10
                        height: parent.height - 10
                        anchors.centerIn: parent
                        Component.onCompleted: initCustomPlot()
                    }
                }

                // Data End
            }
        }
        // Window End
    }
}


