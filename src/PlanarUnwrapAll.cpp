#include <iostream>

#include "PlanarUnwrapAll.h"

#include <maya/MFnPlugin.h>
#include <maya/MSimple.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MIOStream.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MDagPath.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFnMeshData.h>
#include <maya/MDGModifier.h>
#include <maya/MArgParser.h>
#include <maya/MSyntax.h>

#include <cmath>

#include <vector>

namespace
{
    const char*     g_command_name = "SmartUnwrap";
    const char*     g_face_angle = "fa";
    const char*     g_face_angle_long = "faceAngle";

    const char*     g_gui =
        "import maya.cmds as cmds\n"
        "from maya import OpenMayaUI as omui\n"
        "from PySide2 import QtGui\n"
        "from PySide2 import QtWidgets\n"
        "from PySide2.QtCore import *\n"
        "from shiboken2 import wrapInstance\n"
        "\n"
        "\n"
        "class Widget(QtWidgets.QMainWindow):\n"
        "    def __init__(self, parent=None):\n"
        "        QtWidgets.QMainWindow.__init__(self, parent)\n"
        "\n"
        "        frame = QtWidgets.QWidget(self)\n"
        "        self.setCentralWidget(frame)\n"
        "\n"
        "        mainLayout = QtWidgets.QVBoxLayout(frame)\n"
        "\n"
        "        layout = QtWidgets.QHBoxLayout(frame)\n"
        "        mainLayout.addLayout(layout)\n"
        "\n"
        "        label = QtWidgets.QLabel('Separation Angle', self)\n"
        "        self.slider = QtWidgets.QSlider(Qt.Horizontal, self)\n"
        "        self.slider.setRange(0.0, 180.0)\n"
        "        self.sliderVal = QtWidgets.QLabel(\"0.0\", self)\n"
        "\n"
        "        self.slider.valueChanged.connect(self.slider_moved)\n"
        "\n"
        "        layout.addWidget(label)\n"
        "        layout.addWidget(self.slider)\n"
        "        layout.addWidget(self.sliderVal)\n"
        "\n"
        "        layout = QtWidgets.QHBoxLayout(frame)\n"
        "        mainLayout.addLayout(layout)\n"
        "\n"
        "        button = QtWidgets.QPushButton('Unwrap', self)\n"
        "        button.clicked.connect(self.start_unwrap)\n"
        "\n"
        "        self.checkbox = QtWidgets.QCheckBox(\"Auto Update\", self)\n"
        "        self.checkbox.setChecked(False)\n"
        "\n"
        "        layout.addWidget(self.checkbox)\n"
        "        layout.addWidget(button)\n"
        "\n"
        "    def slider_moved(self, position):\n"
        "        if self.checkbox.isChecked():\n"
        "            cmds.PlanarUnwrapAll(fa=float(position))\n"
        "        self.sliderVal.setText(str(position))\n"
        "\n"
        "    def start_unwrap(self):\n"
        "        cmds.PlanarUnwrapAll(fa=self.slider.value())\n"
        "\n"
        "mayaMainWinPtr = omui.MQtUtil_mainWindow()\n"
        "mayaMainWindow = wrapInstance(long(mayaMainWinPtr), QtWidgets.QWidget)\n"
        "\n"
        "win = Widget(mayaMainWindow)\n"
        "win.setWindowTitle(\"Smart Planar Unwrap\")\n"
        "win.resize(300, 50)\n"
        "win.show()";
};

constexpr double PI = 3.1415926535897932;
constexpr double RAD2DEG = 180.0 / PI;
constexpr double DEG2RAD = PI / 180.0;

MString vec_to_string(const MVector& vec)
{
    return MString{} + vec[0] + ", " + vec[1] + ", " + vec[2];
}

void* 
PlanarUnwrapAllGUI::
creator()
{
    return new PlanarUnwrapAllGUI{};
}

MStatus PlanarUnwrapAllGUI::doIt( const MArgList& )
{
    MGlobal::executePythonCommand(g_gui);

    return MS::kSuccess;
}

void* 
PlanarUnwrapAll::
creator()
{
    return new PlanarUnwrapAll{};
}

