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

#include <vector>

DeclareSimpleCommand(HelloWorld, "Autodeks", "2017");

MString vec_to_string(const MVector& vec)
{
    return MString{} + vec[0] + ", " + vec[1] + ", " + vec[2];
}

MStatus HelloWorld::doIt(const MArgList&)
{
    try
    {
        MSelectionList sel;
        MGlobal::getActiveSelectionList(sel);

        MDagPath d = MDagPath();
        sel.getDagPath(0, d);

        if (d.extendToShape() == MStatus::kFailure)
        { MGlobal::displayWarning("Selected node doesn't have a shape"); }

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

                if (groupNormal.isEquivalent(currNormal, 0.0001))
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

        return MS::kSuccess;
    }
    catch(...)
    {
        return MS::kFailure;
    }
}