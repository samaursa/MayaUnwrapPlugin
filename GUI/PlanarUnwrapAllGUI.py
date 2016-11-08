import maya.cmds as cmds
from maya import OpenMayaUI as omui
from PySide2 import QtGui
from PySide2 import QtWidgets
from PySide2.QtCore import *
from shiboken2 import wrapInstance


class Widget(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        QtWidgets.QMainWindow.__init__(self, parent)

        frame = QtWidgets.QWidget(self)
        self.setCentralWidget(frame)

        mainLayout = QtWidgets.QVBoxLayout(frame)

        layout = QtWidgets.QHBoxLayout(frame)
        mainLayout.addLayout(layout)

        label = QtWidgets.QLabel('Separation Angle', self)
        self.slider = QtWidgets.QSlider(Qt.Horizontal, self)
        self.slider.setRange(0.0, 180.0)
        self.sliderVal = QtWidgets.QLabel("0.0", self)

        self.slider.valueChanged.connect(self.slider_moved)

        layout.addWidget(label)
        layout.addWidget(self.slider)
        layout.addWidget(self.sliderVal)

        layout = QtWidgets.QHBoxLayout(frame)
        mainLayout.addLayout(layout)

        button = QtWidgets.QPushButton('Unwrap', self)
        button.clicked.connect(self.start_unwrap)

        self.checkbox = QtWidgets.QCheckBox("Auto Update", self)
        self.checkbox.setChecked(False)

        layout.addWidget(self.checkbox)
        layout.addWidget(button)

    def slider_moved(self, position):
        if self.checkbox.isChecked():
            cmds.PlanarUnwrapAll(fa=float(position))
        self.sliderVal.setText(str(position))

    def start_unwrap(self):
        cmds.PlanarUnwrapAll(fa=self.slider.value())

mayaMainWinPtr = omui.MQtUtil_mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWinPtr), QtWidgets.QWidget)

win = Widget(mayaMainWindow)
win.setWindowTitle("Smart Planar Unwrap")
win.resize(300, 50)
win.show()
