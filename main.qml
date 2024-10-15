import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.3
import QtQuick.Controls 2.5
//import Qt.labs.platform 1.1

import "./qml/"
import IbpicpDataControl 1.0
import CustomPlot 1.0
import FileFuncs 1.0

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
    property bool isSelectLabel: false
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
            labelDc.dayList = dayList;
            dateList = dayList;

            myplot.setCurrentGraphData(timeData, pressureData);
            var id = data["id"];
            if(id.endsWith("P"))
            {
                myplot.setVoltageGraphVisible(false);
            }
            else
            {
                myplot.setVoltageGraphVisible(true);
                myplot.setVoltageGraphData(timeData, temperatureData);
            }
            myplot.rescaleAxes()
            myplot.replot();
        }
    }

    property bool showTempData: false
    property bool showPresData: false
    property double minPres: 0
    property double maxPres: 0
    property double avgPres: 0
    property double minTemp: 0
    property double maxTemp: 0
    property double avgTemp: 0


    function iCalcValue(stime, etime)
    {
        var data = userDataList[cur_user_index];
        var timeData = data["timeData"];
        const firstTime = timeData[0];
        if(stime < firstTime)
        {
            stime = firstTime;
        }
        const sIndex = stime - firstTime;

        const lastTime = timeData[timeData.length - 1];
        if(etime > lastTime)
        {
            etime = lastTime;
        }
        const eIndex = etime - firstTime;

        var pressureData = data["pressureData"];
        var temperatureData = data["temperatureData"];

        var maxPresValue = -999999;
        var minPresValue = 999999;
        var sumPresValue = 0;
        var maxTempValue = -999999;
        var minTempValue = 999999;
        var sumTempValue = 0;

        for(var i = sIndex; i < eIndex; i++)
        {
            var pres = pressureData[i];
            maxPresValue = Math.max(pres, maxPresValue);
            minPresValue = Math.min(pres, minPresValue);
            sumPresValue += pres;

            var temp = temperatureData[i];
            maxTempValue = Math.max(temp, maxTempValue);
            minTempValue = Math.min(temp, minTempValue);
            sumTempValue += temp;
        }
        maxPres = maxPresValue.toFixed(2);
        minPres = minPresValue.toFixed(2);
        avgPres = (sumPresValue / (eIndex - sIndex)).toFixed(2);

        maxTemp = maxTempValue.toFixed(2);
        minTemp = minTempValue.toFixed(2);
        avgTemp = (sumTempValue / (eIndex - sIndex)).toFixed(2);
    }

    function updateTimeRanger()
    {
        var sDate = new Date(stimeText.text);
        const stime = Math.trunc(Date.parse(sDate) / 1000);

        var eDate = new Date(etimeText.text);
        const etime = Math.trunc(Date.parse(eDate) / 1000);

        iCalcValue(stime, etime);
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

    function iGetDate(dayIndex, hour, min, second)
    {
        if(dateList.length - 1 < dayIndex){
            return;
        }
        const day = dateList[dayIndex];
        const hourStr = hour < 10 ? ('0' + hour) : hour;
        const minStr = min < 10 ?  ('0' + min) : min;
        const secondStr = second < 10 ?  ('0' + second) : second;
        const timeStr = day + " " + (hourStr) + ":" + (minStr)+ ":" + secondStr;
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
            iMinWidth = 1920*0.85;
            iMinHeight = 1080*0.85;
            root.showMaximized();
            isMaxShow = true;
        }
    }

    FileDialog {
        id: inputFileDialog
        title: "选择数据文件"
        nameFilters: ["Data files (*.bin *.bson)"]
        selectMultiple: true
        onAccepted: {
            var filePathStr = inputFileDialog.fileUrls.toString();
            ibpicpDataControl.ReadIbpicpDatas(filePathStr.split(","), true);
            cur_user_index = -1;
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

    FileDialog {
        id: outputTextDialog
        title: "导出文本"
        nameFilters: ["files (*.bson)"]
        //fileMode: FileDialog.SaveFile
        selectExisting: false
        onAccepted: {
            var path = outputTextDialog.fileUrl.toString().replace("file:///", "")
            var str = JSON.stringify(userDataList);
            console.log("path", path);
            ff.saveStringToFile(str, path);
        }
    }

    MsgDialog{
        id : msg
        z : 99999
    }

    FileFuncs{
        id : ff
    }

    Popup {
        id: labelSettingPop
        width: 320
        height: 220
        anchors.centerIn: parent
        visible: false
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        function openWithData(time, note, type)
        {
            const date = new Date(time * 1000);
            labelTimeText.text = dateToString(date);
            note_value.text = note;
            openType = type;
            labelSettingPop.open();
            if(type == 0)
            {
                presBox.checked = true;
                presBox.enabled = true;
                tempBox.enabled = true;
            }
            else
            {
                presBox.checked = false;
                presBox.enabled = false;
                tempBox.checked = false;
                tempBox.enabled = false;
            }
        }

        property int openType: 0

        Rectangle{
            id: mainRect
            anchors.fill: parent
            color: "#F9F9F9"
            radius: 5
            border.color: "#33364460"
            border.width: 0.5
            Column{
                width: parent.width - 20
                height: parent.height - 20
                anchors.centerIn : parent
                spacing: 20
                Row{
                    width: parent.width
                    height: 30
                    spacing: 20
                    Text {
                        width: 40
                        height: parent.height
                        text: qsTr("类型: ")
                        font.pixelSize: 14
                        font.family: "微软雅黑"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    ButtonGroup{
                        id:group
                        exclusive: true
                        buttons: [presBox, tempBox]
                    }


                    CheckBox {
                        id: presBox
                        text: qsTr("压强")
                    }

                    CheckBox {
                        id: tempBox
                        text: qsTr("温度")
                    }
                }
                Row{
                    width: parent.width
                    height: 30
                    spacing: 20
                    Text {
                        width: 40
                        height: parent.height
                        text: qsTr("时间: ")
                        font.pixelSize: 14
                        font.family: "微软雅黑"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Rectangle{
                        id: labelTimeRect
                        width: parent.width - 40 - 20
                        height: parent.height
                        border.color: "#B7B7B7"
                        border.width: 1
                        radius: 6
                        Text {
                            id: labelTimeText
                            anchors.fill: parent
                            font.pixelSize: 12
                            font.family: "微软雅黑"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea{
                            anchors.fill: parent
                            onClicked: {
                                if(!isSelectLabel){
                                    labelTimeChoose.open();
                                    var time = labelTimeText.text;
                                    labelDc.iOpen(time)
                                }
                            }
                            Popup {
                                visible: false
                                id: labelTimeChoose
                                x : ((labelTimeRect.width - 200) / 2)
                                y : labelTimeRect.y + labelTimeRect.height
                                width : 200
                                height : 200
                                DateChoose{
                                    id:labelDc
                                    anchors.fill: parent
                                    onIChoose: {
                                        var d = iGetDate(day, hour, min, second);
                                        labelTimeText.text = d;
                                        labelTimeChoose.close();
                                    }
                                }
                            }
                        }
                    }
                }
                Row{
                    width: parent.width
                    height: 30
                    spacing: 20
                    Text {
                        width: 40
                        height: parent.height
                        text: qsTr("备注: ")
                        font.pixelSize: 14
                        font.family: "微软雅黑"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    TextField {
                        id: note_value;
                        width: parent.width - 40 - 20
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter;
                        horizontalAlignment: Text.AlignHCenter;
                        font.pixelSize: 12
                        font.family: "微软雅黑"
                        clip: true;
                        maximumLength: 15
                        background: Rectangle {
                            radius: 6
                            border.color: "#B7B7B7"
                            border.width: 1
                        }
                        onFocusChanged: {
                            if(focus) selectAll();
                        }
                    }
                }
                Row{
                    width: parent.width
                    height: 30
                    Item {
                        width: parent.width / 2
                        height: parent.height
                        Rectangle{
                            width: 90
                            height: parent.height
                            anchors.centerIn: parent
                            color: "#FD8129"
                            radius: 5
                            Text {
                                anchors.fill: parent
                                text: qsTr("确定")
                                font.pixelSize: 12
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
                                    if(!isSelectLabel)
                                    {
                                        if(labelTimeText.text == "" || note_value.text == "")
                                        {
                                            msg.func = function(){

                                            };
                                            msg.isSuccess = false;
                                            msg.tipText = "标签内容未填写,请确认!";
                                            msg.hasCancel = false;
                                            msg.openMsg();
                                            return;
                                        }

                                        var date = new Date(labelTimeText.text)
                                        const curTime = Math.trunc(Date.parse(date) / 1000);

                                        var data = userDataList[cur_user_index];
                                        var timeData = data["timeData"];
                                        const firstTime = timeData[0];
                                        const lastTime = timeData[timeData.length - 1];
                                        if(curTime < firstTime  || curTime > lastTime)
                                        {
                                            msg.func = function(){

                                            };
                                            msg.isSuccess = false;
                                            msg.tipText = "不在有效时间范围内,请确认!";
                                            msg.hasCancel = false;
                                            msg.openMsg();
                                            return;
                                        }
                                        const index = curTime - firstTime;
                                        var yValue = 0;
                                        var type = 0;
                                        if(presBox.checked)
                                        {
                                            var pressureData = data["pressureData"];
                                            yValue = pressureData[index];
                                        }
                                        else
                                        {
                                            type = 1;
                                            var temperatureData = data["temperatureData"];
                                            yValue = temperatureData[index];
                                        }

                                        myplot.addLabel(type, curTime, yValue, note_value.text.trim());
                                    }
                                    else {
                                        myplot.editLabel(note_value.text);
                                    }
                                    labelSettingPop.close();
                                }
                            }
                        }
                    }

                    Item {
                        width: parent.width / 2
                        height: parent.height
                        Rectangle{
                            width: 90
                            height: parent.height
                            color: "#FD8129"
                            anchors.centerIn: parent
                            radius: 5
                            Text {
                                anchors.fill: parent
                                text: qsTr("取消")
                                font.pixelSize: 12
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
                                    labelSettingPop.close();
                                }
                            }
                        }
                    }
                }
            }
        }
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
                            msg.isSuccess = true;
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
                                                        width: parent.width - 60 - parent.spacing
                                                        height: parent.height
                                                        Text {
                                                            width: parent.width
                                                            height: parent.height / 2
                                                            text: "ID: " + modelData.id
                                                            font.pixelSize: 14
                                                            color: cur_user_index === index ? "#000000" : "#999999"
                                                            font.family: "微软雅黑"
                                                            horizontalAlignment: Text.AlignLeft
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                        Text {
                                                            width: parent.width
                                                            height: parent.height / 2
                                                            text: "Item: " + modelData.name
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
                        font.pixelSize: 14
                        color: "#333333"
                        font.family: "微软雅黑"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Rectangle
                    {
                        width : 340
                        height : parent.height
                        border.color: "#B7B7B7"
                        border.width: 1
                        radius: 6
                        Row
                        {
                            width: parent.width - 20
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Item{
                                id: sTimeRect
                                width: (parent.width - 12) / 2
                                height: parent.height
                                Text {
                                    id: stimeText
                                    anchors.fill: parent
                                    font.pixelSize: 12
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
                                        x : sTimeRect.x + ((sTimeRect.width - 200) / 2)
                                        y : sTimeRect.y + sTimeRect.height
                                        width : 200
                                        height : 200
                                        DateChoose{
                                            id:dc
                                            anchors.fill: parent
                                            onIChoose: {
                                                var d = iGetDate(day, hour, min, second);
                                                if(d > etimeText.text)
                                                {
                                                    stimeText.text = etimeText.text;
                                                    etimeText.text = d;
                                                }
                                                else{
                                                    stimeText.text = d;
                                                }
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
                                    font.pixelSize: 12
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
                                        x : eTimeRect.x + ((eTimeRect.width - 200) / 2)
                                        y : eTimeRect.y + eTimeRect.height
                                        width : 200
                                        height : 200
                                        DateChoose{
                                            id:endDc
                                            anchors.fill: parent
                                            onIChoose: {
                                                var d = iGetDate(day, hour, min, second);
                                                if(d < stimeText.text)
                                                {
                                                    etimeText.text = stimeText.text;
                                                    stimeText.text = d;
                                                }
                                                else{
                                                    etimeText.text = d;
                                                }
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
                        width : 100
                        height : parent.height
                        border.color: "#B7B7B7"
                        border.width: 1
                        radius: 6
                        Row
                        {
                            anchors.fill: parent

                            Text {
                                id: dataTytpeText
                                width: 60
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
                        width: 10
                        height: parent.height
                    }
                    Item
                    {
                        width: 100
                        height: parent.height
                        Row
                        {
                            anchors.fill: parent
                            Text {
                                width: 60
                                height: parent.height
                                text: qsTr("最大值")
                                color: "#0A6086"
                                font.pixelSize: 14
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                id: maxText
                                width: 40
                                height: parent.height
                                text: showTempData ? maxTemp : maxPres
                                color: "#0A6086"
                                font.pixelSize: 16
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
                        width: 100
                        height: parent.height
                        Row
                        {
                            anchors.fill: parent
                            Text {
                                id: minText
                                width: 60
                                height: parent.height
                                text: qsTr("最小值")
                                color: "#BA6C00"
                                font.pixelSize: 14
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                width: 40
                                height: parent.height
                                text: showTempData ? minTemp : minPres
                                color: "#BA6C00"
                                font.pixelSize: 16
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
                        width: 100
                        height: parent.height
                        Row
                        {
                            anchors.fill: parent
                            Text {
                                width: 60
                                height: parent.height
                                text: qsTr("平均值")
                                color: "#127070"
                                font.pixelSize: 14
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                id: averageText
                                width: 40
                                height: parent.height
                                text: showTempData ? avgTemp : avgPres
                                color: "#127070"
                                font.pixelSize: 16
                                font.family: "微软雅黑"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Item{
                        width: parent.width - 80 - 340 - 30 - 100 - 10 - 3*100 - 2*20 - 90 - 15*4
                               - (cur_user_index > -1 ? (90 * 4) : 0)
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 90
                        height : parent.height
                        border.color: "#FD9433"
                        border.width: 1
                        radius: 6
                        visible: cur_user_index > -1
                        Row
                        {
                            width: parent.width - 20
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Image
                            {
                                width: height
                                height: parent.height
                                fillMode: Image.PreserveAspectFit;
                                source: isSelectLabel ? "qrc:/Res/edit.png" : "qrc:/Res/add.png"
                            }
                            Text {
                                width: parent.width - parent.height
                                height: parent.height
                                font.pixelSize: 12
                                text: isSelectLabel ? "编辑标签" : "添加标签"
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
                            onEntered: parent.color = "#FD9433";
                            onExited: parent.color = "#FFFFFF";
                            onClicked:
                            {
                                var time, note, type
                                var data = userDataList[cur_user_index];
                                var timeData = data["timeData"];
                                if(isSelectLabel)
                                {
                                    time = myplot.getLabelTime();
                                    note = myplot.getLabelText();
                                    type = 1;
                                }
                                else
                                {
                                    time = timeData[0];
                                    note = ""
                                    type = 0;
                                }
                                labelSettingPop.openWithData(time, note, type);
                            }
                        }
                    }
                    Item
                    {
                        width: 15
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 90
                        height : parent.height
                        border.color: "#FD9433"
                        border.width: 1
                        radius: 6
                        visible: cur_user_index > -1
                        Row
                        {
                            width: parent.width - 20
                            height: parent.height - 24
                            anchors.centerIn: parent
                            Image
                            {
                                width: height
                                height: parent.height
                                fillMode: Image.PreserveAspectFit;
                                source: isSelectLabel ? "qrc:/Res/delete.png" : "qrc:/Res/clear.png"
                            }
                            Text {
                                width: parent.width - parent.height
                                height: parent.height
                                font.pixelSize: 12
                                text: isSelectLabel ? "删除标签" : "清空标签"
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
                            onEntered: parent.color = "#FD9433";
                            onExited: parent.color = "#FFFFFF";
                            onClicked:
                            {

                                msg.func = function(){
                                    if(isSelectLabel)
                                    {
                                        myplot.deleteCurLabel();
                                    }
                                    else
                                    {
                                        myplot.clearLabels();
                                    }
                                };
                                msg.isSuccess = true;
                                msg.tipText = isSelectLabel ? "是否删除此标签, 请确认!" : "是否清除所有标签, 请确认!";
                                msg.hasCancel = true;
                                msg.openMsg();
                            }
                        }
                    }
                    Item
                    {
                        width: 15
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 90
                        height : parent.height
                        border.color: "#FD9433"
                        border.width: 1
                        radius: 6
                        visible: cur_user_index > -1
                        Row
                        {
                            width: parent.width - 20
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
                                font.pixelSize: 12
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
                            onEntered: parent.color = "#FD9433";
                            onExited: parent.color = "#FFFFFF";
                            onClicked:
                            {
                                outputFileDialog.open();
                            }
                        }

                    }
                    Item
                    {
                        width: 15
                        height: parent.height
                    }

                    Rectangle
                    {
                        width : 90
                        height : parent.height
                        border.color: "#FD9433"
                        border.width: 1
                        radius: 6
                        visible: cur_user_index > -1
                        Row
                        {
                            width: parent.width - 20
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
                                font.pixelSize: 12
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
                            onEntered: parent.color = "#FD9433";
                            onExited: parent.color = "#FFFFFF";
                            onClicked:
                            {
                                outputTextDialog.open();
                            }
                        }
                    }
                    Item
                    {
                        width: 15
                        height: parent.height
                    }
                    Rectangle
                    {
                        width : 90
                        height : parent.height
                        color: "#FD9433"
                        radius: 6
                        Row
                        {
                            width: parent.width - 20
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
                                font.pixelSize: 12
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
                                parent.border.color = "#FD9433";
                                parent.color = "#FFFFFF";
                            }
                            onExited: parent.color = "#FD9433";
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
                        onSelectedTextChanged:
                        {
                            console.log("plotItem ", value);
                            isSelectLabel = value;
                        }
                    }
                }

                // Data End
            }
        }
        // Window End
    }
}


