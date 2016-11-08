#pragma once

#include <maya/MPxCommand.h>

class PlanarUnwrapAllGUI : public MPxCommand
{
public:
    PlanarUnwrapAllGUI() {};

    virtual MStatus doIt(const MArgList&) override;
    static  void*   creator();
};

class PlanarUnwrapAll : public MPxCommand
{
public:
    PlanarUnwrapAll() {};

    virtual MStatus doIt(const MArgList&) override;
    static  void*   creator();
};
