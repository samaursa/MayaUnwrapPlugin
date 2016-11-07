#pragma once

#include <qtcore/qpointer.h>
#include <qtwidgets/qpushbutton.h>
#include <qtwidgets/qslider.h>
#include <maya/MPxCommand.h>

class tolerance_slider : public QSlider
{
    Q_OBJECT

public:
    tolerance_slider(QWidget* parent = nullptr)
        : QSlider(Qt::Horizontal, parent)
    {
        connect(this, SIGNAL(valueChanged(int)), this, SLOT(notifyToleranceChanged(int)));
    }

    virtual ~tolerance_slider()
    {
    }

signals:
    void toleranceChanged(double value);

public slots:
    void notifyToleranceChanged(int value)
    {
        double val = value/ 10.0;
        emit toleranceChanged(val);
    }

private:
    double m_tolerance;
};

class unwrap_button : public QPushButton
{
    Q_OBJECT

public:
    unwrap_button(const QString& text, QWidget* parent = nullptr);
    virtual ~unwrap_button();

public slots:
    void unwrap(bool checked);
    void tolerance(double value);

private:
    double m_tolerance;
};

class unwrap_cmd : public MPxCommand
{
public:
    static void     cleanup();
    static void*    creator() { return new unwrap_cmd(); }

    MStatus         doIt(const MArgList& args);

    static QPointer<unwrap_button>      m_button;
    static QPointer<tolerance_slider>   m_tolerance;
    static const MString                m_command_name;
};