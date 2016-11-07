#include <iostream>

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

#include <qtcore/qpointer.h>
#include <qtwidgets/qvboxlayout>

#include <vector>

#include "PlanarUnwrapAll.h"

namespace {

    MString vec_to_string(const MVector& vec)
    {
        return MString { } +vec[0] + ", " + vec[1] + ", " + vec[2];
    }

};

unwrap_button::
unwrap_button(const QString& text, QWidget* parent)
    : QPushButton(text, parent)
    , m_tolerance(0.1)
{ 
}

unwrap_button::
~unwrap_button()
{ }

void
unwrap_button::
unwrap(bool tolerance)
{
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
            return;
        }

        MGlobal::executeCommand("DeleteAllHistory");
        MGlobal::executeCommand("delete -all -ch");

        auto&& mesh = MFnMesh { d };
        auto faceIt = MItMeshPolygon { d };

        MFnDependencyNode dn(mesh.object());
        MGlobal::displayInfo(dn.name());

        //MGlobal::displayInfo(MString("Selected mesh has ") + faceIt.count() + MString(" faces"));

        std::vector<MObject> faces;

        while (faceIt.isDone() == false)
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

        while (faces.size() > 0)
        {
            std::vector<MObject> faceGroup;
            faceGroup.push_back(faces.front());
            faces.erase(faces.begin());

            MVector groupNormal;
            {
                MItMeshPolygon mp { d, faceGroup[0] };
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

                if (groupNormal.isEquivalent(currNormal, 0.1))
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
    }
    catch (...)
    {
        MGlobal::displayError("An internal error occured");
    }
}

void
unwrap_button::
tolerance(double value)
{
    m_tolerance = value;
}

//////////////////////////////////////////////////////////////////////////

QPointer<unwrap_button>     unwrap_cmd::m_button;
QPointer<tolerance_slider>  unwrap_cmd::m_tolerance;
const MString               unwrap_cmd::m_command_name("PlanarUnwrapAll");

void
unwrap_cmd::
cleanup()
{ 
    if (!m_button.isNull()) delete m_button;
}

MStatus
unwrap_cmd::
doIt(const MArgList& args)
{
    if (m_button.isNull())
    {
        m_button = new unwrap_button("Planar Unwrap All");
        m_button->connect(m_button, SIGNAL(clicked(bool)), m_button, SLOT(unwrap(bool)));
        m_button->show();
    }
    else
    {
        m_button->showNormal();
        m_button->raise();
    }

    return MS::kSuccess;
}

MStatus 
initializePlugin(MObject plugin)
{
    MStatus		st;
    MFnPlugin	pluginFn(plugin, "Saad Khattak", "0.1", "Any", &st);

    if (!st)
    {
        MGlobal::displayError(
            MString("PlanarUnwrapAll - could not initialize plugin: ")
            + st.errorString()
        );
        return st;
    }

    //	Register the command.
    st = pluginFn.registerCommand(unwrap_cmd::m_command_name, unwrap_cmd::creator);

    if (!st)
    {
        MGlobal::displayError(
            MString("PlanarUnwrapAll - could not register '")
            + unwrap_cmd::m_command_name + "' command: "
            + st.errorString()
        );
        return st;
    }

    return st;
}


MStatus uninitializePlugin(MObject plugin)
{
    MStatus		st;
    MFnPlugin	pluginFn(plugin);

    if (!st)
    {
        MGlobal::displayError(
            MString("PlanarUnwrapAll - could not uninitialize plugin: ")
            + st.errorString()
        );
        return st;
    }

    //	Make sure that there is no UI left hanging around.
    unwrap_cmd::cleanup();

    //	Deregister the command.
    st = pluginFn.deregisterCommand(unwrap_cmd::m_command_name);

    if (!st)
    {
        MGlobal::displayError(
            MString("PlanarUnwrapAll - could not deregister '")
            + unwrap_cmd::m_command_name + "' command: "
            + st.errorString()
        );
        return st;
    }

    return st;
}