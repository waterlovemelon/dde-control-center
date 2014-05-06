// This file is automatically generated, please don't edit manully.
import QtQuick 2.1
import Deepin.Widgets 1.0
import "../components"

BaseEditSection {
    id: sectionPppoe
    section: "pppoe"
    
    header.sourceComponent: EditDownArrowHeader{
        text: dsTr("PPPoE")
    }

    content.sourceComponent: Column { 
        EditLineTextInput {
            id: lineUsername
            key: "username"
            text: dsTr("Username")
        }
        EditLineTextInput {
            id: lineService
            key: "service"
            text: dsTr("Service")
        }
        EditLinePasswordInput {
            id: linePassword
            key: "password"
            text: dsTr("Password")
        }
    }
}
