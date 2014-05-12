import QtQuick 2.1
import Deepin.Widgets 1.0

BaseEditLine {
    id: root
    
    rightLoader.sourceComponent: DTextInput{
        width: valueWidth
        Connections {
            target: root
            onWidgetShown: {
                text = root.value
            }
            onValueChanged: {
                text = root.value
            }
        }
        onTextChanged: {
            root.value = text
            setKey()
        }
    }
}

