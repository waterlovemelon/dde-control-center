dss
===

deepin-system-settings 

导入模块class的方法：
===================
1. 注意在frame/main.py中有两个用注释包起来的区域
2. 导入class，然后初始化，设置相应的context

Note: 导入的class是全局的，其实在任何模块都是可用的，为了防止命名冲突，从模块中
导入的class，在qml中使用的全局变量名称统一加上"module_"的前缀

添加系统设置新模块内容的方法：
======================

请在frame/DssLaunchPad.qml的头部的列表中按照例子添加相应的入口

三个参数的意义：

列表中显示名称，入口qml，图标名称（这个全称为preferences-*.png
可以在../frame/images/文件夹中添加和查看）

关于系统托盘：
============
目前需要展开显示的托盘界面放在了./frame/trayicon_modules/文件夹之下，入口统一
为`iconId`/main.qml