MStatus PlanarUnwrapAll::doIt(const MArgList& args)
{
    double tolerance = 5.0 * DEG2RAD;

    MSyntax syntax;
    syntax.addFlag(g_face_angle, g_face_angle_long, MSyntax::kDouble);

    MArgParser argP{syntax, args};

    if (argP.isFlagSet(g_face_angle))
    {
        double angle;
        auto status = argP.getFlagArgument(g_face_angle, 0, angle);
        if (MS::kSuccess == status)
        { tolerance = 1.0 - std::cos(angle * DEG2RAD); }
        else
        { status.perror("Failed to get face angle"); }
    }

    try
    {
        MSelectionList sel;

        MGlobal::getActiveSelectionList(sel);

        MDagPath d = MDagPath();
        sel.getDagPath(0, d);

        // if we don't have a shape, return
        if (d.extendToShape() == MStatus::kFailure)
        { 
            MGlobal::displayWarning("Selected node doesn't have a shape"); 
            return MS::kFailure;
        }

        MGlobal::displayInfo(MString("Tolerance: ") + tolerance);

        MGlobal::executeCommand("DeleteAllHistory");
        MGlobal::executeCommand("delete -all -ch");

        auto&& mesh = MFnMesh{d};
        auto faceIt = MItMeshPolygon{d};

        MFnDependencyNode dn(mesh.object());
        MGlobal::displayInfo(dn.name());

        //MGlobal::displayInfo(MString("Selected mesh has ") + faceIt.count() + MString(" faces"));

        std::vector<MObject> faces;

        while(faceIt.isDone() == false)
        {
            faces.push_back(faceIt.currentItem());
            MVector normal;
            { 
                MItMeshPolygon(d, faces.back()).getNormal(normal);
            }
            //{ faceIt.getNormal(normal); }

            //MGlobal::displayInfo(MString("Face Normal: ") + vec_to_string(normal));
            faceIt.next();
        }

        std::vector<std::vector<MObject>> faceGroups;

        while(faces.size() > 0)
        {
            std::vector<MObject> faceGroup;
            faceGroup.push_back(faces.front());
            faces.erase(faces.begin());

            MVector groupNormal;
            {
                MItMeshPolygon mp {d, faceGroup[0] };
                mp.getNormal(groupNormal);
            }

            auto itr = faces.begin();
            while (itr != faces.end())
            {
                MVector currNormal;
                {
                    MItMeshPolygon mp { d, *itr };
                    mp.getNormal(currNormal);
                }

                if (groupNormal.isEquivalent(currNormal, tolerance))
                {
                    faceGroup.push_back(*itr);
                    itr = faces.erase(itr);
                }
                else
                {
                    ++itr;
                }
            }

            faceGroups.push_back(faceGroup);
        }

        MGlobal::displayInfo(MString("Total Groups: ") + faceGroups.size());


        MSelectionList selList;

        for (auto&& group : faceGroups)
        {
            for (auto&& face : group)
            {
                selList.add(d, face);
            }

            MGlobal::setActiveSelectionList(selList);
            MDGModifier cmds;
            cmds.commandToExecute("polyPlanarProjection -ibd on -md b;");
            cmds.commandToExecute("setAttr \"polyPlanarProj1.projectionWidth\" 10; setAttr \"polyPlanarProj1.projectionHeight\" 10;");

            cmds.doIt();
            selList.clear();

            MGlobal::setActiveSelectionList(sel);
            MGlobal::executeCommand("DeleteAllHistory");
            MGlobal::executeCommand("delete -all -ch");
        }

        MGlobal::setActiveSelectionList(sel);
        MGlobal::executeCommand("polyMultiLayoutUV -lm 1 -sc 1 -rbf 1 -fr 1 -ps 0.2 -l 2 -gu 1 -gv 1 -psc 0 -su 1 -sv 1 -ou 0 -ov 0;");

        return MS::kSuccess;
    }
    catch(...)
    {
        return MS::kFailure;
    }
}

MStatus
initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Saad Khattak (C) 2016", "0.1");
    
    auto stat = plugin.registerCommand("PlanarUnwrapAll", PlanarUnwrapAll::creator);
    if (!stat)
    {
        stat.perror("registerCommand");
    }

    stat = plugin.registerCommand("PlanarUnwrapAllGUI", PlanarUnwrapAllGUI::creator);
    if (!stat)
    {
        stat.perror("registerCommand");
    }

    return stat;
}

MStatus
uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    
    auto stat = plugin.deregisterCommand("PlanarUnwrapAll");
    if (!stat)
    {
        stat.perror("deregisterCommand");
    }

    stat = plugin.deregisterCommand("PlanarUnwrapAllGUI");
    if (!stat)
    {
        stat.perror("deregisterCommand");
    }

    return stat;
}
