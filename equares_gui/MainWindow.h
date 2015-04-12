/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScriptEngine>
#include "sim_types.h"
#include "equares_core/equares_core.h"
#include "SimVisualizer.h"
#include "Animator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    QScriptEngine m_jsEngine;
    GuiSimulation m_sim;
    SimVisualizer *m_simVis;
    Animator *m_animator;

    static Box::Ptr toBox(const QScriptValue& scriptBox);
    static GuiLinkTarget toGuiLinkTarget(const QScriptValue& scriptLinkTarget);
    static void commitActivationData(QVector<int>& callerId, ActivationData& d, Animator *animator);
    static void finalizeActivationData(QVector<int>& callerId, int id, int result, Animator *animator);
    static GuiLinkTarget parseActivatorTarget(const QString& s);

private slots:
    void openFile();
    void openAnimation();
};

#endif // MAINWINDOW_H
